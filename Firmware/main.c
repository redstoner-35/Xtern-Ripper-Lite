#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "SideKey.h"
#include "LEDMgmt.h"
#include "ADCCfg.h"
#include "OutputChannel.h"
#include "PWMCfg.h"
#include "BattDisplay.h"
#include "ModeControl.h"
#include "TempControl.h"
#include "TailKey.h"
#include "Strap.h"
#include "SOS.h"
#include "SelfTest.h"

//��������
void SleepMgmt(void);

//������
void main()
	{
	//ʱ�ӳ�ʼ��
 	delay_init();	 //��ʱ������ʼ��
	SetSystemHBTimer(1);//����ϵͳ����8Hz��ʱ��	
	//��ʼ������
	OutputChannel_Init(); //�������ͨ��	
	ADC_Init(); //��ʼ��ADC
	Strap_Init(); //��ȡ���������õ���
	TailKey_POR_Init(); //β����ʼ�������򿪹ؼ��
	PWM_Init(); //����PWM��ʱ��
	LED_Init(); //��ʼ���ఴLED
	ModeFSMInit(); //��ʼ����λ״̬��
  SideKeyInit(); //�ఴ��ʼ��	
	TailMemory_Recall(); //��ȡβ���ϴιػ�ǰ�ĵ�λ
	OutputChannel_TestRun(); //���ͨ��������
	DisplayVBattAtStart(); //��ʾ�����ѹ
	EnableADCAsync(); //����ADC���첽ģʽ��ߴ����ٶ�
	IRQ_ALL_ENABLE(); //�������ж�
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
		ThermalItgCalc(); //����������
		SideKey_TIM_Callback();//�ఴ�����ļ�ⶨʱ������
		TailKeyCounter(); //��ʱ��
		BattDisplayTIM(); //��ص�����ʾTIM
		SOSTIMHandler(); //SOS��ʱ��
		ModeFSMTIMHandler(); //ģʽ״̬��
		HoldSwitchGearCmdHandler(); //��������
		DisplayErrorTIMHandler(); //���ϴ�����ʾ
		SleepMgmt(); //���߹���
		if(TailKeyTIM<TailKeyRelTime)continue;
		LEDControlHandler();//�ఴָʾLED���ƺ���
		OutputFaultDetect();//������ϼ��		
		}
	}
