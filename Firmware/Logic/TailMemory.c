#include "ModeControl.h"
#include "Flash.h"
#include "TailKey.h"

//���������ĵ�λ����
code ModeIdxDef NonMemList[]={Mode_Turbo,Mode_Strobe,Mode_SOS};

//��Flash��Ѱ����һ�ιػ�֮ǰ�Ľ��
static int SearchForLastMode(ModeIdxDef *Result)
	{
	int i;
	char buf;
	for(i=512;i<1020;i++)
		{
	  Flash_Operation(Flash_Read,i,&buf);
		if(buf<1||buf>11)return i; //�ҵ��յĵط�
		else *Result=(ModeIdxDef)(buf-1); //��ǰ��δ�ִ����һ��ģʽ		
		}
	//����һȦɶҲû�ҵ�
	*Result=Mode_OFF;
  return i;
	}
//��λ����������ƥ��
static ModeIdxDef ModeMemoryLookup(ModeIdxDef Mode,ModeIdxDef LastMode)	
	{
	char i;
	if(Mode==Mode_Fault)Mode=Mode_OFF; //�ص��ػ�״̬
	else for(i=0;i<sizeof(NonMemList);i++)if(Mode==NonMemList[i])
		{
		if(LastMode!=Mode_OFF)Mode=LastMode;
		else Mode=Mode_Low; //����ϴμ����ǹػ�״̬��������Ϊ������
		}
	//�������
	return Mode;
	}

//�ϵ�ʱ����β�����������recall
void TailMemory_Recall(void)
	{
	ModeIdxDef ModeBuf;
	//��ȡEEPROM�ҵ����µļ�����
	SetFlashState(1);
	SearchForLastMode(&ModeBuf); //Ѱ���ϴν����Ľ��
	SwitchToGear(ModeMemoryLookup(ModeBuf,Mode_OFF)); //�ָ����ϸ���λ
	SetFlashState(0); //�ر��ж�
	}

//����������
void TailMemory_Save(ModeIdxDef Mode)
	{
	int Idx;
	ModeIdxDef LastMode;
	char buf;
	//���б�����ȡ
	SetFlashState(1);
  Idx=SearchForLastMode(&LastMode);
	//�ȶ�����
  ModeMemoryLookup(Mode,LastMode); //��������ƥ��
	if(LastMode==(unsigned char)Mode) //��ǰģʽ����ģ���������ݺ�Ŀ��Ҫд���������ͬ
		{
		SetFlashState(0);
	  return;
		}
	//�洢���Ѿ�д���ˣ�����
	if(Idx==1020)
		{
		Idx=512; //�ص��洢�ṹ��ͷ����ʼд��
		Flash_Operation(Flash_Erase,Idx,&buf);
		}
	//��ʼд������
	buf=((char)Mode)+1;
	Flash_Operation(Flash_Write,Idx,&buf);
	SetFlashState(0); //д�������סFlash
	}
