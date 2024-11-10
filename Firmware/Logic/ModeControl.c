#include "ModeControl.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"
#include "cms8s6990.h"

//挡位结构体
code ModeStrDef ModeSettings[ModeTotalDepth]=
	{
		//关机状态
    {
		Mode_OFF,
		0,
		0,  //电流0mA
		0,  //关机状态阈值为0强制解除警报
		true,
		false
		}, 
		//出错了
		{
		Mode_Fault,
		0,
		0,  //电流0mA
		0,
		false,
		false
		}, 
		//月光
		{
		Mode_Moon,
		350,  //默认350mA电流
		0,   //最小电流没用到，无视
		2800,  //2.8V关断
		false, //月光档有专用入口，无需带记忆
		false
		}, 	
    //低亮
		{
		Mode_Low,
		800,  //800mA电流
		0,   //最小电流没用到，无视
		2900,  //2.8V关断
		true,
		false
		},
    //中亮
		{
		Mode_Mid,
		1500,  //1500mA电流
		0,   //最小电流没用到，无视
		3000,  //3V关断
		true,
		false
		}, 	
    //中高亮
		{
		Mode_MHigh,
		3000,  //3000mA电流
		0,   //最小电流没用到，无视
		3050,  //3.05V关断
		true,
		true
		}, 	
    //高亮
		{
		Mode_High,
		7500,  //7500mA电流
		0,   //最小电流没用到，无视
		3100,  //3.1V关断
		true,
		true
		}, 	
    //极亮
		{
		Mode_Turbo,
		14000,  //14000mA电流
		0,   //最小电流没用到，无视
		3200,  //3.2V关断
		false, //极亮不能带记忆
		true
		}, 	
    //爆闪		
		{
		Mode_Strobe,
		10000,  //10000mA电流
		0,   //最小电流没用到，无视
		3000,  //3.0V关断
		false, //爆闪不能带记忆
		true
		}, 
	  //无极调光		
		{
		Mode_Ramp,
		7500,  //7500mA电流最大
		500,   //最小500mA
		3000,  //3.0V关断
		false, //不能带记忆  
		true
		}, 
	  //SOS
		{
		Mode_SOS,
		10000,  //10000mA电流
		0,   //最小电流没用到，无视
		3000,  //3.0V关断
		false,	//SOS不能带记忆
		true
		}, 
	};

//全局变量(挡位)
ModeStrDef *CurrentMode; //挡位结构体指针
xdata ModeIdxDef LastMode; //开机为低亮
RampConfigDef RampCfg; //无极调光配置		
xdata MoonLightBrightnessDef MoonCfg;	 //月光模式配置
	
//全局变量(状态位)
bit IsRampEnabled; //是否开启无极调光
bit IsLocked; //锁定指示
bit IsTacMode; //开启战术模式
bit IsEnableMoonConfigMode; //打开月光配置模式
bit IsSideLEDCfgMode; //侧按LED配置模式
static xdata SOSStateDef SOSState; //全局变量状态位
xdata FaultCodeDef ErrCode; //错误代码	
	
//软件计时变量
xdata char BattAlertTimer=0; //电池低电压告警调档
xdata char HoldChangeGearTIM=0; //挡位模式下长按换挡
xdata char DisplayLockedTIM=0; //锁定和战术模式进入退出显示	
xdata char ClickHoldReverseGearTIM=0; //挡位模式下单击+长按倒向换挡
xdata	char MoonCfgTIM=0; //月光挡位配置计时
xdata char SOSTIM=0;  //SOS计时
	
//初始化模式状态机
void ModeFSMInit(void)
{
	RampCfg.RampMaxDisplayTIM=0;
	ReadRampConfig(); //从EEPROM内读取无极调光配置
	//挡位模式配置
	SOSState=SOSState_Prepare; //SOS状态机重置为初始值
	LastMode=Mode_Low;
	ErrCode=Fault_None; //没有故障
	CurrentMode=&ModeSettings[0]; //记忆重置为第一个档
	IsLocked=0; //关闭锁定
	IsEnableMoonConfigMode=0;
	IsSideLEDCfgMode=0; //非配置模式
	IsTacMode=0; //退出战术模式
}	

