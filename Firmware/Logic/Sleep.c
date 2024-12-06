#include "cms8s6990.h"
#include "delay.h"
#include "SideKey.h"
#include "PWMCfg.h"
#include "PinDefs.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "BattDisplay.h"
#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "TailKey.h"

//�ⲿ����
extern volatile int SleepTimer;
void CheckNTCStatus(void);
void BatteryTelemHandler(void);
bit IsWakupFromSleep=0; //��˯�߽׶λ���

//����/��������ϵͳ����
void SystemPeripheralCTRL(bit IsEnable)
	{
	if(IsEnable)
		{
		LED_Init(); //��ʼ���ఴLED
		ADC_Init(); //��ʼ��ADC
		PWM_Init(); //��ʼ��PWM������
		OutputChannel_Init(); //��ʼ�����ͨ��
		VshowFSMState=BattVdis_Waiting; //��λΪ����״̬
		TailKey_Init(); //�򿪱Ƚ���
		return;
		}
	//�ر���������
	SetSystemHBTimer(0); //����������ʱ��
	PWM_DeInit();
	ADC_DeInit(); //�ر�PWM��ADC
	OutputChannel_DeInit(); //�ر��������
	}

//˯�߹�����
void SleepMgmt(void)
	{
	int i;
	//�ǹػ�����Ȼ����ʾ��ص�ѹ��ʱ��ʱ����λ��ֹ˯��
	if(VshowFSMState!=BattVdis_Waiting||CurrentMode->ModeIdx!=Mode_OFF) 
		{
		SleepTimer=8*SleepTimeOut;		
		return;
		}
	//����ʱ
	if(SleepTimer>0)
		{
		SleepTimer--;
		return; //ʱ��δ����������ʱ
		}
	//��������˯�߽׶�
	C0CON0=0; //�ఴ�ػ���رձȽ���
	SystemPeripheralCTRL(0);//�ر���������
	STOP();  //��STOP=1��ʹ��Ƭ������˯��
	//ϵͳ�ѻ��ѣ�������ʼ���
	delay_init();	 //��ʱ������ʼ��
	SetSystemHBTimer(1); 
	MarkAsKeyPressed(); //������ǰ�������
	do	
		{
		delay_ms(1);
		SideKey_LogicHandler(); //����ఴ����
		}
	while(!IsKeyEventOccurred()); //�ȴ���������
	//ϵͳ�ѱ����ѣ��������빤��ģʽ			
	SystemPeripheralCTRL(1);
	//����ADC��飬�����ص�ѹ���ͣ��������ٶȽ���ֹͣģʽ
	for(i=0;i<3;i++)
			{
			SystemTelemHandler(); //��ȡ��ѹ
			BatteryTelemHandler(); //��ؾ���
			if(!IsBatteryFault)break;
			}
	if(i==3)SleepTimer=15;//��ص�ѹ���ͽ�����ʾ�������ر�
	//���г�ʼ����ϣ�����ADC�첽����ģʽ
	EnableADCAsync(); 
	}
