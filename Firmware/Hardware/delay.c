#include "cms8s6990.h"
#include "delay.h"
#include "PinDefs.h"
#include "SideKey.h"
#include "GPIO.h"

volatile bit SysHBFlag=0; //ϵͳ����Flag
volatile bit IsT0OVF; //T0�����
volatile bit StrobeFlag=0; //����Flag
volatile char HBcounter; //������ʱ������

//8Hz��ʱ����ʼ��
void SetSystemHBTimer(bit IsEnable)
	{
	if(!IsEnable)	
		{
		T2CON=0x00; //����T2��ʱ��
		IE&=~0x20; //����T2�ж�
		return;
		}
  //���ö�ʱ��ģʽ			
  CCEN=0x00; //�رձȽϺͲ���
	RLDH=0x0B;
	RLDL=0xDB; //����װ��ֵ����Ϊ����31.25mS�ӳ�(1/32��)�����㹫ʽΪ65535-(48/24(0.5uS)=2000*31.25mS)=3035[0x0BDB]
  TH2=0x5D;
  TL2=0x66; //������������Ϊ����31.25mS�ӳٵĳ�ֵ
	//�����ж�
  IE|=0x20;   //��ET2=1������T2�ж�
	T2IF=0x00; //����T2�ж�
	T2IE=0x80; //��T2OVIE=1������T2 OVF�ж�
	//������ʱ��
	HBcounter=4; //�Է�Ƶ���������г�ʼ��
	T2CON=0x91; //����T2ʱ��ԴΪfSys/24=1MHz����ʱ����������
	}

#ifdef EnableHBCheck
//���������ʱ���Ƿ����
void CheckIfHBTIMIsReady(void)
	{
	int retry=255;
	SysHBFlag=0;
	do
		{
	  delay_ms(1);
		if(SysHBFlag)return; //��ʱ�����������˳�
		retry--;
		}
	while(retry);
	//��ʱ���ȴ���ʱ��������ɫLED
	while(1); 	
	}
#endif
	
//ϵͳ������ʱ�����жϴ���	
void Timer2_IRQHandler(void) interrupt TMR2_VECTOR
{ 
	T2IF=0x00; //����T2�ж�
  //�����ķ�Ƶ
	HBcounter--;
  StrobeFlag=~StrobeFlag;
	if(HBcounter)return; //ʱ��δ��ֱ���˳�
	//ʱ�䵽����λ��Ƶ��������Flag
	HBcounter=4;
	SysHBFlag=1; //����flag
}		
	
//�����ʱ��ʱ�����жϴ���
void Timer0_IRQHandler(void) interrupt TMR0_VECTOR  //0x0B 
{
  TCON&=0xEF; //���������λ
	IsT0OVF=1;
} 	
	
//��ʱ��ʼ��
void delay_init()
	{
	TCON&=0xCF; //���������λ���رն�ʱ��
	TMOD&=0xF0;
	TMOD|=0x01; //T0����Ϊʹ��Fext,16bit���ϼ���ģʽ
	TH0=0x00;
	TL0=0x00; //��ʼ����ֵ
	IE=0x82; //��ET0=1�����ö�ʱ�ж�,EA=1������ȫ�����ж�
	}
#ifdef EnableMicroSecDelay
//uS�ӳ�
void delay_us(int us)
	{
	bit IsEA=EA;
	us<<=2; //������λ,��uS*4�õ�������ֵ
	us=0xFFFF-us; //�õ�������ֵ
	//װ�ض�ʱ��ֵ
	TH0=(us>>8)&0xFF;
	TL0=us&0xFF; 
	IE&=0x7D; //��ET0,EA=0���رն�ʱ�жϺ�ȫ�����жϿ���
	//������ʱ����ʼ����ʱ
	TCON|=0x10; //TR0=1,��ʱ����ʼ��ʱ	
	while(!(TCON&0x20)); //�ȴ�ֱ��T0���
	//��ʱ��������λ���б�־λ�����´��ж�
	TCON&=0xCF; //���������λ���رն�ʱ��
  if(IsEA)IE|=0x82;
	else IE|=0x02;
	}
#endif
//1ms��ʱ
void delay_ms(int ms)
	{
	unsigned long CNT;
	int repcounter=0;
	//���㶨ʱ����װֵ
	if(ms==0)return;
  do
	  {
		repcounter++; //�ظ�������+1
		CNT=(long)ms*4000; //T0һ��������48/12=4MHz=0.25uS
		CNT/=(long)repcounter; //�����ظ������õ����μ���ֵ
		}
  while(CNT>=65535); //����ѭ��ȷ����ʱ��ֵС��65535
	CNT=0xFFFF-CNT; //�����������16bit����������ֵ���ص���ʱ����
	//��ʼ���е��λ��ε���ʱ
	do
		{			
		//װ�ض�ʱ��ֵ
		TH0=(CNT>>8)&0xFF;
	  TL0=CNT&0xFF; 
		IsT0OVF=0; //��λ��־λ
		//������ʱ����ʼ����ʱ
		TCON|=0x10; //TR0=1,��ʱ����ʼ��ʱ	
		while(!(TCON&0x20)&&!IsT0OVF); //�ȴ�ֱ��T0���
		//��ʱ������׼��������һ��
		TCON&=0xCF; //���������λ���رն�ʱ��
		repcounter--; //�ظ�����-1
		}
	while(repcounter);
	}