//SOS处理模块
static int SOSFSM(void)
{
  int buf;
	switch(SOSState)
		{
		//准备阶段
		case SOSState_Prepare:
			 SOSTIM=(3*SOSDotTime*2)-1;
			 SOSState=SOSState_3Dot;
		   break;
		//第一次三点
		case SOSState_3Dot:
		   buf=SOSTIM%(SOSDotTime*2); //根据参数设置换算计时器的总时间
       if(buf>(SOSDotTime-1))return CurrentMode->Current; //当前状态需要LED电流，返回目标电流值		
		   if(SOSTIM==0) //显示结束
				 {
				 SOSTIM=SOSGapTime; 
				 SOSState=SOSState_3DotWait;  //进入延时等待阶段
				 }
		   break;
		//三点结束后的等待延时阶段
	  case SOSState_3DotWait:
			 if(SOSTIM>0)break;
		   SOSTIM=(3*SOSDashTime*2)-1;
		   SOSState=SOSState_3Dash;
		   break;
		//三划
		case SOSState_3Dash:
			 buf=SOSTIM%(SOSDashTime*2); //根据参数设置换算计时器的总时间
       if(buf>(SOSDashTime-1))return CurrentMode->Current; //当前状态需要LED电流，返回目标电流值	
		   if(SOSTIM==0) //显示结束
				 {
				 SOSTIM=SOSGapTime; 
				 SOSState=SOSState_3DashWait;  //进入延时等待阶段
				 }
		   break;			
		//三划结束后的等待延时阶段
	  case SOSState_3DashWait:
			 if(SOSTIM>0)break;
			 SOSTIM=(3*SOSDotTime*2)-1;
			 SOSState=SOSState_3DotAgain;
		   break;		
		//第二次三点
		case SOSState_3DotAgain:
			 buf=SOSTIM%(SOSDotTime*2); //根据参数设置换算计时器的总时间
       if(buf>(SOSDotTime-1))return CurrentMode->Current; //当前状态需要LED电流，返回目标电流值		
		   if(SOSTIM==0) //显示结束
				 {
				 SOSTIM=SOSFinishGapTime; 
				 SOSState=SOSState_Wait;  //进入延时等待阶段
				 }
		   break;		
	  //本轮信号发出完毕，等待
	  case SOSState_Wait:	
			 if(SOSTIM>0)break;
		   SOSState=SOSState_Prepare; //回到准备状态
		   break;
		}
	//其余情况返回-1
	return -1;
}

//月光挡位循环配置功能
void MoonConfigHandler(void)
{
	int buf;
	//非月光模式或者配置未打开，禁止配置
	if(!IsEnableMoonConfigMode||CurrentMode->ModeIdx!=Mode_Moon)MoonCfgTIM=0; 
	//启用配置模式，循环操作
  else
		{
		MoonCfgTIM++;
		if(MoonCfgTIM<16)return;
		MoonCfgTIM=0;
		//开始递增并反复循环月光挡位的index以构成循环
		buf=(int)MoonCfg;
		if(buf<(int)MoonLight_UsingModeDef)buf++;
		else buf=0;
		MoonCfg=(MoonLightBrightnessDef)buf; //反复循环index
		}
}

//挡位状态机所需的软件定时器处理
void ModeFSMTIMHandler(void)
{
	char buf;
	//SOS定时器
	if(SOSTIM>0)SOSTIM--;
	//无极调光提示定时器
	if(RampCfg.CfgSavedTIM<32)RampCfg.CfgSavedTIM++;
	if(RampCfg.RampMaxDisplayTIM>0)RampCfg.RampMaxDisplayTIM--;
	//锁定操作提示计时器
  if(DisplayLockedTIM>0)DisplayLockedTIM--;
	//检测定时器状态
	if(BattAlertTimer&0x80)
		{
		buf=BattAlertTimer&0x7F; //取出TIM值
		BattAlertTimer&=0x80; //去除掉原始的TIM值
		if(buf<(BatteryAlertDelay+1))buf++;
		BattAlertTimer|=buf; //把数值写回去
		}
	else BattAlertTimer=0; //清除buf	
}

