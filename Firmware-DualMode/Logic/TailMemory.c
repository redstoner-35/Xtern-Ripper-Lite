#include "ModeControl.h"
#include "Flash.h"
#include "TailKey.h"

//上电时进行尾部按键记忆的recall
void TailMemory_Recall(void)
	{
	int i;
	char buf;
	ModeIdxDef ModeBuf=Mode_OFF;
	//读取EEPROM找到最新的记忆结果
	SetFlashState(1);
	for(i=512;i<1020;i++)
		{
	  Flash_Operation(Flash_Read,i,&buf);
		if(buf<1||buf>11)break; //找到空的地方
		else ModeBuf=(ModeIdxDef)(buf-1); //当前还未抵达最后一个模式		
		}
	if(ModeBuf==Mode_Turbo||ModeBuf==Mode_Strobe||ModeBuf==Mode_SOS)ModeBuf=Mode_Low; //找了老半天都没找到合适的
	SwitchToGear(ModeBuf); //恢复到上个挡位
	SetFlashState(0); //关闭中断
	}

//数据区保存
void TailMemory_Save(ModeIdxDef Mode)
	{
	int i;
	char LastMode,buf;
	//判断传入的模式值是否有不允许记忆的	
	if(Mode==Mode_Fault)return;
	//进行遍历读取
	SetFlashState(1);
	for(i=512;i<1020;i++)
		{
	  Flash_Operation(Flash_Read,i,&buf);
		if(buf<1||buf>11)break; //找到空的地方
		else LastMode=buf-1; //当前还未抵达最后一个模式		
		}
	//比对数据
	if(Mode==Mode_Turbo||Mode==Mode_Strobe||Mode==Mode_SOS||Mode==Mode_Moon)//极亮爆闪和SOS不记忆。使用进入之前的挡位
		{
		if(LastMode!=Mode_OFF)Mode=LastMode;
		else Mode=Mode_Low; //如果上次记忆是关机状态，则配置为低亮档
		}			
	if(LastMode==(unsigned char)Mode) //当前模式记忆模块里面数据和目标要写入的数据相同
		{
		SetFlashState(0);
	  return;
		}
	//存储区已经写满了，擦除
	if(i==1020)
		{
		i=512; //回到存储结构的头部开始写入
		Flash_Operation(Flash_Erase,i,&buf);
		}
	//开始写入数据
	buf=((char)Mode)+1;
	Flash_Operation(Flash_Write,i,&buf);
	SetFlashState(0); //写入完毕锁住Flash
	}
