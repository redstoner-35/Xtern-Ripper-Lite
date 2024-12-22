#include "ModeControl.h"
#include "SOS.h"

//״̬λ
static xdata SOSStateDef SOSState; //ȫ�ֱ���״̬λ
static xdata char SOSTIM;  //SOS��ʱ

//SOS״̬������ת����
static void SOSFSM_Jump(SOSStateDef State,char Time)
{
	if(SOSTIM)return; //��ʾδ����
	SOSTIM=Time; 
	SOSState=State;  //������ʱ�ȴ��׶�
}

//SOS״̬����ʱ����
void SOSTIMHandler(void)
{
	//�Լ�ʱ����ֵ���еݼ�
	if(SOSTIM>0)SOSTIM--;
}

//��λ����SOSģ��
void ResetSOSModule(void)
{
	SOSState=SOSState_Prepare;
	SOSTIM=0;
}
//SOS��ʱ��״̬���
static bit SOSTIMDetect(char Time)
{
//������ʱ�ж�
if((SOSTIM%(Time*2))>(Time-1))return 1;
//�ر�״̬����0
return 0;
}

//SOS״̬������ģ��
int SOSFSM(void)
{
	switch(SOSState)
		{
		//׼���׶�
		case SOSState_Prepare:
			 SOSTIM=0;
			 SOSFSM_Jump(SOSState_3Dot,(3*SOSDotTime*2)-1);
		   break;
		//��һ�͵ڶ�������
		case SOSState_3DotAgain:
		case SOSState_3Dot:
       if(SOSTIMDetect(SOSDotTime))return QueryCurrentGearILED(); //��ǰ״̬��ҪLED����������Ŀ�����ֵ		
			 if(SOSState==SOSState_3Dot)SOSFSM_Jump(SOSState_3DotWait,SOSGapTime);  //������ʱ�ȴ��׶�
		   else SOSFSM_Jump(SOSState_Wait,SOSFinishGapTime);//������ʱ�ȴ��׶�
		   break;
		//���������ĵȴ���ʱ�׶�
	  case SOSState_3DotWait:
			 SOSFSM_Jump(SOSState_3Dash,(3*SOSDashTime*2)-1);
		   break;
		//����
		case SOSState_3Dash:
			 if(SOSTIMDetect(SOSDashTime))return QueryCurrentGearILED(); //��ǰ״̬��ҪLED����������Ŀ�����ֵ	
		   SOSFSM_Jump(SOSState_3DashWait,SOSGapTime);
		   break;			
		//����������ĵȴ���ʱ�׶�
	  case SOSState_3DashWait:
			 SOSFSM_Jump(SOSState_3DotAgain,(3*SOSDotTime*2)-1);
		   break;		
	  //�����źŷ�����ϣ��ȴ�
	  case SOSState_Wait:	
			 SOSFSM_Jump(SOSState_Prepare,0);//�ص�׼��״̬
		   break;
		}
	//�����������0�رշ����ӣ�ȷ��β��������ȷ��Ӧ
	return 0;
}