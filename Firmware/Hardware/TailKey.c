#include "PinDefs.h"
#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "TailKey.h"
#include "ADCCfg.h"
#include "PWMCfg.h"

//����β����������
#define EnableMechTailKey //����������β������

//�ڲ�ȫ�ֱ���
static xdata char TailKeyCount=0; //β���������µĴ���
static char TailSenTIM=0; //��ʱ����β�����Ķ�ʱ��
static bit IsTailKeyPressed=0;
static volatile unsigned char IsEnterLowPowerMode=0xFF;

//�ⲿ���ñ���
char TailKeyTIM;

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
	delay_ms(5); 
	CNIF=0; //�ӳ�10mS�������Flag���򿪱Ƚ����ж�ǰ��Ҫ���Flag��
	CNIE=0x01; //ʹ��ACMP0�жϣ�β����⼤��
	}
#endif
//β����������Ҫ��ģ��Ƚ�����ʼ��,�Լ����򿪹ؼ��
void TailKey_Init(void)
	{
	#ifdef EnableMechTailKey
	char wait;
	extern bit IsPosTailKey;
	//���ñȽ���ACMP0
  C0CON0=0x09; //�Ƚ�������ģʽ��ֹ��������ΪC0P1��������Ϊ�ڲ�REF	
	C0CON2=0x19; //�Ƚ�������Ϊʹ���˲����ܣ��˲�ʱ�䳣��Ϊ256*1/48MHz=5.33uS�����������
	C0HYS=0x00; //���ó���
	CNVRCON=0x38; //�Ƚ�����������Ļ�׼��ѹΪ�ڲ�1.2V��϶��׼����10/20������ѹ�õ�0.6V
	CNFBCON=0x05; //ʹ�ܱȽ���0��ɲ�����ܣ��ڸ�����ʱ��ֹPWM���
	EIP1=0x80; //�Ƚ����жϱ���ʵʱ��Ӧ��������Ϊ�������ȼ�
		
	//ʹ��β����⿪ʼ������򿪹صĶ���
	if(!IsPosTailKey)return; //���õ�������Ϊ���򿪹أ��˳����
	wait=TailKeyRelTime*30;
	EnableTailDetect(); 
	do
		{
		TailKeyTIM=TailKeyRelTime; //��β����ʱ��������Ϊ�ɿ���⣬����������£��򽫻����β������Ȼ���������������ʱ���õȴ�ʱ��
		delay_ms(5);
		TailKey_Handler(); //�ȴ���Ӧ
		if(!TailKeyTIM)wait=TailKeyRelTime*30; //β�����£���Ӧ����
		}
	while(--wait);
	//�����Ϻ�رձȽ���
	CNIE=0x00; //�رձȽ����ж�
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
	//β�����ذ��¼�ʱ
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
  if(IsEnterLowPowerMode)return;
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
	}
