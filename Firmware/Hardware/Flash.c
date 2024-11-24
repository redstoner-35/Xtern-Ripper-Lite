#include "cms8s6990.h"
#include "Flash.h"

//����/����Flash
void SetFlashState(bit IsUnlocked)
	{
	//��ֹ�жϲ�����flash
	if(IsUnlocked)
		{
		EA=0;
		_nop_();
		MLOCK = 0xAA;	
		}
	//���´��жϲ���סFlash
	else
		{
		MLOCK = 0x55;		
		_nop_();
		EA=1; //���������ж�
		}
	}
	
//��ȡ����
void Flash_Operation(FlashOperationDef Operation,int ADDR,char *Data)
	{
	if(Operation==Flash_Write)MDATA=*Data; //д��ģʽ����Ҫд����	
	MADRL = ADDR&0xFF;
	MADRH = (ADDR>>8)&0xFF; //���õ�ַ
	_nop_();	
	MCTRL = (unsigned char)Operation; //�����������ж�ȡ����
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while(MCTRL & 0x01); //�ȴ���ȡ����
	if(Operation==Flash_Read)*Data=MDATA; //��������
	}
