#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "SpecialMode.h"
#include "TempControl.h"
#include "OutputChannel.h"
#include "ADCCfg.h"
#include "TailKey.h"
#include "Strap.h"
#include "SelfTest.h"

//�ڲ�SFR
sbit RevPGate=RevProtIOP^RevProtIOx; //���ӱ���MOSFET
sbit DCDCEN=DCDCENIOP^DCDCENIOx; //DCDCʹ�ܹ���
sbit LShuntSEL=LShuntSelIOP^LShuntSelIOx; //�����¹⵵ר�÷�����ѡ��λ
sbit HShuntSEL=HShuntSelIOP^HShuntSelIOx; //�����������ѡ��λ

//�ڲ�����
bit IsSlowRamp; //��������Ramp
volatile bit IsDCDCEnabled; //DCDC�Ƿ�ʹ��
xdata int CurrentBuf; //�洢��ǰ�Ѿ��ϴ��ĵ���ֵ 
xdata volatile int Current; //Ŀ�����(mA)

//��ʼ������
void OutputChannel_Init(void)
	{
	GPIOCfgDef OCInitCfg;
	//���ýṹ��
	OCInitCfg.Mode=GPIO_Out_PP;
  OCInitCfg.Slew=GPIO_Fast_Slew;		
	OCInitCfg.DRVCurrent=GPIO_High_Current; //��MOSFET,��Ҫ������б��
	//��ʼ��bit
	RevPGate=0;
	DCDCEN=0;
	LShuntSEL=0;
	HShuntSEL=0;
	//��ʼ����IO	
	GPIO_ConfigGPIOMode(RevProtIOG,GPIOMask(RevProtIOx),&OCInitCfg);	
	GPIO_ConfigGPIOMode(DCDCENIOG,GPIOMask(DCDCENIOx),&OCInitCfg);		
	GPIO_ConfigGPIOMode(LShuntSelIOG,GPIOMask(LShuntSelIOx),&OCInitCfg);		
	GPIO_ConfigGPIOMode(HShuntSelIOG,GPIOMask(HShuntSelIOx),&OCInitCfg);			
	//ϵͳ�ϵ�ʱ��������Ϊ0
	Current=0;
	CurrentBuf=0;
	IsDCDCEnabled=0;
	IsSlowRamp=0;
	}

//���ͨ����������
void OutputChannel_TestRun(void)
	{
	int retry=64,i;
	xdata float LastOutput[5]={0};
  xdata float buf,Err;
	//׼���������
	if(Data.RawBattVolt<5.5||CurrentMode->ModeIdx!=Mode_OFF)return; //�����ѹ���ͱ����󱨣������ϴιػ�ǰû��Ϩ�ƣ�Ϊ�˾�������������
	PWM_ForceSetDuty(1); //��PWMDAC���һ����ֵȷ���˷��������ڵ�����		
	DCDCEN=1; //��DCDCEN=1������DCDC
	//�ȴ�DCDC����	
	do
		{
		delay_ms(5);
		SystemTelemHandler();
	  if(Data.OutputVoltage>4.2)break; //�����ѹ����
		retry--;
		}
	while(retry>0);
	//DCDC����ʧ��
	if(retry==0)
		{
		DCDCEN=0; //��DCDCEN=0
	  ReportError(Fault_DCDCFailedToStart); //�������
	  return;
		}
	//�������EN���Ƶļ��
	retry=100; //��λ��ʱ�ȴ�
	DCDCEN=0; //��DCDCEN=0
	delay_ms(20); //�ӳ�20ms
	PWM_ForceSetDuty(0); //�ر�PWMDAC
	do
		{
		SystemTelemHandler();
		delay_ms(10);
		//��������
		for(i=4;i>0;i--)LastOutput[i]=LastOutput[i-1];	
		LastOutput[0]=Data.OutputVoltage;
		//��ƽ��	
		buf=0;
    for(i=0;i<5;i++)buf+=LastOutput[i];
    buf/=(float)5;			
		//����������ÿ�����ݵĲ��
		Err=0;	
		for(i=0;i<5;i++)Err+=fabs(buf-LastOutput[i]);
	  if(retry<93&&Err>0.5)break; //�����ѹ����˥���У�����
		retry--;
		}
	while(retry>0);
	//DCDCֹͣʧ�ܣ�EN���ܿأ�����
	if(retry==0)ReportError(Fault_DCDCENOOC);
	}	

