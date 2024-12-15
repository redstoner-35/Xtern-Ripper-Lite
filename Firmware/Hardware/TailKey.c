#include "PinDefs.h"
#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "TailKey.h"
#include "SelfTest.h"
#include "ADCCfg.h"
#include "PWMCfg.h"

//����β����������
//#define EnableMechTailKey //����������β������

//�ڲ�ȫ�ֱ���
static xdata char TailKeyCount=0; //β���������µĴ���
static char TailSenTIM; //��ʱ����β�����Ķ�ʱ��
static bit IsTailKeyPressed=0;
volatile bit IsPOSTKPressed=0; //����β���Ƿ���
static volatile unsigned char IsEnterLowPowerMode=0xFF;

//�ⲿ���ñ���
#ifndef EnableMechTailKey
char TailKeyTIM=TailKeyRelTime+1;
#else
char TailKeyTIM;
#endif

#ifdef EnableMechTailKey
//�Ƚ����ж�
void ACMP_IRQHandler(void)  interrupt ACMP_VECTOR 
{
	HShuntSelIOP&=~(0x01<<HShuntSelIOx);
	LShuntSelIOP&=~(0x01<<LShuntSelIOx);
	RevProtIOP&=~(0x01<<RevProtIOx);
	DCDCENIOP&=~(0x01<<DCDCENIOx); //��������DCDCEN
	RedLEDIOP&=~(0x01<<RedLEDIOx);
  GreenLEDIOP&=~(0x01<<GreenLEDIOx); //��LED����Ϩ��
	IsEnterLowPowerMode=0x00;
	//��Ӧ����
	CNIF=0;	
}	
//ʹ��β������
static void EnableTailDetect(void)
	{
	C0CON0|=0x80; //��C0EN=1���Ƚ�����ʼ����
	delay_ms(20); 
	CNIF=0; //�ӳ�20mS�������Flag���򿪱Ƚ����ж�ǰ��Ҫ���Flag��
	CNIE=0x01; //ʹ��ACMP0�жϣ�β����⼤��
	}
#endif
	
void TailKey_Init(void)	
	{
  C0CON0=0x09; //�Ƚ�������ģʽ��ֹ��������ΪC0P1��������Ϊ�ڲ�REF	
	C0CON2=0x19; //�Ƚ�������Ϊʹ���˲����ܣ��˲�ʱ�䳣��Ϊ256*1/48MHz=5.33uS�����������
	C0HYS=0x00; //���ó���
	CNVRCON=0x38; //�Ƚ�����������Ļ�׼��ѹΪ�ڲ�1.2V��϶��׼����10/20������ѹ�õ�0.6V
	CNFBCON=0x05; //ʹ�ܱȽ���0��ɲ�����ܣ��ڸ�����ʱ��ֹPWM���
	EIP1=0x80; //�Ƚ����жϱ���ʵʱ��Ӧ��������Ϊ�������ȼ�
	}

//β����������Ҫ��ģ��Ƚ�����ʼ��,�Լ����򿪹ؼ��(�ϵ��ʱ���õ�)
void TailKey_POR_Init(void)
	{
	#ifdef EnableMechTailKey
	unsigned char wait;
	extern bit IsPosTailKey;
	bit TKState=1;
	//��ʼ���Ƚ�������λβ����ʱ��
	TailKey_Init();	
	TailKeyTIM=0; 
	//ʹ��β����⿪ʼ������򿪹صĶ���
	if(!IsPosTailKey)return; //���õ�������Ϊ���򿪹أ��˳����
	wait=30;
	C0CON0|=0x80; //��C0EN=1���Ƚ�����ʼ����
	do
		{		
		//������ʱ
		delay_ms(7);
		//���Ƚ���״̬
		IsEnterLowPowerMode<<=1;
		if(C0CON1&0x80)IsEnterLowPowerMode++;
		//���ݿ���״̬���ж���	
		if(IsEnterLowPowerMode==0xFF)
			{
			TKState=1;
			wait--; //β����������ͨ�磬�ݼ���ʱ��
			}
    else if(!(IsEnterLowPowerMode&0x1F)) //�����ɿ��㹻ʱ��˵���ǵ㶯����			
		  {
			wait=30; //β�����£���λ��ʱ��
			if(!TKState)continue; //β����ǰû��ͨ���㹻����ʱ�䣬�������ж�
			IsPOSTKPressed=1;
			IsEnterLowPowerMode=0; //�����������ȴ�����ȷʵ�����ٶ���
			TailKeyCount++; //������Ч�İ�������
		  TKState=0; //��ǿ����ɿ�
			}		
		}
	while(wait);
	//�����ϵĺ���
	if(IsPOSTKPressed)TailKeyTIM=TailKeyRelTime; //β���ж������룬��β����Ӧ��ʱ������Ϊ����ѭ�����е�β�������������Ӧ
	C0CON0&=0x7F; //��C0EN=0���Ƚ���ֹͣ����	
	#endif
	}

//��ȡβ�����µĴ���
char GetTailKeyCount(void)
	{
	char buf;
	if(!IsTailKeyPressed)return 0;
	else 
		{
		buf=TailKeyCount;
		TailKeyCount=0;
		IsTailKeyPressed=false;
		}
	return buf;
	}	

//β����ʱ��
void TailKeyCounter(void)
	{
	//�ֵ翪��״̬�²Ž���β�����
	#ifdef EnableMechTailKey
	if(CurrentMode->ModeIdx==Mode_OFF||CurrentMode->ModeIdx==Mode_Fault)
		{
	  CNIE=0x00; //�رձȽ����ж�
	  C0CON0&=0x7F; //��C0EN=0���Ƚ���ֹͣ����
		TailSenTIM=0;			
		}
	else if(TailSenTIM<3)TailSenTIM++;
	else if(TailSenTIM==3)
		{
		EnableTailDetect();
		TailSenTIM++;
		}
	//β�����ذ���֮�����ڼ��������ΰ��µļ�ʱģ��
	if(TailKeyTIM<TailKeyRelTime)TailKeyTIM++;
	else if(TailKeyTIM==TailKeyRelTime)
		{
		TailKeyTIM++;
		if(TailKeyCount>0)IsTailKeyPressed=1;
		}
	#endif
	}	
	
//β���߼�����	
void TailKey_Handler(void)
	{
	#ifdef EnableMechTailKey
	//β�������û���û�а��£��˳�����
  if(!(C0CON0&0x80)||IsEnterLowPowerMode)return;
	//ѭ���ȴ���ť���½�ͨ�ָ�����
	do
		{
		delay_ms(5);
		IsEnterLowPowerMode<<=1;
		if(C0CON1&0x80)IsEnterLowPowerMode++;
		}			
	while(IsEnterLowPowerMode!=0xFF);
  //�ָ����磬���а����߼�����
  PWM_Enable(); //����ʹ��PWM		
	TailKeyCount++;
	TailKeyTIM=0;  //β���������£������¼���λ��ʱ��
	#endif
	}
