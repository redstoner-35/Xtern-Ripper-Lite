#include "ModeControl.h"
#include "cms8s6990.h"
#include "stdbool.h"
#include "RampConfig.h"
#include "LEDMgmt.h"

//隐藏在ROM里面的彩蛋
code char VendorString[]="Xtern Ripper Lite V1.0 By:Redstoner_35\x0D\x0A";

//CRC-8计算 
static u8 PEC8Check(char *DIN,long Len)
{
 unsigned char crcbuf=0x00;
 long i,ptr=0;
 while(Len>0)
 {
  //载入数据
  crcbuf=crcbuf^DIN[ptr];
  //计算
  for(i=8;i>0;i--)
   {
	 if(crcbuf&0x80)crcbuf=(crcbuf<<1)^0x07;//最高位为1，左移之后和多项式XOR
	 else crcbuf=crcbuf<<1;//最高位为0，只移位不XOR
	 }
	//计算完一轮，指向下一个数据
	ptr++;
	Len--;
 }
 //和内存里面的Vendor String进行XOR
 for(i=0;i<sizeof(VendorString);i++)crcbuf^=VendorString[i];
 //输出结果
 return crcbuf;
}

//读取无极调光配置
void ReadRampConfig(void)
	{
	int i;
	extern code ModeStrDef ModeSettings[ModeTotalDepth];
	RampROMImg ROMData;
	//禁止中断并解锁flash
	EA=0;
	_nop_();
	MLOCK = 0xAA;
	//开始读取
	for(i=0;i<sizeof(RampROMImageDef);i++)
		{
		MADRL = i;
		MADRH = i>>8; //设置地址
		_nop_();	
		MCTRL = 0x11; //对数据区进行读取操作
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();	 //等待6个周期后开始判断数据内容
		while(MCTRL & 0x01); //等待读取结束
		//读取结束，写入内容
		ROMData.ByteBuf[i]=MDATA;
		}
	//读取操作完毕，锁定flash	
	MLOCK = 0x55;		
	_nop_();
	EA=1; //重新启用中断
	//进行读出数据的校验
	if(ROMData.Data.CheckSum==PEC8Check(ROMData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)))
		{
		//校验成功，加载数据
		LEDBrightNess=ROMData.Data.RampConfig.Data.SideLEDBrightness;
		MoonCfg=ROMData.Data.RampConfig.Data.MoonCfg;
		RampCfg.Current=ROMData.Data.RampConfig.Data.RampCurrent;
		IsRampEnabled=ROMData.Data.RampConfig.Data.IsRampEnabled?1:0;
		}
	//校验失败重建数据
	else 
		{
		MoonCfg=MoonLight_10mA; //出厂默认为10mA月光
		RampCfg.Current=800;
		LEDBrightNess=2399; //使用默认亮度
		for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)
			RampCfg.Current=ModeSettings[i].MinCurrent; //找到挡位数据中无极调光的挡位
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
	//禁止中断并解锁flash
	EA=0;
	_nop_();
	MLOCK = 0xAA;	
	//开始读取
	if(!IsForceSave)for(i=0;i<sizeof(RampROMImageDef);i++)
		{
		MADRL=i&0xFF;
		MADRH=(i>>8)&0xFF; //设置地址
		_nop_();	
		MCTRL = 0x11; //对数据区进行读取操作
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();	 //等待6个周期后开始判断数据内容
		while(MCTRL & 0x01); //等待读取结束
		//读取结束，写入内容
		ROMData.ByteBuf[i]=MDATA;
		}
  //开始进行数据构建
	SavedData.Data.RampConfig.Data.SideLEDBrightness=LEDBrightNess;
	SavedData.Data.RampConfig.Data.MoonCfg=MoonCfg;
  SavedData.Data.RampConfig.Data.RampCurrent=RampCfg.Current;
	SavedData.Data.RampConfig.Data.IsRampEnabled=IsRampEnabled?true:false;
	SavedData.Data.CheckSum=PEC8Check(SavedData.Data.RampConfig.ByteBuf,sizeof(RampStorDef)); //计算CRC
	//进行数据比对
	if(!IsForceSave&&SavedData.Data.CheckSum==ROMData.Data.CheckSum)
		{
		MLOCK = 0x55;		
		_nop_();
		EA=1; //重新启用中断
	  return; //跳过保存操作，数据相同	
		}
	//数据需要保存，开始擦除
	MADRL=0;
  MADRH=0;
 	_nop_();
	MCTRL  = 0x1D;		//对数据区进行擦除
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();	
	while(MCTRL & 0x01)if(MCTRL&0x20)return;			//等待擦除结束
	//开始保存数据
	for(i=0;i<sizeof(RampROMImageDef);i++)	
		{
		MDATA=SavedData.ByteBuf[i];
		//设置地址
		MADRL=i&0xFF;
		MADRH=(i>>8)&0xFF;
		_nop_();
		MCTRL  = 0x19;		//对数据区进行写入
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();	
		while(MCTRL & 0x01)if(MCTRL&0x20)return;			//等待写入结束
		}
	//操作完毕，重新使能中断
	MLOCK = 0x55;		
	_nop_();
	EA=1; //重新启用中断
	}	
