#include "ModeControl.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "TempControl.h"
#include "SpecialMode.h"

//ȫ�ֱ������ⲿ����
extern xdata char DisplayLockedTIM;
static xdata char ShowTacModeTIM;
bit IsDisplayLocked;
SpecialOperationDef SysMode; //ϵͳģʽ

//�����˳������л�
static void EnterExitLock(void)
	{
	DisplayLockedTIM=8; //ָʾ����״̬�л�
	SysMode=!SysMode?Operation_Locked:Operation_Normal;
	}
	
//�����˳�ս���л�
static void EnterExitTac(void)
	{
	DisplayLockedTIM=2; //ָʾս���л�
	SysMode=!SysMode?Operation_TacTurbo:Operation_Normal;
	}	

//��������ͨģʽ
void PowerToNormalMode(ModeIdxDef Mode)
	{
	if(Battery>2.9)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode); //��������
	else if(Battery>2.7)SwitchToGear(Mode_Moon);	 //����2.7V��ʱ��ֻ�ܿ��¹�
	else LEDMode=LED_RedBlinkFifth; //��ص������ز��㣬��ɫ�����
	}
	
//���뼫���ͱ������ж�
void EnterTurboStrobe(char TKCount,char ClickCount)	
	{
	char Count=TKCount>ClickCount?TKCount:ClickCount;
	//˫������
	if(Count==2)
		{
		if(Battery>3.1)SwitchToGear(Mode_Turbo); //��ص���������������
		else PowerToNormalMode(LastMode);  //��ص�ص�������ʱ˫��������ͨģʽ
		}
	//��������
	if(Count==3)
		{
		if(Battery>2.7)SwitchToGear(Mode_Strobe);   //���뱬��
		else LEDMode=LED_RedBlinkFifth; //�������������˸��ʾ
		}
	}
	
//����ģʽ�»ص����⹦��������л�
void LeaveSpecialMode(char TKCount,char ClickCount)	
	{
	char Count=TKCount>ClickCount?TKCount:ClickCount;
	if(Count==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
	if(Count==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
	}	

//��ʾս��ģʽ����
bit DisplayTacModeEnabled(void)
	{
	//��ʱ������
	if(CurrentMode->ModeIdx!=Mode_OFF||SysMode<Operation_TacTurbo)ShowTacModeTIM=0;
	else //�����ۼ�
		{
		ShowTacModeTIM++; //��������
		if(ShowTacModeTIM==14&&SysMode==Operation_TacStrobe)return 1; //ս������ģʽ���Ƶ��2��
		else if(ShowTacModeTIM==16)
			{
			ShowTacModeTIM=0;
			return 1; //����1����ʾ
			}		
		}
	//����״̬����0
	return 0;
	}	
	
//���⹦���л�	
void SpecialModeOperation(char Click)
	{
		//��λflag
	  IsDisplayLocked=0;
		//�������ģʽ�л�
			switch(SysMode)
				{
				//��ͨģʽ
				case Operation_Normal:
					if(Click==5)EnterExitLock(); //��������ģʽ
				  if(Click==6)EnterExitTac(); //����ս��ģʽ
					break;
				//����ģʽ
				case Operation_Locked:
				   if(Click==5)EnterExitLock();
				   else if(getSideKeyHoldEvent())IsDisplayLocked=1;
				   else if(IsKeyEventOccurred())LEDMode=LED_RedBlinkFifth; //ָʾ�ֵ��ѱ�����
				   break;
				//ս��ģʽ
				case Operation_TacTurbo:
				case Operation_TacStrobe:
				  if(Click==6)EnterExitTac();
					if(Click==2) //�л�ģʽ
						{
						if(SysMode==Operation_TacTurbo)
							{
							SysMode=Operation_TacStrobe;
							LEDMode=LED_GreenBlinkThird; //��������ս��
							}
						else
							{
							SysMode=Operation_TacTurbo;
							LEDMode=LED_RedBlinkThird;  //�رձ���ս��
							}
						}
					if(getSideKeyHoldEvent())EnterTurboStrobe(SysMode==Operation_TacStrobe?3:2,0); //���ý��뺯�����Խ�����
			  break;
				}
	}	
