#include "PinDefs.h"
#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "TailKey.h"
#include "ADCCfg.h"

//�ⲿ����
void SystemPeripheralCTRL(bit IsEnable);//����/��������ϵͳ����
static xdata char TailKeyCount=0;
xdata char TailKeyTIM=TailKeyRelTime+1;
static xdata bool IsTailKeyPressed=false;

//�ڲ�ȫ��
static volatile unsigned char IsEnterLowPowerMode=0xFF;

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

//β�����س�ʼ��
void TailKey_Init(void)
	{
  C0CON0=0x09; //�Ƚ�������ģʽ��ֹ��������ΪC0P1��������Ϊ�ڲ�REF	
	C0CON2=0x00; //�Ƚ�������Ϊ��ֹ�˲����ܣ����������
	C0HYS=0x00; //���ó���
	CNVRCON=0x39; //�Ƚ�����������Ļ�׼��ѹΪ�ڲ�1.2V��϶��׼����11/20������ѹ�õ�0.66V
	CNFBCON=0x00; //�ر����бȽ�����PWMɲ������
	C0CON0|=0x80; //��C0EN=1���Ƚ�����ʼ����
	
	//ʹ���ж�
	CNIF=0; //�򿪱Ƚ����ж�
	EIP1=0x80; //�Ƚ����жϱ���ʵʱ��Ӧ��������Ϊ�������ȼ�
	CNIE=0x01; //ʹ�ܱȽ����ж�
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
	if(TailKeyTIM<TailKeyRelTime)TailKeyTIM++;
	else if(TailKeyTIM==TailKeyRelTime)
		{
		TailKeyTIM++;
		if(TailKeyCount>0)IsTailKeyPressed=true;
		}
	}	
	
//β���߼�����	
void TailKey_Handler(void)
	{
  if(IsEnterLowPowerMode)return;
	//ϵͳ�ѻ��ѣ�������ʼ���
	do
		{
		delay_ms(5);
		IsEnterLowPowerMode<<=1;
		if(C0CON1&0x80)IsEnterLowPowerMode++;
		}			
	while(IsEnterLowPowerMode!=0xFF);
	TailKeyCount++;
	TailKeyTIM=0;  //β���������£������¼���λ��ʱ��
	}
