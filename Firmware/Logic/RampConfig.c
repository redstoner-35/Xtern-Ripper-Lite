#include "ModeControl.h"
#include "cms8s6990.h"
#include "stdbool.h"
#include "RampConfig.h"
#include "LEDMgmt.h"

//������ROM����Ĳʵ�
code char VendorString[]="Xtern Ripper Lite V1.0 By:Redstoner_35\x0D\x0A";

//CRC-8���� 
static u8 PEC8Check(char *DIN,long Len)
{
 unsigned char crcbuf=0x00;
 long i,ptr=0;
 while(Len>0)
 {
  //��������
  crcbuf=crcbuf^DIN[ptr];
  //����
  for(i=8;i>0;i--)
   {
	 if(crcbuf&0x80)crcbuf=(crcbuf<<1)^0x07;//���λΪ1������֮��Ͷ���ʽXOR
	 else crcbuf=crcbuf<<1;//���λΪ0��ֻ��λ��XOR
	 }
	//������һ�֣�ָ����һ������
	ptr++;
	Len--;
 }
 //���ڴ������Vendor String����XOR
 for(i=0;i<sizeof(VendorString);i++)crcbuf^=VendorString[i];
 //������
 return crcbuf;
}

//��ȡ�޼���������
void ReadRampConfig(void)
	{
	int i;
	extern code ModeStrDef ModeSettings[ModeTotalDepth];
	RampROMImg ROMData;
	//��ֹ�жϲ�����flash
	EA=0;
	_nop_();
	MLOCK = 0xAA;
	//��ʼ��ȡ
	for(i=0;i<sizeof(RampROMImageDef);i++)
		{
		MADRL = i;
		MADRH = i>>8; //���õ�ַ
		_nop_();	
		MCTRL = 0x11; //�����������ж�ȡ����
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();	 //�ȴ�6�����ں�ʼ�ж���������
		while(MCTRL & 0x01); //�ȴ���ȡ����
		//��ȡ������д������
		ROMData.ByteBuf[i]=MDATA;
		}
	//��ȡ������ϣ�����flash	
	MLOCK = 0x55;		
	_nop_();
	EA=1; //���������ж�
	//���ж������ݵ�У��
	if(ROMData.Data.CheckSum==PEC8Check(ROMData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)))
		{
		//У��ɹ�����������
		LEDBrightNess=ROMData.Data.RampConfig.Data.SideLEDBrightness;
		MoonCfg=ROMData.Data.RampConfig.Data.MoonCfg;
		RampCfg.Current=ROMData.Data.RampConfig.Data.RampCurrent;
		IsRampEnabled=ROMData.Data.RampConfig.Data.IsRampEnabled?1:0;
		}
	//У��ʧ���ؽ�����
	else 
		{
		MoonCfg=MoonLight_10mA; //����Ĭ��Ϊ10mA�¹�
		RampCfg.Current=800;
		LEDBrightNess=2399; //ʹ��Ĭ������
		for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)
			RampCfg.Current=ModeSettings[i].MinCurrent; //�ҵ���λ�������޼�����ĵ�λ
		IsRampEnabled=0; //Ĭ��Ϊ��λģʽ
		SaveRampConfig(1); //�ؽ����ݺ������������
		}
	}

//�ָ����޼�����ģʽ����͵���
void RestoreToMinimumRampCurrent(void)	
	{
	int i;
	extern code ModeStrDef ModeSettings[ModeTotalDepth];
	RampCfg.Current=800;
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)
			RampCfg.Current=ModeSettings[i].MinCurrent; //�ҵ���λ�������޼�����ĵ�λ
	}

//�����޼���������
void SaveRampConfig(bit IsForceSave)
	{
	int i;
	RampROMImg ROMData,SavedData;
	//��ֹ�жϲ�����flash
	EA=0;
	_nop_();
	MLOCK = 0xAA;	
	//��ʼ��ȡ
	if(!IsForceSave)for(i=0;i<sizeof(RampROMImageDef);i++)
		{
		MADRL=i&0xFF;
		MADRH=(i>>8)&0xFF; //���õ�ַ
		_nop_();	
		MCTRL = 0x11; //�����������ж�ȡ����
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();	 //�ȴ�6�����ں�ʼ�ж���������
		while(MCTRL & 0x01); //�ȴ���ȡ����
		//��ȡ������д������
		ROMData.ByteBuf[i]=MDATA;
		}
  //��ʼ�������ݹ���
	SavedData.Data.RampConfig.Data.SideLEDBrightness=LEDBrightNess;
	SavedData.Data.RampConfig.Data.MoonCfg=MoonCfg;
  SavedData.Data.RampConfig.Data.RampCurrent=RampCfg.Current;
	SavedData.Data.RampConfig.Data.IsRampEnabled=IsRampEnabled?true:false;
	SavedData.Data.CheckSum=PEC8Check(SavedData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)); //����CRC
	//�������ݱȶ�
	if(!IsForceSave&&SavedData.Data.CheckSum==ROMData.Data.CheckSum)
		{
		MLOCK = 0x55;		
		_nop_();
		EA=1; //���������ж�
	  return; //�������������������ͬ	
		}
	//������Ҫ���棬��ʼ����
	MADRL=0;
  MADRH=0;
 	_nop_();
	MCTRL  = 0x1D;		//�����������в���
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();	
	while(MCTRL & 0x01)if(MCTRL&0x20)return;			//�ȴ���������
	//��ʼ��������
	for(i=0;i<sizeof(RampROMImageDef);i++)	
		{
		MDATA=SavedData.ByteBuf[i];
		//���õ�ַ
		MADRL=i&0xFF;
		MADRH=(i>>8)&0xFF;
		_nop_();
		MCTRL  = 0x19;		//������������д��
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();	
		while(MCTRL & 0x01)if(MCTRL&0x20)return;			//�ȴ�д�����
		}
	//������ϣ�����ʹ���ж�
	MLOCK = 0x55;		
	_nop_();
	EA=1; //���������ж�
	}	
