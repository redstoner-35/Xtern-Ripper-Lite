#include "SpecialMode.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"
#include "TempControl.h"
#include "LowVoltProt.h"
#include "TailKey.h"
#include "Strap.h"
#include "SelfTest.h"
#include "SOS.h"

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
		false,
		100
		}, 
		//出错了
		{
		Mode_Fault,
		0,
		0,  //电流0mA
		0,
		false,
		false,
		100
		}, 
		//月光
		{
		Mode_Moon,
		15,  //默认15mA电流
		0,   //最小电流没用到，无视
		2800,  //2.8V关断
		false, //月光档有专用入口，无需带记忆
		false,
		50
		}, 	
    //低亮
		{
		Mode_Low,
		1000,  //1A电流
		0,   //最小电流没用到，无视
		2900,  //2.8V关断
		true,
		false,
		69
		},
    //中亮
		{
		Mode_Mid,
		2000,  //2000mA电流
		0,   //最小电流没用到，无视
		3000,  //3V关断
		true,
		false,
		86
		}, 	
    //中高亮
		{
		Mode_MHigh,
		4000,  //4000mA电流
		0,   //最小电流没用到，无视
		3050,  //3.05V关断
		true,
		true,
		96
		}, 	
    //高亮
		{
		Mode_High,
		8000,  //8000mA电流
		0,   //最小电流没用到，无视
		3100,  //3.1V关断
		true,
		true,
		102
		}, 	
    //极亮
		{
		Mode_Turbo,
		23000,  //23A电流(具体多少电流取决于strap)
		0,   //最小电流没用到，无视
		3200,  //3.2V关断
		false, //极亮不能带记忆
		true,
		106
		}, 	
    //爆闪		
		{
		Mode_Strobe,
		22000,  //22A电流
		0,   //最小电流没用到，无视
		2500,  //2.5V关断(实际上2.7就会拉闸，这里调成2.5是为了避免低电压处理反复触发导致爆闪工作异常)
		false, //爆闪不能带记忆
		true,
		105
		}, 
	  //无极调光		
		{
		Mode_Ramp,
		8000,  //8000mA电流最大
		1000,   //最小1000mA
		3200,  //3.2V关断
		false, //不能带记忆  
		true,
		69
		}, 
	  //SOS
		{
		Mode_SOS,
		12000,  //12A电流
		0,   //最小电流没用到，无视
		2500,  //2.5V关断(实际上2.7就会拉闸，实际上这里调成2.5是为了避免低电压处理反复触发重置SOS状态机导致SOS工作异常)
		false,	//SOS不能带记忆
		true,
		105
		}, 
	};

//全局变量(挡位)
ModeStrDef *CurrentMode; //挡位结构体指针
xdata ModeIdxDef LastMode; //开机为低亮
RampConfigDef RampCfg; //无极调光配置	

//全局变量(状态位)
bit IsRampEnabled; //是否开启无极调光
bit IsRampStart=0; //尾按调整无极调光
	
//软件计时变量
xdata char HoldChangeGearTIM; //挡位模式下长按换挡
xdata char DisplayLockedTIM=0; //锁定和战术模式进入退出显示	
xdata char TailSaveTIM=25; //尾部按键保存计时器
	
//初始化模式状态机
void ModeFSMInit(void)
{
	char i;
	//初始化无极调光
	RampCfg.RampMaxDisplayTIM=0;
  for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)//读取无极调光的数据
	  {
		RampCfg.BattThres=ModeSettings[i].LowVoltThres; //低压检测上限恢复
		RampCfg.CurrentLimit=ModeSettings[i].Current; //找到挡位数据中无极调光的挡位，电流上限恢复
		}
	ReadRampConfig(); //从EEPROM内读取无极调光配置
	//挡位模式配置
	ResetSOSModule(); //复位SOS模块
	LastMode=Mode_Low;
	ErrCode=Fault_None; //没有故障
	CurrentMode=&ModeSettings[0]; //记忆重置为第一个档
  SysMode=Operation_Normal; //平常模式
}	

//挡位状态机所需的软件定时器处理
void ModeFSMTIMHandler(void)
{
	//无极调光相关的定时器
  if(TailSaveTIM<24)TailSaveTIM++;
	if(RampCfg.CfgSavedTIM<32)RampCfg.CfgSavedTIM++;
	if(RampCfg.RampMaxDisplayTIM>0)RampCfg.RampMaxDisplayTIM--;
	//锁定操作提示计时器
  if(DisplayLockedTIM>0)DisplayLockedTIM--;
}