//挡位跳转
int SwitchToGear(ModeIdxDef TargetMode)
	{
  int i;
	extern xdata float VBattBeforeTurbo;
	ModeIdxDef BeforeMode=CurrentMode->ModeIdx; //存储当前模式	
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==TargetMode)
		{
		SOSState=SOSState_Prepare; //每次换挡都把SOS状态机重置为初始值
		CurrentMode=&ModeSettings[i]; //找到匹配index，赋值结构体
		if(TargetMode==Mode_Turbo&&BeforeMode!=Mode_Turbo)VBattBeforeTurbo=Data.RawBattVolt; //切换到turbo模式时进行采样
		return 0;
		}
	//啥也没找到，出错
	return 1;
	}

//长按关机函数	
void ReturnToOFFState(void)
	{
	if(CurrentMode->ModeIdx==Mode_OFF)return; //关机状态不执行		
	if(CurrentMode->IsModeHasMemory)LastMode=CurrentMode->ModeIdx; //存储关机前的挡位
	SwitchToGear(Mode_OFF); //强制跳回到关机挡位
	}	
	
//低电量保护函数
static void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump)
	{
	char time,Thr;
	if(!IsBatteryAlert&&!IsBatteryFault)//没有告警
		{
		BattAlertTimer=0;
		return;
		}
	if(BattAlertTimer==0)BattAlertTimer=0x80;//定时器启动
	time=BattAlertTimer&0x7F; //获取当前的计时值
	if(!IsBatteryFault)Thr=BatteryAlertDelay;
	else Thr=2;
	//电池电量严重过低
	if(IsNeedToShutOff&&IsBatteryFault&&time>=3)ReturnToOFFState(); //电池电压低于关机阈值大于0.5秒，立即关闭
	//触发动作
	else if(time>Thr)
		 {
	   BattAlertTimer=0x80;//重置定时器
	   SwitchToGear(ModeJump); //复位到指定挡位
		 }
	}	

//长按换挡的间隔命令生成
void HoldSwitchGearCmdHandler(void)
	{
	char buf;
	if(!getSideKeyHoldEvent()||CurrentMode->ModeIdx==Mode_OFF)HoldChangeGearTIM=0; //按键松开或者是关机状态，计时器复位
	else
		{
		buf=HoldChangeGearTIM&0x3F; //取出TIM值
		if(buf==0&&!(HoldChangeGearTIM&0x40))HoldChangeGearTIM|=0x80; //令最高位为1指示换挡可以继续
		HoldChangeGearTIM&=0xC0; //去除掉原始的TIM值
		if(buf<HoldSwitchDelay&&!(HoldChangeGearTIM&0x40))buf++;
		else buf=0;  //时间到，清零结果
		HoldChangeGearTIM|=buf; //把数值写回去
		}
	//单击+长按倒换
  if(!getSideKeyClickAndHoldEvent()||CurrentMode->ModeIdx==Mode_OFF)ClickHoldReverseGearTIM=0;	//按键松开或者是关机状态，计时器复位	
	else
		{
		buf=ClickHoldReverseGearTIM&0x3F; //取出TIM值
		if(buf==0&&!(ClickHoldReverseGearTIM&0x40))ClickHoldReverseGearTIM|=0x80; //令最高位为1指示换挡可以继续
		ClickHoldReverseGearTIM&=0xC0; //去除掉原始的TIM值
		if(buf<HoldSwitchDelay&&!(ClickHoldReverseGearTIM&0x40))buf++;
		else buf=0;  //时间到，清零结果
		ClickHoldReverseGearTIM|=buf; //把数值写回去
		}
	}	
	
