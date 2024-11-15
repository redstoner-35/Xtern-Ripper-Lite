#include "LEDMgmt.h"
#include "delay.h"
#include "ADCCfg.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "TailKey.h"

//内部变量
static xdata int ErrDisplayIndex;
static xdata char ShortDetectTIM;
xdata float VBattBeforeTurbo;
xdata char InputDetectTIM;

//报告错误
void ReportError(FaultCodeDef Code)
	{
	ErrCode=Code;
	if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //指示故障发生
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
  if(ErrDisplayIndex<5)switch(ErrDisplayIndex) 
		{
		case 0:LEDMode=LED_Green;break;
		case 1:LEDMode=LED_Amber;break;
		case 2:LEDMode=LED_Red;break;
		default:LEDMode=LED_OFF;
		}
	else if(ErrDisplayIndex<(5+(6*(int)ErrCode)))
		{
		buf=(ErrDisplayIndex-5)/3; 
		if(!(buf%2))LEDMode=LED_Red;
		else LEDMode=LED_OFF;  //按照错误ID闪烁指定次数
		}
  else LEDMode=LED_OFF; //LED熄灭
	}
	
//输出故障检测
void OutputFaultDetect(void)
	{
	char buf;
	if(CurrentMode->ModeIdx==Mode_OFF||TailKeyTIM<(TailKeyRelTime+1))ShortDetectTIM=0; //关机状态复位检测
	else if(Current>0) //开始检测
		{
		buf=ShortDetectTIM&0x7F; //取出定时器值
		if(Data.OutputVoltage<4.5) //输出短路
			{
			buf=buf<8?buf+2:8; //计时器累计
			ShortDetectTIM&=0x7F;
			}
		else if(Data.OutputVoltage>7.15) //输出开路
			{
			buf=buf<8?buf+1:8;  //计时器累计
			ShortDetectTIM|=0x80;
			}
			else buf=buf>0?buf-1:0; //没有发生错误，清除计数器
		//进行定时器数值的回写
		ShortDetectTIM&=0x80;
		ShortDetectTIM|=buf;
		//状态检测
		if(buf<8)return; //没有故障
		ReportError(ShortDetectTIM&0x80?Fault_DCDCOpen:Fault_DCDCShort); //时间到，报告错误
		}
	}