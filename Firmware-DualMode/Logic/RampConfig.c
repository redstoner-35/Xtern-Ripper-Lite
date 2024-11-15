#include "ModeControl.h"
#include "cms8s6990.h"
#include "stdbool.h"
#include "RampConfig.h"
#include "LEDMgmt.h"
#include "Flash.h"

//������ROM����Ĳʵ�
code char VendorString[]="XTRLite FW1.1a";

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
	//����flash����ʼ��ȡ
	SetFlashState(1);
	for(i=0;i<sizeof(RampROMImageDef);i++)Flash_Operation(Flash_Read,i,&ROMData.ByteBuf[i]); //��ROM�ڶ�ȡ����
	SetFlashState(0);//��ȡ������ϣ�����flash	
	//���ж������ݵ�У��
	if(ROMData.Data.CheckSum==PEC8Check(ROMData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)))
		{
		//У��ɹ�����������
		MoonCfg=ROMData.Data.RampConfig.Data.MoonCfg;
		RampCfg.Current=ROMData.Data.RampConfig.Data.RampCurrent;
		IsRampEnabled=ROMData.Data.RampConfig.Data.IsRampEnabled?1:0;
		}
	//У��ʧ���ؽ�����
	else 
		{
		MoonCfg=MoonLight_10mA; //����Ĭ��Ϊ10mA�¹�
		RestoreToMinimumRampCurrent();
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
	//����flash����ʼ��ȡ
	SetFlashState(1);
	if(!IsForceSave)for(i=0;i<sizeof(RampROMImageDef);i++)Flash_Operation(Flash_Read,i,&ROMData.ByteBuf[i]); //��ROM�ڶ�ȡ����
  //��ʼ�������ݹ���
	SavedData.Data.RampConfig.Data.MoonCfg=MoonCfg;
  SavedData.Data.RampConfig.Data.RampCurrent=RampCfg.Current;
	SavedData.Data.RampConfig.Data.IsRampEnabled=IsRampEnabled?true:false;
	SavedData.Data.CheckSum=PEC8Check(SavedData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)); //����CRC
	//�������ݱȶ�
	if(!IsForceSave&&SavedData.Data.CheckSum==ROMData.Data.CheckSum)
		{
		SetFlashState(0);//��ȡ������ϣ�����flash	
	  return; //�������������������ͬ	
		}
	//������Ҫ���棬��ʼ��������������
	Flash_Operation(Flash_Erase,0,&ROMData.ByteBuf[0]); //��������0
	for(i=0;i<sizeof(RampROMImageDef);i++)Flash_Operation(Flash_Write,i,&SavedData.ByteBuf[i]);	
	SetFlashState(0);//д�������ϣ�����flash	
	}	