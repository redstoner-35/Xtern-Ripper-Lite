#ifndef _ModeControl_
#define _ModeControl_

#include "stdbool.h"

typedef struct
	{
	int Current;
	int BattThres;
	int CurrentLimit;
	char RampMaxDisplayTIM;
	char CfgSavedTIM;
	}RampConfigDef;	
	
typedef enum
	{
	Mode_OFF=0, //�ػ�
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
	char Offset; //����ֵ��ƫ����(%)
	}ModeStrDef; 

//�ⲿ����
extern ModeStrDef *CurrentMode; //��ǰģʽ�ṹ��
extern xdata ModeIdxDef LastMode; //��һ����λ	
extern RampConfigDef RampCfg; //�޼���������	
extern bit IsRampEnabled; //�Ƿ������޼�����	
	
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
void HoldSwitchGearCmdHandler(void); //�����������	
void ModeFSMInit(void); //��ʼ��״̬��	
	
#endif