//挡位跳转
void SwitchToGear(ModeIdxDef TargetMode)
	{
	char i;
  int LastICC;
	//记录换档前的结果
	ModeIdxDef BeforeMode=CurrentMode->ModeIdx; //存储当前模式				
	LastICC=CurrentMode->Current; //存储之前的挡位
	//开始寻找
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==TargetMode)
		{
    ResetSOSModule();		//复位整个SOS模块
		CurrentMode=&ModeSettings[i]; //找到匹配index，赋值结构体
		if(BeforeMode==Mode_Turbo&&TargetMode!=Mode_Turbo)RecalcPILoop(LastICC); //从极亮切换到其他挡位，重新设置PI环
		if(BeforeMode==Mode_OFF&&TargetMode!=Mode_OFF)TailMemory_Save(TargetMode); //关机切换到开机，立即保存记忆
		else TailSaveTIM=0; //清除计时器准备等一会再记忆
		}
	}
	
//长按关机函数	
void ReturnToOFFState(void)
	{
	if(CurrentMode->ModeIdx==Mode_OFF)return; //关机状态不执行		
	if(CurrentMode->IsModeHasMemory)LastMode=CurrentMode->ModeIdx; //存储关机前的挡位
	SwitchToGear(Mode_OFF); //强制跳回到关机挡位
	TailMemory_Save(Mode_OFF); //关机的时候立即保存记忆
	}	

//长按换挡的间隔命令生成
void HoldSwitchGearCmdHandler(void)
	{
	char buf;
	if(SysMode||IsRampEnabled)HoldChangeGearTIM=0; //战术模式或者进入锁定、或者处于无极调光模式，禁止换挡系统运行
	else if(!getSideKeyHoldEvent()&&!getSideKey1HEvent())HoldChangeGearTIM=0; //按键松开，计时器复位
	else //执行换挡程序
		{
		buf=HoldChangeGearTIM&0x1F; //取出TIM值
		if(buf==0&&!(HoldChangeGearTIM&0x40))HoldChangeGearTIM|=getSideKey1HEvent()?0x20:0x80; //令换挡命令位1指示换挡可以继续
		HoldChangeGearTIM&=0xE0; //去除掉原始的TIM值
		if(buf<HoldSwitchDelay&&!(HoldChangeGearTIM&0x40))buf++;
		else buf=0;  //时间到，清零结果
		HoldChangeGearTIM|=buf; //把数值写回去
		}
	}	

//获取当前挡位的电流值
int QueryCurrentGearILED(void)	
	{
	//如果当前挡位电流值大于极亮电流则返回极亮限制值
	return CurrentMode->Current<TurboCurrent?CurrentMode->Current:TurboCurrent;
	}

//侧按长按换挡操作执行
static void SideKeySwitchGearHandler(ModeIdxDef TargetMode,char TKCount)	
	{
	if(!(HoldChangeGearTIM&0x80)&&TKCount!=1)return;
	HoldChangeGearTIM&=0x7F; //清除标记位标记本次换挡完成
  SwitchToGear(TargetMode); //换到目标挡位
	}
	
//侧按单击+长按换挡回退操作执行
static void SideKey1HRevGearHandler(ModeIdxDef TargetMode)
	{
	if(!(HoldChangeGearTIM&0x20))return;
	HoldChangeGearTIM&=0xDF; //清除标记位标记本次换挡完成
	SwitchToGear(TargetMode); //换到目标挡位
	}	
	
//无极调光处理
static void RampAdjHandler(char TKCount)
	{
	static bit IsKeyPressed=0;	
	static bit RampDIR=0;
  int Limit;
	bit IsPress;
  //计算出无极调光上限
	IsPress=(getSideKey1HEvent()||getSideKeyHoldEvent())?1:0;
	Limit=RampCfg.CurrentLimit<CurrentMode->Current?RampCfg.CurrentLimit:CurrentMode->Current;
	if(Limit<CurrentMode->Current&&IsPress&&RampCfg.Current>Limit)RampCfg.Current=Limit; //在电流被限制的情况下用户按下按键尝试调整电流，立即限幅
	//尾按模式下循环的方式实现无极调光
	if(!IsRampStart)
		{
		//关闭在状态下单击开始亮度循环
		if(TKCount==1)IsRampStart=1;
		}
	else //开始亮度循环
		{
		if(RampDIR)RampCfg.Current++; 
		else RampCfg.Current--; //调整电流	
		if(RampCfg.Current<=CurrentMode->MinCurrent)
			{
			RampDIR=1;
		  RampCfg.Current=CurrentMode->MinCurrent; //电流达到下限开始翻转
			}
	 if(RampCfg.Current>=Limit) //当前电流大于限制
			{
			RampDIR=0;
			RampCfg.Current=Limit; //限制电流最大值	
			}
		//用户按下按键，结束调整
		if(TKCount||IsKeyEventOccurred())
			{
		  IsRampStart=0;
			RampCfg.CfgSavedTIM=30; //复位定时器进行亮度保存
			}
		}	
	//进行亮度调整
	if(getSideKeyHoldEvent()&&!IsKeyPressed) //长按增加电流
			{	
			if(RampCfg.Current<Limit)RampCfg.Current++;
			else
				{
				RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒指示已经到上限
				RampCfg.Current=Limit; //限制电流最大值	
				IsKeyPressed=1;
				}
			}	
	else if(getSideKey1HEvent()&&!IsKeyPressed) //单击+长按减少电流
		 {
			if(RampCfg.Current>CurrentMode->MinCurrent)RampCfg.Current--; //减少电流	
	    else
				{
				RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒指示已经到下限
				RampCfg.Current=CurrentMode->MinCurrent; //限制电流最小值
				IsKeyPressed=1;
				}
		 }
  else if(!IsPress&&IsKeyPressed)IsKeyPressed=0; //用户放开按键，允许调节		
	//进行数据保存的判断
	if(IsPress)RampCfg.CfgSavedTIM=0; //按键按下说明正在调整，复位计时器
	else if(RampCfg.CfgSavedTIM==32)
			{
			RampCfg.CfgSavedTIM++;
			SaveRampConfig(0);  //一段时间内没操作说明已经调节完毕，保存数据
			}
	}

