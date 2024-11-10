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

//��������
void CheckNTCStatus(void);
void SleepMgmt(void);
void DisplayErrorTIMHandler(void);
void ThermalCalcProcess(void);
void OutputFaultDetect(void);
void InputFaultDetect(void);

//������
void main()
	{
	//ʱ�ӳ�ʼ��
 	delay_init();	 //��ʱ������ʼ��
	SetSystemHBTimer(1);//����ϵͳ����8Hz��ʱ��	
	//��ʼ������
	CheckIfHBTIMIsReady();//���ϵͳ������ʱ���Ƿ��Ѽ���
	LED_Init(); //��ʼ���ఴLED
	ADC_Init(); //��ʼ��ADC
	CheckNTCStatus(); //���NTC״̬
	ModeFSMInit(); //��ʼ����λ״̬��
  SideKeyInit(); //�ఴ��ʼ��
	PWM_Init(); //����PWM��ʱ��	
	OutputChannel_Init(); //�������ͨ��	
	OutputChannel_TestRun(); //���ͨ��������
	DisplayVBattAtStart(); //��ʾ�����ѹ
	LEDMgmt_SwitchToPWM(); //���ݼ�����ϣ��л���PWM�������ȵĲఴLEDģʽ
	WDog_Init(); //�������Ź�
	EnableADCAsync(); //����ADC���첽ģʽ��ߴ����ٶ�
	//��ѭ��	
  while(1)
		{
	  //ʵʱ����
		SystemTelemHandler();//��ȡ�����Ϣ	
		SideKey_LogicHandler(); //����ఴ����
		BatteryTelemHandler(); //������ң��
		ModeSwitchFSM(); //��λ״̬��
		ThermalCalcProcess(); //�¿�PI��·����͹��ȱ���
		OutputChannel_Calc(); //���ݵ����������ͨ������
		PWM_OutputCtrlHandler(); //����PWM�������	
		//8Hz��ʱ����
		if(!SysHBFlag)continue; //ʱ��û������������
		SysHBFlag=0;
		WDog_Feed(); //ι��
		BattDisplayTIM(); //��ص�����ʾTIM
		LEDControlHandler(); //�ఴָʾLED���ƺ���
		ModeFSMTIMHandler();//��λ״̬������������ʱ������
		HoldSwitchGearCmdHandler(); //��������
		DisplayErrorTIMHandler(); //���ϴ�����ʾ
		MoonConfigHandler(); //�¹����ù���
		OutputFaultDetect();//������ϼ��
		InputFaultDetect(); //������Ӽ��
		SleepMgmt(); //���߹���
		}
	}

//GPIO2�жϻص�������
void Key_IRQHandler(void)  interrupt SideKeyVector 
  {
	//�ఴ�жϴ�������Ӧ�ж�
	if(GPIO_GetIntFlag(SideKeyGPIOG,SideKeyGPIOx))
		{
		SideKey_Int_Callback();  //���а�����Ӧ
		GPIO_ClearIntFlag(SideKeyGPIOG,SideKeyGPIOx); //���Flag
		}
	}