#ifndef _SOS_
#define _SOS_

//SOS״̬ö��
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

//SOSʱ������
#define SOSDotTime 2 //SOS�ź�(.)��ʱ��	
#define SOSDashTime 6 //SOS�ź�(-)��ʱ��	
#define SOSGapTime 7 //SOS�ź���ÿ����ʾ;�еȴ���ʱ��
#define SOSFinishGapTime 35 //ÿ��SOS����������ĵȴ�ʱ��	

//����
char SOSFSM(void);	//SOS״̬������ģ��
void SOSTIMHandler(void);//SOS״̬����ʱ����
void ResetSOSModule(void);	//��λ����SOSģ��
	
#endif