#include "LEDMgmt.h"
#include "delay.h"
#include "ADCCfg.h"
#include "BattDisplay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "TailKey.h"
#include "Strap.h"

//内部变量
static xdata int ErrDisplayIndex;
static xdata char ShortDetectTIM;
xdata char InputDetectTIM;

//报告错误
void ReportError(FaultCodeDef Code)
	{
	ErrCode=Code;
	if(CurrentMode->ModeIdx==Mode_Fault)return;
	TailMemory_Save(Mode_Fault); //故障发生后重置挡位记忆至关机模式
	SwitchToGear(Mode_Fault);  //指示故障发生
	}

//错误ID显示计时函数	
void DisplayErrorTIMHandler(void)	
	{
	//没有错误发生，复位计时器
	if(ErrCode==Fault_None)ErrDisplayIndex=0;
	else //发生错误，开始计时
		{
		ErrDisplayIndex++;
    if(ErrDisplayIndex>=(5+(6*(int)ErrCode)+10))ErrDisplayIndex=0; //上限到了，开始翻转
		}
	}

//出现错误时显示DCDC的错误ID
void DisplayErrorIDHandler(void)
	{
	int buf;
	//先导提示红黄绿交替闪
  if(ErrDisplayIndex<5)
		{
		if(ErrDisplayIndex<3)LEDMode=(LEDStateDef)(ErrDisplayIndex+1);	
		else LEDMode=LED_OFF;
		}
	//闪烁指定次数显示Err ID
	else if(ErrDisplayIndex<(5+(6*(int)ErrCode)))
		{
		buf=(ErrDisplayIndex-5)/3; 
		if(!(buf%2))LEDMode=LED_Red;
		else LEDMode=LED_OFF;  //按照错误ID闪烁指定次数
		}
  else LEDMode=LED_OFF; //LED熄灭
	}
//内部函数，故障计数器
static char ErrTIMCounter(char buf,char Count)
	{
	//累加计数器
	return buf<8?buf+Count:8;
	}

//输出故障检测
void OutputFaultDetect(void)
	{
	char buf,OErrID;
	bit IsLED6V;
	if(CurrentMode->ModeIdx==Mode_OFF||TailKeyTIM<(TailKeyRelTime+1))ShortDetectTIM=0; //关机状态复位检测
	else if(Current>0) //开始检测
		{		
		buf=ShortDetectTIM&0x0F; //取出定时器值					
		IsLED6V=Data.OutputVoltage>4.5?1:0;//LED类型检测
		//输入过压保护
		if(Data.BatteryVoltage>4.4)ReportError(Fault_InputOVP); 
		//短路检测	
		if(Data.OutputVoltage<2.1||Data.FBInjectVolt>4.8) //输出短路
			{
			buf=ErrTIMCounter(buf,2); //计时器累计
			OErrID=0;
			}
		//输出开路检测
		else if(Data.FBInjectVolt<0.25||Data.OutputVoltage>7.5) 
			{
			buf=ErrTIMCounter(buf,1); //计时器累计
			OErrID=1;
			}
		//LED类型错误检测
		else if(Is6VLED!=IsLED6V)
			{
			buf=ErrTIMCounter(buf,1); //计时器累计
			OErrID=2;
			}
		else buf=buf>0?buf-1:0; //没有发生错误，清除计数器
		//进行定时器数值的回写
		ShortDetectTIM=buf|(OErrID<<4);
		//状态检测
		if(buf<8)return; //没有故障,跳过执行
		switch((ShortDetectTIM>>4)&0x0F)	
			{
			case 1:ReportError(Fault_DCDCOpen);break;
			case 2:ReportError(Fault_StrapMismatch);break;
			default:ReportError(Fault_DCDCShort);
			}
		}
	}