#ifndef _TK_
#define _TK_

//�ڲ�����
#include "ModeControl.h"

//β����������
#define TailKeyRelTime 3 //β���ſ���ʱ��

//�ⲿ����
extern char TailKeyTIM;

//����
void TailMemory_Recall(void);
void TailMemory_Save(ModeIdxDef Mode);
void TailKey_POR_Init(void); //�������򿪹ؼ���β����ʼ��
void TailKey_Init(void); //β����ʼ��
void TailKey_Handler(void); //�ఴ��ʼ��
void TailKeyCounter(void); //��ʱ��
char GetTailKeyCount(void); //��ȡβ��״̬

#endif
