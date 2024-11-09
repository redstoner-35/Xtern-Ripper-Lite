#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "ADCCfg.h"
#include "Watchdog.h"

//�ڲ�SFR
sbit RevPGate=RevProtIOP^RevProtIOx; //���ӱ���MOSFET
sbit DCDCEN=DCDCENIOP^DCDCENIOx; //DCDCʹ�ܹ���
sbit LShuntSEL=LShuntSelIOP^LShuntSelIOx; //�����¹⵵ר�÷�����ѡ��λ
sbit HShuntSEL=HShuntSelIOP^HShuntSelIOx; //�����������ѡ��λ

//�ڲ�����
xdata int Current; //Ŀ�����(mA)
static xdata int CurrentBuf;

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
  float buf,Err;
	//����Ƿ��ɿ��Ź����¸�λ	
	if(GetIfWDogCauseRST())	
		{
		ErrCode=Fault_MPUHang; //ָʾ�����ɵ�Ƭ����������
		if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
		return;
		}
	//׼���������
	if(Data.RawBattVolt<5.5)return; //�����ѹ���ͱ����󱨣��������
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
	  if(Data.OutputVoltage>7.1)break; //�����ѹ����
		retry--;
		}
	while(retry>0);
	//DCDC����ʧ��
	if(retry==0)
		{
		DCDCEN=0; //��DCDCEN=0
	  ErrCode=Fault_DCDCFailedToStart;
		if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
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
	if(retry==0)
		{
	  ErrCode=Fault_DCDCENOOC;
		if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
		}
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
	buf=((float)Current*Offset)*ShuntmOhm; //���봫�����ĵ���(mA)�����Լ���������ֵ(mR)�õ��˷Ŷ�������ѹ(uV)
	buf/=(float)1000; //uVתmV
	buf/=((float)VdivDownResK/(float)(VdivUpResK+VdivDownResK+PWMDACResK)); //���˷Ŷ�������ѹ���Ե���ķ�ѹ�����õ�DAC�˵ĵ�ѹ
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
	//������Ч���ظ�����
	if(CurrentBuf==Current)return;
	CurrentBuf=Current;
	//����С�ڵ���0���ر��������
	if(CurrentBuf<=0)
		{
		PWMDuty=0;
		RevPGate=CurrentBuf==-1?1:0;
		DCDCEN=0;
		LShuntSEL=0;
		HShuntSEL=0;
		}
	//ʹ�ø���ͨ��
	else if(CurrentBuf<AUXChannelImax)
		{
		PWMDuty=Duty_Calc(AUXChannelShuntmOhm,CurrentBuf,LowShuntIOffset);
		RevPGate=Data.MCUVDD<9.5?1:0;   //��������������빦�ʲ���ʱ�¹رշ�����FET��ʡ����
		DCDCEN=1;
		LShuntSEL=1;  
		HShuntSEL=0;  //����DCDC��ѡ�������ͨ��
		}
	//�������ڸ���ͨ�����ޣ�ʹ����ͨ��
	else
		{
		PWMDuty=Duty_Calc(MainChannelShuntmOhm,CurrentBuf,HighShuntIOffset);
		RevPGate=1;   //��������ã��򿪷�����FET�����Ч
		DCDCEN=1;
		LShuntSEL=0;  
		HShuntSEL=1;  //����DCDC��ѡ�������ͨ��
		}	
	//PWMռ�ձȷ���������������
	IsNeedToUploadPWM=1;
	}
