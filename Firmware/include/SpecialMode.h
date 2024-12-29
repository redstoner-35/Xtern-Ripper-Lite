#ifndef _SPMode_
#define _SPMode_

#include "ModeControl.h"
#include "LEDMgmt.h"

//�������ö��
typedef enum
	{
	Operation_Normal=0, //��������
	Operation_Locked=1, //����ģʽ
	Operation_TacTurbo=2, //ս��ģʽ(����)
	Operation_TacStrobe=3, //����ģʽ
	}SpecialOperationDef;	

//�ⲿ����
extern SpecialOperationDef SysMode; //���⹦��
extern bit IsDisplayLocked; //��ʾ����
	
//����
void PowerToNormalMode(ModeIdxDef Mode);//��������ͨģʽ
void EnterTurboStrobe(char TKCount,char ClickCount);//���뼫���ͱ������ж�
void LeaveSpecialMode(char TKCount,char ClickCount); //�˳������ͼ���
void SpecialModeOperation(char Click);//���⹦���л��Ĵ���
	
#endif
