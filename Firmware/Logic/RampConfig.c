#include "ModeControl.h"
#include "cms8s6990.h"
#include "stdbool.h"
#include "RampConfig.h"
#include "LEDMgmt.h"
#include "Flash.h"

//CRC-8计算 
static u8 PEC8Check(char *DIN,long Len)
{
 unsigned char crcbuf=0x00;
 char i;
 do
	{
  //载入数据
  crcbuf^=*DIN++;
  //计算
	i=8;
  for(i=8;i;i--)
   {
	 if(crcbuf&0x80)crcbuf=(crcbuf<<1)^0x07;//最高位为1，左移之后和多项式XOR
	 else crcbuf<<=1;//最高位为0，只移位不XOR
	 }
	}
 while(--Len);
 //输出结果
 return crcbuf;
}

//读取无极调光配置
void ReadRampConfig(void)
	{
	int i;
	extern code ModeStrDef ModeSettings[ModeTotalDepth];
	RampROMImg ROMData;
	//解锁flash并开始读取
	SetFlashState(1);
	for(i=0;i<sizeof(RampROMImageDef);i++)Flash_Operation(Flash_Read,i,&ROMData.ByteBuf[i]); //从ROM内读取数据
	SetFlashState(0);//读取操作完毕，锁定flash	
	//进行读出数据的校验
	if(ROMData.Data.CheckSum==PEC8Check(ROMData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)))
		{
		//校验成功，加载数据
		RampCfg.Current=ROMData.Data.RampConfig.Data.RampCurrent;
		IsRampEnabled=ROMData.Data.RampConfig.Data.IsRampEnabled?1:0;
		}
	//校验失败重建数据
	else 
		{
		RestoreToMinimumRampCurrent();
		IsRampEnabled=0; //默认为挡位模式
		SaveRampConfig(1); //重建数据后立即保存参数
		}
	}

//恢复到无极调光模式的最低电流
void RestoreToMinimumRampCurrent(void)	
	{
	int i;
	extern code ModeStrDef ModeSettings[ModeTotalDepth];
	RampCfg.Current=800;
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)
			RampCfg.Current=ModeSettings[i].MinCurrent; //找到挡位数据中无极调光的挡位
	}

//保存无极调光配置
void SaveRampConfig(bit IsForceSave)
	{
	int i;
	RampROMImg ROMData,SavedData;
	//解锁flash并开始读取
	SetFlashState(1);
	if(!IsForceSave)for(i=0;i<sizeof(RampROMImageDef);i++)Flash_Operation(Flash_Read,i,&ROMData.ByteBuf[i]); //从ROM内读取数据
  //开始进行数据构建
  SavedData.Data.RampConfig.Data.RampCurrent=RampCfg.Current;
	SavedData.Data.RampConfig.Data.IsRampEnabled=IsRampEnabled?true:false;
	SavedData.Data.CheckSum=PEC8Check(SavedData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)); //计算CRC
	//进行数据比对
	if(!IsForceSave&&SavedData.Data.CheckSum==ROMData.Data.CheckSum)
		{
		SetFlashState(0);//读取操作完毕，锁定flash	
	  return; //跳过保存操作，数据相同	
		}
	//数据需要保存，开始擦除并保存数据
	Flash_Operation(Flash_Erase,0,&ROMData.ByteBuf[0]); //擦除扇区0
	for(i=0;i<sizeof(RampROMImageDef);i++)Flash_Operation(Flash_Write,i,&SavedData.ByteBuf[i]);	
	SetFlashState(0);//写入操作完毕，锁定flash	
	}	