//侧按长按换挡操作执行
static void SideKeySwitchGearHandler(ModeIdxDef TargetMode)	
	{
	if(!(HoldChangeGearTIM&0x80))return;
	HoldChangeGearTIM&=0x7F; //清除标记位标记本次换挡完成
  SwitchToGear(TargetMode); //换到目标挡位
	}
	
//侧按单击+长按换挡回退操作执行
static void SideKey1HRevGearHandler(ModeIdxDef TargetMode)
	{
	if(!(ClickHoldReverseGearTIM&0x80))return;
	ClickHoldReverseGearTIM&=0x7F; //清除标记位标记本次换挡完成
	SwitchToGear(TargetMode); //换到目标挡位
	}	
//侧按指示灯亮度配置函数
void SideKeyLEDBriAdjHandler(void)
	{
	static xdata bool SideLEDRampDir=false;
	static xdata char SpeedDIV=8;
	//当前占空比正在调整
	if(LEDMgmt_WaitSubmitDuty())return;
	//减缓速度的分频	
	SpeedDIV--;	
	if(SpeedDIV)return;
	SpeedDIV=8;
	//从低ramp到高
	if(!SideLEDRampDir)
		{
		if(LEDBrightNess<2399)LEDBrightNess++;
		else SideLEDRampDir=true; //翻转状态
		}
	//从高Ramp到低
	else
		{
		if(LEDBrightNess>50)LEDBrightNess--;
		else SideLEDRampDir=false;
		}
		LEDMgmt_SetBrightness(); //将更改后的亮度保存
	}				 	
	
//无极调光处理
static void RampAdjHandler(void)
	{
	static bit IsKeyPressed=0;		
	//进行亮度调整
	if(getSideKeyHoldEvent()&&!IsKeyPressed)RampCfg.Current+=3; //正向增加或者减少电流
	else if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed)RampCfg.Current-=3; //增加或者减少电流	
  else if(!getSideKeyClickAndHoldEvent()&&!getSideKeyHoldEvent()&&IsKeyPressed)IsKeyPressed=0; //用户放开按键，允许调节		
	//电流达到上限
	if(getSideKeyHoldEvent()&&!IsKeyPressed&&RampCfg.Current>=CurrentMode->Current)
			{
			RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒指示已经到上限
			RampCfg.Current=CurrentMode->Current; //限制电流最大值	
			IsKeyPressed=1;
			}		
	//电流达到下限
	if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed&&RampCfg.Current<=CurrentMode->MinCurrent)
			{
			RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒指示已经到下限
			RampCfg.Current=CurrentMode->MinCurrent; //限制电流最小值
			IsKeyPressed=1;
			}			
	//进行数据保存的判断
	if(getSideKeyHoldEvent()||getSideKeyClickAndHoldEvent())RampCfg.CfgSavedTIM=0; //按键按下说明正在调整，复位计时器
	else if(RampCfg.CfgSavedTIM==32)
			{
			RampCfg.CfgSavedTIM++;
			SaveRampConfig(0);  //一段时间内没操作说明已经调节完毕，保存数据
			}
	}
//PI环路的温控数据处理声明
int ThermalILIMCalc(int Input);
	
