#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "SideKey.h"
#include "LEDMgmt.h"
#include "ADCCfg.h"
#include "PinDefs.h"
#include "OutputChannel.h"
#include "PWMCfg.h"
#include "BattDisplay.h"
#include "ModeControl.h"
#include "Watchdog.h"
#include "TailKey.h"
#include "Strap.h"

//��������
void CheckNTCStatus(void);
void SleepMgmt(void);
void DisplayErrorTIMHandler(void);
void ThermalCalcProcess(void);
void OutputFaultDetect(void);

//������
void main()
	{
	//ʱ�ӳ�ʼ��
 	delay_init();	 //��ʱ������ʼ��
	SetSystemHBTimer(1);//����ϵͳ����8Hz��ʱ��	
	//��ʼ������
	LED_Init(); //��ʼ���ఴLED
	ADC_Init(); //��ʼ��ADC
	CheckNTCStatus(); //���NTC״̬
	Strap_Init(); //��ȡ���������õ���
	ModeFSMInit(); //��ʼ����λ״̬��
  SideKeyInit(); //�ఴ��ʼ��
	TailKey_Init(); //β����ʼ��
	PWM_Init(); //����PWM��ʱ��	
	OutputChannel_Init(); //�������ͨ��	
	TailMemory_Recall(); //��ȡβ���ϴιػ�ǰ�ĵ�λ
	OutputChannel_TestRun(); //���ͨ��������
	DisplayVBattAtStart(); //��ʾ�����ѹ
	WDog_Init(); //�������Ź�
	EnableADCAsync(); //����ADC���첽ģʽ��ߴ����ٶ�
	//��ѭ��	
  while(1)
		{
	  //ʵʱ����
		SystemTelemHandler();//��ȡ�����Ϣ	
		SideKey_LogicHandler(); //����ఴ����
		TailKey_Handler(); //����β������
		BatteryTelemHandler(); //������ң��
		ModeSwitchFSM(); //��λ״̬��
		ThermalCalcProcess(); //�¿�PI��·����͹��ȱ���
		OutputChannel_Calc(); //���ݵ����������ͨ������
		PWM_OutputCtrlHandler(); //����PWM�������	
		//8Hz��ʱ����
		if(!SysHBFlag)continue; //ʱ��û������������
		SysHBFlag=0;
		WDog_Feed(); //ι��
		TailKeyCounter(); //��ʱ��
		BattDisplayTIM(); //��ص�����ʾTIM
		ModeFSMTIMHandler(); //ģʽ״̬��
		HoldSwitchGearCmdHandler(); //��������
		DisplayErrorTIMHandler(); //���ϴ�����ʾ
		SleepMgmt(); //���߹���
		if(TailKeyTIM>TailKeyRelTime)LEDControlHandler();//�ఴָʾLED���ƺ���
		OutputFaultDetect();//������ϼ��		
		}
	}

//GPIO2�жϻص�������
void Key_IRQHandler(void)  interrupt P2EI_VECTOR 
  {
	//�ఴ�жϴ�������Ӧ�ж�
	SideKey_Int_Callback();  //���а�����Ӧ
  P2EXTIF=0;
	}