//�ڲ����ڼ���PWMDACռ�ձȵĺ���	
static float Duty_Calc(float ShuntmOhm,int CurrentInput)
	{
	float buf;
	char Offset;
	//���㲹��ֵ
	Offset=CurrentMode->Offset;
	if(CurrentMode->ModeIdx==Mode_Ramp)Offset+=(char)(CurrentInput/205); //�޼�����ģʽ���Զ�����offset
	//����ʵ��ռ�ձ�
	buf=(float)CurrentInput*ShuntmOhm; //���봫�����ĵ���(mA)�����Լ���������ֵ(mR)�õ��˷Ŷ�������ѹ(uV)
	buf/=(float)1000; //uVתmV
	buf/=((float)VdivDownResK/(float)(VdivUpResK+VdivDownResK+PWMDACResK)); //���˷Ŷ�������ѹ���Ե���ķ�ѹ�����õ�DAC�˵ĵ�ѹ
	buf*=(float)Offset/(float)100; //���Խ���ϵ����������
	buf/=Data.MCUVDD*(float)1000; //�����Ŀ��DAC�����ѹ��PWMDAC�����������ѹ(MCUVDD)֮��ı�ֵ
	buf*=100; //ת��Ϊ�ٷֱ�
	//������	
	return buf>100?100:buf;
	}
	
//���ͨ������
void OutputChannel_Calc(void)
	{
	int TargetCurrent,ILIM;
	bit IsMainChannel;
	//���ݵ�ǰ�������������״̬�ó�ʵ��Ҫ����¿ؼ��㺯���ĵ���
	if(TailKeyTIM<(TailKeyRelTime+1))TargetCurrent=0; //��ǰ�������ģʽ�������ر����
	else TargetCurrent=Current; //���յ�λ״̬����������Ľ����д  
	//�¿ؼ���
	ILIM=ThermalILIMCalc();
	if(ILIM>TurboCurrent)ILIM=TurboCurrent; //����ֵ���ڼ�������ֵ
  if(TargetCurrent>ILIM)TargetCurrent=ILIM; //�¿ط��������е�������Ŀ��ֵ����������
	//������Ч���ظ�����
	if(CurrentBuf==TargetCurrent)return;	
	//��������0���������ѡ���Ӧͨ��
	if(TargetCurrent>0)
		{
		//����LED�ĵ���б��������
		if(TargetCurrent-CurrentBuf>6000)IsSlowRamp=1; //��⵽�ǳ���ĵ���˲̬������屬�����������
		if(!SysMode&&IsSlowRamp)
			{
			if(CurrentBuf==0)CurrentBuf=1500; //����Ϊ0��1500��ʼ���
			else switch(CurrentMode->ModeIdx)
				{
				case Mode_Strobe:CurrentBuf+=3000;break;
				case Mode_SOS:CurrentBuf+=500;break;
				default:CurrentBuf+=20;
				}
			if(CurrentBuf>=TargetCurrent)
				{
				IsSlowRamp=0;
				CurrentBuf=TargetCurrent; //�޷���������Ŀ�������������ֵ
				}
			}
		else CurrentBuf=TargetCurrent; //ֱ��ͬ��		
		//�ж��Ƿ�ʹ�ø���ͨ��
		IsMainChannel=CurrentBuf>AUXChannelImax?1:0;
		//EN���ڹر�״̬������DCDC����PWMDAC=0�ȴ�һ��ʱ��������������嵼����˸������
		if(!IsDCDCEnabled)
				{
				RevPGate=IsMainChannel;   //���빦�ʲ���ʱ�¹رշ�����FET��ʡ����
				DCDCEN=1;  
				LShuntSEL=IsMainChannel?0:1;  
				HShuntSEL=IsMainChannel;  //����DCDC��ѡ���Ӧͨ��
				delay_ms(CurrentMode->ModeIdx==Mode_Strobe?2:10); //��PWMDAC���=0����һ��ʱ���������ݽ���Ԥ��磬Ȼ�����͸���ֵ������˸
				IsDCDCEnabled=1; //���DCDC�Ѿ���ʼ����
				}
		//�������ռ�ձ�
		PWMDuty=Duty_Calc(IsMainChannel?MainChannelShuntmOhm:AUXChannelShuntmOhm,CurrentBuf);
		}
	//��������0���ر��������
	else
		{
		DCDCEN=0;
	  PWMDuty=0;			//DCDC���رգ��������
		IsDCDCEnabled=0;  //���DCDC�ѱ��ر�
		RevPGate=0; //��λ������MOS
		LShuntSEL=0;
		HShuntSEL=0;  //��λ����ѡ��MOS
		//�������Ϊ0
		CurrentBuf=0;
		}	
	//��������ϴ�PWM��ֵ
	IsNeedToUploadPWM=1;
	}
