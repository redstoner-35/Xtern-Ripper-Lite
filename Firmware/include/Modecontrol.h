#ifndef _ModeControl_
#define _ModeControl_

#include "stdbool.h"

typedef enum
	{
	MoonLight_10mA,
	MoonLight_25mA,
	MoonLight_50mA,
	MoonLight_100mA,
	MoonLight_200mA,
	MoonLight_UsingModeDef,
	}MoonLightBrightnessDef;	

typedef struct
	{
	int Current;
	char RampMaxDisplayTIM;
	char CfgSavedTIM;
	}RampConfigDef;	

typedef enum
	{
	Fault_None,    //û�д�����
	Fault_DCDCFailedToStart, //DCDC�޷����� ID:1
	Fault_DCDCENOOC, //DCDCʹ���źŲ��ܿ� ID:2
	Fault_DCDCShort, //DCDC�����·  ID:3
	Fault_DCDCOpen,  //LED��· ID:4
	Fault_NTCFailed, //NTC���� ID:5
	Fault_OverHeat, //���ȹ��� ID:6
	Fault_InputConnectFailed, //������ӱ��� ID:7
	Fault_MPUHang, //��Ƭ���������¿��Ź�������λ ID:8
	}FaultCodeDef;	
	
typedef enum
	{
	Mode_OFF, //�ػ�
	Mode_Fault, //���ִ���
		
	Mode_Ramp, //�޼�����
  Mode_Moon, //�¹�
	Mode_Low, //����
	Mode_Mid, //����
	Mode_MHigh,   //�и���
	Mode_High,   //����
		
	Mode_Turbo, //����
  Mode_Strobe, //����		
	Mode_SOS, //SOS��λ
	}ModeIdxDef;

typedef enum
	{
	SOSState_Prepare,
	SOSState_3Dot,
	SOSState_3DotWait,
	SOSState_3Dash,
	SOSState_3DashWait,
	SOSState_3DotAgain,
	SOSState_Wait,
	}SOSStateDef;	
	
typedef struct
	{
  ModeIdxDef ModeIdx;
  int Current; //��λ����(mA)
	int MinCurrent; //��С����(mA)�����޼�������Ҫ
	int LowVoltThres; //�͵�ѹ����ѹ(mV)
	bool IsModeHasMemory; //�Ƿ������
	bool IsNeedStepDown; //�Ƿ���Ҫ����
	}ModeStrDef; 

//�ⲿ����
extern ModeStrDef *CurrentMode; //��ǰģʽ�ṹ��
extern xdata ModeIdxDef LastMode; //��һ����λ	
extern RampConfigDef RampCfg; //�޼���������	
extern bit IsRampEnabled; //�Ƿ������޼�����	
extern xdata FaultCodeDef ErrCode; //�������
extern xdata MoonLightBrightnessDef MoonCfg; //�¹�ģʽ����	
extern xdata float TargetCurrent; //��λ״̬���������Ŀ�����	
	
//��������
#define BatteryAlertDelay 9 //��ؾ����ӳ�	
#define HoldSwitchDelay 6 // ���������ӳ�	
#define SleepTimeOut 5 //����״̬��ʱ	
#define ModeTotalDepth 11 //ϵͳһ���м�����λ			
#define SOSDotTime 2 //SOS�ź�(.)��ʱ��	
#define SOSDashTime 6 //SOS�ź�(-)��ʱ��	
#define SOSGapTime 7 //SOS�ź���ÿ����ʾ;�еȴ���ʱ��
#define SOSFinishGapTime 35 //ÿ��SOS����������ĵȴ�ʱ��
	
//����
void ModeFSMTIMHandler(void);//��λ״̬������������ʱ������
void ModeSwitchFSM();//��λ״̬��	
int SwitchToGear(ModeIdxDef TargetMode);//����ָ����λ
void ReturnToOFFState(void);//�ػ�	
int GetModeCurrentFromIdx(ModeIdxDef TargetMode); //���ص���ֵ	
void HoldSwitchGearCmdHandler(void); //�����������	
void ModeFSMInit(void); //��ʼ��״̬��	
void MoonConfigHandler(void);	//�¹⵲λѭ�����ù���
	
#endif