//挡位状态机
void ModeSwitchFSM(void)
	{
	bit IsHoldEvent;
	int ClickCount;
	xdata float TargetCurrent; //当前目标电流	
	//外部变量声明
	extern volatile bit StrobeFlag;
	extern bit IsDisableTurbo;
	extern bit IsForceLeaveTurbo;
	//获取按键状态
	IsHoldEvent=getSideKeyLongPressEvent();	
	ClickCount=getSideKeyShortPressCount(1);	//读取按键处理函数传过来的参数
	//挡位记忆参数检查
	if(LastMode==Mode_OFF||LastMode>=ModeTotalDepth)LastMode=Mode_Low;
	//状态机
	if(ErrCode==Fault_DCDCFailedToStart||ErrCode==Fault_DCDCENOOC)return; //致命初始化错误
	else switch(CurrentMode->ModeIdx)	
		{
		//出现错误	
		case Mode_Fault:
      IsTacMode=0; //故障后自动取消战术模式			
			if(!IsHoldEvent||ErrCode==Fault_OverHeat)break; //用户没有按下按钮或者是过热状态不允许重置
		  ErrCode=Fault_None; //无故障
			SwitchToGear(Mode_OFF);  //长按重置错误
		  break;
		//关机状态
		case Mode_OFF:
			if(ClickCount==5)
					{
					IsTacMode=0; //锁定解锁时自动退出战术模式
					IsLocked=IsLocked?0:1; //锁定状态切换
					DisplayLockedTIM=8; //指示锁定状态切换
					}
			else if(IsLocked&&(ClickCount>0||IsKeyEventOccurred()))LEDMode=LED_RedBlinkFifth; //指示手电已被锁定
			//非锁定状态正常处理的事项
	    if(IsLocked)break;
			//战术模式
      if(ClickCount==6)  //6击进入
					{
					IsTacMode=IsTacMode?0:1; //切换战术模式开关
					DisplayLockedTIM=2; //指示战术切换
					}
			if(IsTacMode) //战术模式激活时进行判断
					{
					if(!getSideKeyHoldEvent())break;
					if(Battery>3.1&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //电池电量充足且没有过热锁极亮，正常开启
					else if(Battery>2.7)SwitchToGear(Mode_High);  //电池电池电量不足时进入高亮
					else LEDMode=LED_RedBlinkFifth; //电池电量严重不足，红色闪五次
					break;
					}
		  //非锁定正常单击开关机的事项
			if(ClickCount==1) //侧按单击开机进入循环
					{
				  if(Battery>2.9)SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode); //正常开启
					else if(Battery>2.7)SwitchToGear(Mode_Moon);	 //大于2.7V的时候只能开月光
					else LEDMode=LED_RedBlinkFifth; //电池电量严重不足，红色闪五次
					}			
			else if(ClickCount==2)  //双击一键极亮		
					{
					if(IsDisableTurbo)break; //手电温度过高锁死极亮，不做反应
				  else if(Battery>3.1)SwitchToGear(Mode_Turbo); //电池电量充足正常开启
					else SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode);  //电池电池电量不足时双击进入普通模式
					}
      else if(IsHoldEvent)SwitchToGear(Mode_Moon); //长按开机直接进月光					
			else if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按三击进入爆闪   
			else if(ClickCount==4) //四击切换挡位模式和无极调光
					{	
					IsRampEnabled=IsRampEnabled?0:1; //转换无极调光状态	
					LEDMode=!IsRampEnabled?LED_RedBlinkThird:LED_GreenBlinkThird; //显示是否开启
					SaveRampConfig(0); //保存配置到ROM内
					}
			else if(getSideKeyClickAndHoldEvent())TriggerVshowDisplay(); //单击长按查看电池当前电压和电量
  		break;
		//月光状态
		 case Mode_Moon:
			 BatteryLowAlertProcess(true,Mode_Moon);
			 if(ClickCount==1)//侧按单击关机
					{
					if(IsEnableMoonConfigMode||IsSideLEDCfgMode)SaveRampConfig(0); //月光和侧按亮度发生调整，保存配置到ROM内
			    IsEnableMoonConfigMode=0;
			    IsSideLEDCfgMode=0;
					ReturnToOFFState(); //回到关机状态
					}
			 if(ClickCount==5&&!IsSideLEDCfgMode)IsSideLEDCfgMode=1;	 //五击调整侧按LED亮度	
			 //启用侧按LED亮度配置
       if(IsSideLEDCfgMode)SideKeyLEDBriAdjHandler();
			 if(IsEnableMoonConfigMode||IsSideLEDCfgMode)break; //进入配置模式后阻止响应
			 //非配置模式下允许的操作
			 if(ClickCount==4)IsEnableMoonConfigMode=1; //四击进入配置模式
		   if(IsHoldEvent&&Battery>2.9)  //电池电压充足，长按进入低亮挡位
					{
					SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //长按回到正常挡位模式
					if(IsRampEnabled)RestoreToMinimumRampCurrent(); //如果是无极调光则恢复到最低电流
					HoldChangeGearTIM|=0x40; //短时间内禁止长按换挡，确保要用户松开后才能换
					RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒进行切换
					}		    
		    break;			
    //无极调光状态				
    case Mode_Ramp:
			  BatteryLowAlertProcess(true,Mode_Ramp);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
				if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    //无极调光处理
        RampAdjHandler();			
		    break;
    //低亮状态		
    case Mode_Low:
			  BatteryLowAlertProcess(true,Mode_Low);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Mid); //换到中档
		    break;	    		
    //中亮状态		
    case Mode_Mid:
			  BatteryLowAlertProcess(false,Mode_Low);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_MHigh); //换到中高档
		    SideKey1HRevGearHandler(Mode_Low); //单击+长按回退挡位到低档
		    break;	
	  //中高亮状态
    case Mode_MHigh:
			  BatteryLowAlertProcess(false,Mode_Mid);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_High); //换到高档
		    SideKey1HRevGearHandler(Mode_Mid); //单击+长按回退挡位到中档
		    break;	
	  //高亮状态
    case Mode_High:
			  BatteryLowAlertProcess(false,Mode_MHigh);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Low); //换到低档位构成循环
		    SideKey1HRevGearHandler(Mode_MHigh); //单击+长按回退挡位到中高档
		    break;
		//极亮状态
    case Mode_Turbo:
			  BatteryLowAlertProcess(false,Mode_High);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
			  if(ClickCount==2||IsForceLeaveTurbo)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //双击或者温度达到上限值，强制返回到低亮
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    break;	
		//爆闪状态
    case Mode_Strobe:
			  BatteryLowAlertProcess(true,Mode_Strobe);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //三击退回到普通模式
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_SOS); //长按切换到SOS
		    break;	
    //SOS求救挡位		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //三击退回到普通模式
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Strobe); //长按切换到爆闪
		    break;	
		}
  //应用输出电流
	if(DisplayLockedTIM>0)TargetCurrent=80; //用户进入或者退出锁定，用80mA短暂点亮提示一下
	else if(!StrobeFlag&&CurrentMode->ModeIdx==Mode_Strobe)TargetCurrent=-1;	 //爆闪模式下让电流跟随flag(-1电流表示不关闭防反接FET)
	else if(RampCfg.RampMaxDisplayTIM>0)TargetCurrent=-1; //无极调光模式在抵达上下限后短暂熄灭(-1电流表示不关闭防反接FET)
  else if(CurrentMode->ModeIdx==Mode_Ramp)TargetCurrent=RampCfg.Current; //无极调光模式取无极调光参数结构体内的电流
	else if(CurrentMode->ModeIdx==Mode_SOS)TargetCurrent=SOSFSM(); //SOS模式，输出电流受SOS状态机调控
	else if(CurrentMode->ModeIdx==Mode_Moon)switch(MoonCfg)
		{
		case MoonLight_10mA:TargetCurrent=10;break;  //10mA
		case MoonLight_25mA:TargetCurrent=25;break;  //25mA
		case MoonLight_50mA:TargetCurrent=50;break;  //50mA
		case MoonLight_100mA:TargetCurrent=100;break; //100mA
		case MoonLight_200mA:TargetCurrent=200;break; //200mA
		case MoonLight_UsingModeDef:TargetCurrent=CurrentMode->Current;break; //使用模式结构体内的结果
		}
  else TargetCurrent=CurrentMode->Current;		
	//根据温控的运算结果对输出电流进行限幅
	Current=ThermalILIMCalc(TargetCurrent);	
	}