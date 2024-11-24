#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "ADCCfg.h"
#include "Watchdog.h"
#include "TailKey.h"

//�ڲ�SFR
sbit RevPGate=RevProtIOP^RevProtIOx; //���ӱ���MOSFET
sbit DCDCEN=DCDCENIOP^DCDCENIOx; //DCDCʹ�ܹ���
sbit LShuntSEL=LShuntSelIOP^LShuntSelIOx; //�����¹⵵ר�÷�����ѡ��λ
sbit HShuntSEL=HShuntSelIOP^HShuntSelIOx; //�����������ѡ��λ

//�ڲ�����
xdata int Current; //Ŀ�����(mA)
static xdata int CurrentBuf;
static xdata char IsEnableDCDCCounter=0; //��ʱ����DCDC�ļ�ʱ��

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
	}

//���ͨ����������
void OutputChannel_TestRun(void)
	{
	int retry=64,i;
	xdata float LastOutput[5]={0};
  xdata float buf,Err;
	//����Ƿ��ɿ��Ź����¸�λ	
	if(GetIfWDogCauseRST())	
		{
		ReportError(Fault_MPUHang); //ָʾ�����ɵ�Ƭ����������
		return;
		}
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
	
//���ͨ���������ߵĲ���
void OutputChannel_DeInit(void)
	{
	//��λ����������
	Current=0;
	CurrentBuf=0;
	//�ر��������
	RevPGate=0;
	DCDCEN=0;
	LShuntSEL=0;
	HShuntSEL=0;	
	}
	
//�ڲ����ڼ���PWMDACռ�ձȵĺ���	
static float Duty_Calc(float ShuntmOhm,int Current,float Offset)
	{
	float buf;
	buf=(float)Current*ShuntmOhm; //���봫�����ĵ���(mA)�����Լ���������ֵ(mR)�õ��˷Ŷ�������ѹ(uV)
	buf/=(float)1000; //uVתmV
	buf/=((float)VdivDownResK/(float)(VdivUpResK+VdivDownResK+PWMDACResK)); //���˷Ŷ�������ѹ���Ե���ķ�ѹ�����õ�DAC�˵ĵ�ѹ
	buf*=Offset; //���Խ���ϵ����������
	buf/=Data.MCUVDD*(float)1000; //�����Ŀ��DAC�����ѹ��PWMDAC�����������ѹ(MCUVDD)֮��ı�ֵ
	buf*=100; //ת��Ϊ�ٷֱ�
	//�����޷��ͽ�����	
	if(buf>100)buf=100;
	if(buf<0)buf=0;
	return buf;
	}
	
//���ͨ������
void OutputChannel_Calc(void)
	{
	//��ʱ����DCDC	
	if(IsEnableDCDCCounter&&!IsNeedToUploadPWM)	
		{
		IsEnableDCDCCounter--;
		if(!IsEnableDCDCCounter)DCDCEN=1; //ʱ�䵽����DCDC
		}
	//������Ч���ظ�����
	if(TailKeyTIM<(TailKeyRelTime+1))Current=0; //��ǰ�������ģʽ�������ر����
	if(CurrentBuf==Current)return;
	CurrentBuf=Current;
	//����С�ڵ���0���ر��������
	if(CurrentBuf<=0)
		{
		if(CurrentMode->ModeIdx!=Mode_Strobe) //�Ǳ���ģʽ������PWMDAC��׼���
			{
	    PWMDuty=0;
		  IsNeedToUploadPWM=1;
			}
		RevPGate=CurrentBuf==-1?1:0;
		DCDCEN=0;
		LShuntSEL=0;
		HShuntSEL=0;
		}
	//ʹ�ø���ͨ��
	else if(CurrentBuf<AUXChannelImax)
		{
		PWMDuty=Duty_Calc(AUXChannelShuntmOhm,CurrentBuf,LowShuntIOffset);
		RevPGate=0;   //���빦�ʲ���ʱ�¹رշ�����FET��ʡ����
		if(!DCDCEN)IsEnableDCDCCounter=PWMDACSettleDelay; //�����ǰDCDC�ǹر�״̬����ʱһ��ʱ���ٴ�
		LShuntSEL=1;  
		HShuntSEL=0;  //����DCDC��ѡ�������ͨ��
		IsNeedToUploadPWM=1; //��Ҫ����PWM���
		}
	//�������ڸ���ͨ�����ޣ�ʹ����ͨ��
	else
		{
		PWMDuty=Duty_Calc(MainChannelShuntmOhm,CurrentBuf,HighShuntIOffset);
		RevPGate=1;   //��������ã��򿪷�����FET�����Ч
		DCDCEN=1; 
		LShuntSEL=0;  
		HShuntSEL=1;  //����DCDC��ѡ�������ͨ��
		IsNeedToUploadPWM=1; //��Ҫ����PWM���
		}
	}
