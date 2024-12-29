#include "BattDisplay.h"
#include "ModeControl.h"
#include "LowVoltProt.h"
#include "SideKey.h"

//内部变量
static xdata char BattAlertTimer=0; //电池低电压告警处理
static xdata char RampRiseCurrentTIM=0; //无极调光恢复电流的计时器	

//低电量保护函数
static void StartBattAlertTimer(void)
	{
	//启动定时器
	if(BattAlertTimer)return;
	BattAlertTimer=1;
	}	

//电池低电量报警处理函数
void BattAlertTIMHandler(void)
	{
	//无极调光警报定时
	if(RampRiseCurrentTIM>0&&RampRiseCurrentTIM<9)RampRiseCurrentTIM++;
	//电量警报
	if(BattAlertTimer>0&&BattAlertTimer<(BatteryAlertDelay+1))BattAlertTimer++;
	}	
	
//电池低电量保护函数
void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump)
	{
	char Thr;
	bit IsChangingGear;
	//获取尾按状态
	if(!getSideKey1HEvent())IsChangingGear=0;
	else IsChangingGear=getSideKeyHoldEvent();
	//控制计时器启停
	if(!IsBatteryFault) //电池没有发生低压故障
		{
		Thr=BatteryAlertDelay; //没有故障可以慢一点降档
		//当前在换挡阶段或者没有告警，停止计时器,否则启动
		if(!IsBatteryAlert||IsChangingGear)BattAlertTimer=0;
		else StartBattAlertTimer();
		}
  else //发生低压告警立即启动定时器
		{
	  Thr=BatteryFaultDelay;
		StartBattAlertTimer(); 
		}
	//当前模式需要关机
	if(IsNeedToShutOff||IsChangingGear)
		 {
		 //电池电压低于关机阈值足够时间，立即关闭
		 if(IsBatteryFault&&BattAlertTimer>Thr)ReturnToOFFState(); 
		 }
	//不需要关机，触发换挡动作
	else if(BattAlertTimer>Thr)
		 {
	   BattAlertTimer=0;//重置定时器至初始值
	   SwitchToGear(ModeJump); //复位到指定挡位
		 }
	}		

//无极调光的低电压保护
void RampLowVoltHandler(void)
	{
	if(!IsBatteryAlert&&!IsBatteryFault)//没有告警
		{
		BattAlertTimer=0;
		if(BattState==Battery_Plenty) //电池电量回升到充足状态，缓慢增加电流限制
			{
	    if(RampCfg.CurrentLimit<CurrentMode->Current)
				 {
			   if(!RampRiseCurrentTIM)RampRiseCurrentTIM=1; //启动定时器开始计时
				 else if(RampRiseCurrentTIM<9)return; //时间未到
         RampRiseCurrentTIM=1;
				 if(RampCfg.BattThres>CurrentMode->LowVoltThres)RampCfg.BattThres=CurrentMode->LowVoltThres; //电压检测达到上限，禁止继续增加
				 else RampCfg.BattThres+=50; //电压检测上调50mV
         if(RampCfg.CurrentLimit>CurrentMode->Current)RampCfg.CurrentLimit=CurrentMode->Current;//增加电流之后检测电流值是否超出允许值
				 else RampCfg.CurrentLimit+=250;	//电流上调250mA		 
				 }
			else RampRiseCurrentTIM=0; //已达到电流上限禁止继续增加
			}
		return;
		}
	else RampRiseCurrentTIM=0; //触发警报，复位尝试增加电流的定时器
	//低压告警发生，启动定时器
	StartBattAlertTimer(); //发生命令启动定时器
	if(IsBatteryFault&&BattAlertTimer>4)ReturnToOFFState(); //电池电压低于关机阈值大于0.5秒，立即关闭
	else if(BattAlertTimer>BatteryAlertDelay) //电池挡位触发
		{
		if(RampCfg.CurrentLimit>750)RampCfg.CurrentLimit-=250; //电流下调250mA
		if(RampCfg.BattThres>2750)RampCfg.BattThres-=25; //减少25mV
    BattAlertTimer=1;//重置定时器
		}
	}
