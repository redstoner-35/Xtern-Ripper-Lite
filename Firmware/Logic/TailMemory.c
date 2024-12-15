#include "ModeControl.h"
#include "Flash.h"
#include "TailKey.h"

//不允许记忆的挡位名单
code ModeIdxDef NonMemList[]={Mode_Turbo,Mode_Strobe,Mode_SOS};

//在Flash内寻找上一次关机之前的结果
static int SearchForLastMode(ModeIdxDef *Result)
	{
	int i;
	char buf;
	for(i=512;i<1020;i++)
		{
	  Flash_Operation(Flash_Read,i,&buf);
		if(buf<1||buf>11)return i; //找到空的地方
		else *Result=(ModeIdxDef)(buf-1); //当前还未抵达最后一个模式		
		}
	//找了一圈啥也没找到
	*Result=Mode_OFF;
  return i;
	}
//挡位记忆名单的匹配
static ModeIdxDef ModeMemoryLookup(ModeIdxDef Mode,ModeIdxDef LastMode)	
	{
	char i;
	if(Mode==Mode_Fault)Mode=Mode_OFF; //回到关机状态
	else for(i=0;i<sizeof(NonMemList);i++)if(Mode==NonMemList[i])
		{
		if(LastMode!=Mode_OFF)Mode=LastMode;
		else Mode=Mode_Low; //如果上次记忆是关机状态，则配置为低亮档
		}
	//计算完毕
	return Mode;
	}

//上电时进行尾部按键记忆的recall
void TailMemory_Recall(void)
	{
	ModeIdxDef ModeBuf;
	//读取EEPROM找到最新的记忆结果
	SetFlashState(1);
	SearchForLastMode(&ModeBuf); //寻找上次结束的结果
	SwitchToGear(ModeMemoryLookup(ModeBuf,Mode_OFF)); //恢复到上个挡位
	SetFlashState(0); //关闭中断
	}

//数据区保存
void TailMemory_Save(ModeIdxDef Mode)
	{
	int Idx;
	ModeIdxDef LastMode;
	char buf;
	//进行遍历读取
	SetFlashState(1);
  Idx=SearchForLastMode(&LastMode);
	//比对数据
  ModeMemoryLookup(Mode,LastMode); //进行名单匹配
	if(LastMode==(unsigned char)Mode) //当前模式记忆模块里面数据和目标要写入的数据相同
		{
		SetFlashState(0);
	  return;
		}
	//存储区已经写满了，擦除
	if(Idx==1020)
		{
		Idx=512; //回到存储结构的头部开始写入
		Flash_Operation(Flash_Erase,Idx,&buf);
		}
	//开始写入数据
	buf=((char)Mode)+1;
	Flash_Operation(Flash_Write,Idx,&buf);
	SetFlashState(0); //写入完毕锁住Flash
	}
