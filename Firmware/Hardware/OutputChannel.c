#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "ModeControl.h"
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
	LShuntSEL=0;
	HShuntSEL=0;
	RevPGate=0; //�رշ����Ӽ��PIN
	PWM_ForceSetDuty(1); //��PWMDAC���һ����ֵ		
	DCDCEN=1; //��DCDCEN=1
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
	int TargetCurrent;
	bit IsAux;
	//���ݵ�ǰ�������������״̬�ó�ʵ��Ҫ����¿ؼ��㺯���ĵ���
	if(TailKeyTIM<(TailKeyRelTime+1))TargetCurrent=0; //��ǰ�������ģʽ�������ر����
	else if(Current>TurboCurrent)TargetCurrent=TurboCurrent; //���Ŀ���������������ֵ�������Ŀ������
	else TargetCurrent=Current; //���յ�λ״̬����������Ľ����д  
	//�¿ؼ���
  if(TargetCurrent>ThermalILIMCalc())TargetCurrent=ThermalILIMCalc(); //�¿ط����ĵ������Ƴ�������ֵ
	//������Ч���ظ�����
	if(CurrentBuf==TargetCurrent)return;
	//����LED�ĵ���б��������
	if(TargetCurrent-CurrentBuf>6000)IsSlowRamp=1; //��⵽�ǳ���ĵ���˲̬������屬�����������
  if(IsSlowRamp&&TargetCurrent>0)
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
	//����С�ڵ���0���ر��������
	if(CurrentBuf<=0)
		{
		DCDCEN=0;
	  PWMDuty=0;			//DCDC���رգ��������
		IsDCDCEnabled=0;  //���DCDC�ѱ��ر�
		RevPGate=CurrentBuf==-1?1:0; //��λ������MOS
		LShuntSEL=0;
		HShuntSEL=0;  //��λ����ѡ��MOS
		}
	//���������ѡ���Ӧͨ��
	else
		{
		//�ж��Ƿ�ʹ�ø���ͨ��
		IsAux=CurrentBuf<AUXChannelImax?1:0;
		//EN���ڹر�״̬������DCDC����PWMDAC=0�ȴ�һ��ʱ��������������嵼����˸������
		if(!IsDCDCEnabled)
				{
				RevPGate=~IsAux;   //���빦�ʲ���ʱ�¹رշ�����FET��ʡ����
				DCDCEN=1;  
				LShuntSEL=IsAux;  
				HShuntSEL=~IsAux;  //����DCDC��ѡ���Ӧͨ�����ӳ�5mS�����͸���
				delay_ms(5); 
				IsDCDCEnabled=1; //���DCDC�Ѿ���ʼ����
				}
		//�������ռ�ձ�
		PWMDuty=Duty_Calc(IsAux?AUXChannelShuntmOhm:MainChannelShuntmOhm,CurrentBuf);
		}
	//��������ϴ�PWM��ֵ
	IsNeedToUploadPWM=1;
	}
