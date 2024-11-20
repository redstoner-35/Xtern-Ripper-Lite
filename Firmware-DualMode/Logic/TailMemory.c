#include "ModeControl.h"
#include "Flash.h"
#include "TailKey.h"

//�ϵ�ʱ����β�����������recall
void TailMemory_Recall(void)
	{
	int i;
	char buf;
	ModeIdxDef ModeBuf=Mode_OFF;
	//��ȡEEPROM�ҵ����µļ�����
	SetFlashState(1);
	for(i=512;i<1020;i++)
		{
	  Flash_Operation(Flash_Read,i,&buf);
		if(buf<1||buf>11)break; //�ҵ��յĵط�
		else ModeBuf=(ModeIdxDef)(buf-1); //��ǰ��δ�ִ����һ��ģʽ		
		}
	if(ModeBuf==Mode_Turbo||ModeBuf==Mode_Strobe||ModeBuf==Mode_SOS)ModeBuf=Mode_Low; //�����ϰ��춼û�ҵ����ʵ�
	SwitchToGear(ModeBuf); //�ָ����ϸ���λ
	SetFlashState(0); //�ر��ж�
	}

//����������
void TailMemory_Save(ModeIdxDef Mode)
	{
	int i;
	char LastMode,buf;
	//�жϴ����ģʽֵ�Ƿ��в���������	
	if(Mode==Mode_Fault)return;
	//���б�����ȡ
	SetFlashState(1);
	for(i=512;i<1020;i++)
		{
	  Flash_Operation(Flash_Read,i,&buf);
		if(buf<1||buf>11)break; //�ҵ��յĵط�
		else LastMode=buf-1; //��ǰ��δ�ִ����һ��ģʽ		
		}
	//�ȶ�����
	if(Mode==Mode_Turbo||Mode==Mode_Strobe||Mode==Mode_SOS||Mode==Mode_Moon)//����������SOS�����䡣ʹ�ý���֮ǰ�ĵ�λ
		{
		if(LastMode!=Mode_OFF)Mode=LastMode;
		else Mode=Mode_Low; //����ϴμ����ǹػ�״̬��������Ϊ������
		}			
	if(LastMode==(unsigned char)Mode) //��ǰģʽ����ģ���������ݺ�Ŀ��Ҫд���������ͬ
		{
		SetFlashState(0);
	  return;
		}
	//�洢���Ѿ�д���ˣ�����
	if(i==1020)
		{
		i=512; //�ص��洢�ṹ��ͷ����ʼд��
		Flash_Operation(Flash_Erase,i,&buf);
		}
	//��ʼд������
	buf=((char)Mode)+1;
	Flash_Operation(Flash_Write,i,&buf);
	SetFlashState(0); //д�������סFlash
	}