//检测是否需要关机
static void DetectIfNeedsOFF(int ClickCount)
	{
	if(getSideKeyNClickAndHoldEvent()==2)TriggerVshowDisplay();
	if(!SysMode&&ClickCount!=1)return;
	if(SysMode&&getSideKeyHoldEvent())return;
	ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
	}	

//挡位状态机
void ModeSwitchFSM(void)
	{
	char ClickCount;
	char TKCount=0;
	//外部变量声明
	extern volatile bit StrobeFlag;
	//获取按键状态
	if(!SysMode)TKCount=GetTailKeyCount(); //侧按战术模式激活后禁用尾按
	ClickCount=getSideKeyShortPressCount(0);	//读取按键处理函数传过来的参数
	//挡位记忆参数检查和EEPROM记忆
	if(LastMode==Mode_OFF||LastMode>=ModeTotalDepth)LastMode=Mode_Low;
	//状态机
	IsHalfBrightness=0; //按键灯默认全亮
	switch(CurrentMode->ModeIdx)	
		{
		//出现错误	
		case Mode_Fault:
      SysMode=Operation_Normal; //故障后自动回到普通模式			
			if(!getSideKeyLongPressEvent()||IsErrorFatal())break; //用户没有按下按钮或者是致命的错误状态不允许重置
			ClearError(); //消除掉当前错误
		  break;
		//关机状态
		case Mode_OFF:
		  //处理特殊功能
		  SpecialModeOperation(ClickCount);
	    if(SysMode)break;
		  //非特殊模式正常单击开关机的事项
			if(ClickCount==1||TKCount)PowerToNormalMode(LastMode); //侧按单击开机进入循环	
			//进入极亮和爆闪
			else EnterTurboStrobe(ClickCount,TKCount);		
      if(getSideKeyLongPressEvent())SwitchToGear(Mode_Moon); //长按开机直接进月光					
			if(ClickCount==4) //四击切换挡位模式和无极调光
					{	
					IsRampEnabled=~IsRampEnabled; //转换无极调光状态	
					LEDMode=IsRampEnabled?LED_GreenBlinkThird:LED_RedBlinkThird; //显示是否开启
					SaveRampConfig(0); //保存配置到ROM内
					}
		  //查询电压
			if(getSideKeyNClickAndHoldEvent())TriggerVshowDisplay();
  		break;
		//月光状态
		 case Mode_Moon:
			 IsHalfBrightness=1; //月光模式按键灯亮度减半
			 BatteryLowAlertProcess(true,Mode_Moon);
		   DetectIfNeedsOFF(ClickCount); //执行关机动作检测
		   EnterTurboStrobe(TKCount,0); //尾按模式下，需要可以一键进入极亮或者爆闪所以加上检测			
			 //电池电压充足，长按进入低亮挡位
		   if(getSideKeyLongPressEvent()||TKCount==1)  
					{
					PowerToNormalMode(Mode_Low); //开机到低亮模式
					if(CurrentMode->ModeIdx==Mode_Moon)break;//换挡之后无法成功离开月光模式，不进行下面的复位操作
					if(IsRampEnabled)RestoreToMinimumRampCurrent(); //如果是无极调光则恢复到最低电流
					HoldChangeGearTIM|=0x40; //短时间内禁止长按换挡，确保要用户松开后才能换
					if(!TKCount)RampCfg.RampMaxDisplayTIM=4; //侧按模式因为没有物理断电，需要熄灭0.5秒进行切换
					}		    
		    break;			
    //无极调光状态				
    case Mode_Ramp:
		    if(!IsRampStart) //非调整模式，允许关机和进入其他模式
					{
					if(TKCount==4)SwitchToGear(Mode_Moon); //尾按四击进入月光
			    DetectIfNeedsOFF(ClickCount); //检测是否需要关机
					EnterTurboStrobe(TKCount,ClickCount); //进入极亮或者爆闪的检测
					}
		    //无极调光处理
		    RampLowVoltHandler(); //低电压保护
        RampAdjHandler(TKCount);			
		    break;
    //低亮状态		
    case Mode_Low:
			  BatteryLowAlertProcess(true,Mode_Low);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
				EnterTurboStrobe(TKCount,ClickCount); //进入极亮或者爆闪的检测
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Mid,TKCount); //换到中档
		    break;	    		
    //中亮状态		
    case Mode_Mid:
			  BatteryLowAlertProcess(false,Mode_Low);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
				EnterTurboStrobe(TKCount,ClickCount); //进入极亮或者爆闪的检测
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_MHigh,TKCount); //换到中高档
		    SideKey1HRevGearHandler(Mode_Low); //单击+长按回退挡位到低档
		    break;	
	  //中高亮状态
    case Mode_MHigh:
			  BatteryLowAlertProcess(false,Mode_Mid);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
				EnterTurboStrobe(TKCount,ClickCount); //进入极亮或者爆闪的检测
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_High,TKCount); //换到高档
		    SideKey1HRevGearHandler(Mode_Mid); //单击+长按回退挡位到中档
		    break;	
	  //高亮状态
    case Mode_High:
			  BatteryLowAlertProcess(false,Mode_MHigh);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
				EnterTurboStrobe(TKCount,ClickCount); //进入极亮或者爆闪的检测
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Low,0); //换到低档位构成循环
		    if(TKCount==1)SwitchToGear(Mode_Moon); //跳到月光	    
		    SideKey1HRevGearHandler(Mode_MHigh); //单击+长按回退挡位到中高档
		    break;
		//极亮状态
    case Mode_Turbo:
			  BatteryLowAlertProcess(false,Mode_High);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
			  if(TKCount==1||ClickCount==2||IsForceLeaveTurbo)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //双击或者温度达到上限值，强制返回到低亮
				if(TKCount==3||ClickCount==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
		    break;	
		//爆闪状态
    case Mode_Strobe:
			  BatteryLowAlertProcess(true,Mode_Strobe);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
		    LeaveSpecialMode(TKCount,ClickCount); //退出特殊模式回到其他地方的入口
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_SOS,TKCount); //长按切换到SOS
		    break;	
    //SOS求救挡位		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
			  LeaveSpecialMode(TKCount,ClickCount); //退出特殊模式回到其他地方的入口
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Strobe,TKCount); //长按切换到爆闪
		    break;	
		}
  //应用输出电流
	if(DisplayLockedTIM>0||IsDisplayLocked)Current=50; //用户进入或者退出锁定，用50mA短暂点亮提示一下
	else if(RampCfg.RampMaxDisplayTIM>0)Current=0; //无极调光模式在抵达上下限后短暂熄灭
	else switch(CurrentMode->ModeIdx)	
		{
		case Mode_SOS: 
		case Mode_Strobe://爆闪模式和SOS模式	     
	     switch(BattState)//取出挡位电流
				 {
				 case Battery_Plenty:Current=QueryCurrentGearILED();break;
			   case Battery_Mid:Current=10000;break;
         case Battery_Low:Current=5000;break;
				 case Battery_VeryLow:Current=110;break;
				 }
			 //根据状态控制电流
			 if(CurrentMode->ModeIdx==Mode_Strobe&&!StrobeFlag)Current=0; 
			 if(CurrentMode->ModeIdx==Mode_SOS&&!SOSFSM())Current=0;
		   break; 
		//其余模式，电流取正常值
		default:
		  if(LowPowerStrobe())Current=0; //触发低压报警，闪烁
			else if(CurrentMode->ModeIdx==Mode_Ramp)Current=RampCfg.CurrentLimit<RampCfg.Current?RampCfg.CurrentLimit:RampCfg.Current; //无极调光模式取结构体内数据
		  else Current=QueryCurrentGearILED();//其他挡位使用设置值作为目标电流
		}	
	//清除按键处理
	getSideKeyShortPressCount(1); 
	//ROM挡位记忆控制
	if((IsPOSTKPressed&&TKCount)||TailSaveTIM==24) //在挡位停留的时间足够或者开机时按下了挡位开关，保存数据
		{
		IsPOSTKPressed=0; //复位标记为
		TailSaveTIM++;
		TailMemory_Save(CurrentMode->ModeIdx);
		}
	}