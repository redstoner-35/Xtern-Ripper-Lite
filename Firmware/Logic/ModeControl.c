#include "ModeControl.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"
#include "cms8s6990.h"
#include "TempControl.h"
#include "TailKey.h"
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
		12000,  //12A电流
		0,   //最小电流没用到，无视
		3000,  //3.0V关断
		false, //爆闪不能带记忆
		true,
		105
		}, 
	  //无极调光		
		{
		Mode_Ramp,
		8000,  //8000mA电流最大
		1000,   //最小1000mA
		3100,  //3.1V关断
		false, //不能带记忆  
		true,
		69
		}, 
	  //SOS
		{
		Mode_SOS,
		12000,  //12A电流
		0,   //最小电流没用到，无视
		3000,  //3.0V关断
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
bit IsLocked; //锁定指示
bit IsTacMode; //开启战术模式
bit IsRampStart=0; //尾按调整无极调光
	
//软件计时变量
xdata char BattAlertTimer=0; //电池低电压告警调档
xdata char HoldChangeGearTIM=0; //挡位模式下长按换挡
xdata char DisplayLockedTIM=0; //锁定和战术模式进入退出显示	
xdata char ClickHoldReverseGearTIM=0; //挡位模式下单击+长按倒向换挡
xdata char TailSaveTIM=25; //尾部按键保存计时器
xdata char RampRiseCurrentTIM=0; //无极调光恢复电流的计时器	
	
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
	IsLocked=0; //关闭锁定
	IsTacMode=0; //退出战术模式
}	

//挡位状态机所需的软件定时器处理
void ModeFSMTIMHandler(void)
{
	char buf;
	//无极调光相关的定时器
  if(TailSaveTIM<24)TailSaveTIM++;
	if(RampRiseCurrentTIM>0&&RampRiseCurrentTIM<9)RampRiseCurrentTIM++;
	if(RampCfg.CfgSavedTIM<32)RampCfg.CfgSavedTIM++;
	if(RampCfg.RampMaxDisplayTIM>0)RampCfg.RampMaxDisplayTIM--;
	//锁定操作提示计时器
  if(DisplayLockedTIM>0)DisplayLockedTIM--;
	//电池警报计时器
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
  int i,LastICC;
	ModeIdxDef BeforeMode=CurrentMode->ModeIdx; //存储当前模式				
	LastICC=CurrentMode->Current; //存储之前的挡位
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==TargetMode)
		{
    ResetSOSModule();		//复位整个SOS模块
		CurrentMode=&ModeSettings[i]; //找到匹配index，赋值结构体
		if(BeforeMode==Mode_Turbo&&TargetMode!=Mode_Turbo)RecalcPILoop(LastICC); //从极亮切换到其他挡位，重新设置PI环
		if(BeforeMode==Mode_OFF&&TargetMode!=Mode_OFF)TailMemory_Save(TargetMode); //关机切换到开机，立即保存记忆
		else TailSaveTIM=0; //清除计时器准备等一会再记忆
		return 0;
		}
	//啥也没找到，出错
	return 1;
	}
	
//无极调光的低电压保护
void RampLowVoltHandler(void)
	{
	char time;
	extern BattStatusDef BattState;
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
	if(BattAlertTimer==0)BattAlertTimer=0x80;//定时器启动
	time=BattAlertTimer&0x7F; //获取当前的计时值
	if(IsBatteryFault&&time>4)ReturnToOFFState(); //电池电压低于关机阈值大于0.5秒，立即关闭
	else if(time>BatteryAlertDelay) //电池挡位触发
		{
		if(RampCfg.CurrentLimit>750)RampCfg.CurrentLimit-=250; //电流下调250mA
		if(RampCfg.BattThres>2750)RampCfg.BattThres-=25; //减少25mV
    BattAlertTimer=0x80;//重置定时器
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
	if(CurrentMode->ModeIdx==Mode_OFF)return; //关机状态禁止计时器运行
	if(!getSideKeyHoldEvent())HoldChangeGearTIM=0; //按键松开或者是关机状态，计时器复位
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
  if(!getSideKeyClickAndHoldEvent())ClickHoldReverseGearTIM=0;	//按键松开或者是关机状态，计时器复位	
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
static void SideKeySwitchGearHandler(ModeIdxDef TargetMode,char TKCount)	
	{
	if(!(HoldChangeGearTIM&0x80)&&TKCount!=1)return;
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
	
//无极调光处理
static void RampAdjHandler(char TKCount)
	{
	static bit IsKeyPressed=0;	
	static bit RampDIR=0;
  int Limit;
	bit IsPress;
  //计算出无极调光上限
	IsPress=(getSideKeyClickAndHoldEvent()||getSideKeyHoldEvent())?1:0;
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
		if(TKCount==1||IsKeyEventOccurred())
			{
		  IsRampStart=0;
			RampCfg.CfgSavedTIM=30; //复位定时器进行亮度保存
			}
		}	
	//进行亮度调整
	if(getSideKeyHoldEvent()&&!IsKeyPressed)RampCfg.Current++; //正向增加或者减少电流
	else if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed)RampCfg.Current--; //增加或者减少电流	
  else if(!IsPress&&IsKeyPressed)IsKeyPressed=0; //用户放开按键，允许调节		
	//电流达到上限
	if(getSideKeyHoldEvent()&&!IsKeyPressed&&RampCfg.Current>=Limit)
			{
			RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒指示已经到上限
			RampCfg.Current=Limit; //限制电流最大值	
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
	if(ClickCount!=1)return;
	if(!IsTacMode&&getSideKeyHoldEvent())return;
	ReturnToOFFState();//侧按单击或者在战术模式下松开按钮时关机
	}	

//进入极亮和爆闪的判断
static void EnterTurboStrobe(int TKCount,int ClickCount)	
	{
	extern bit IsDisableTurbo;
	int Count=TKCount>ClickCount?TKCount:ClickCount;
	if(Count==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
	if(Count==3)SwitchToGear(Mode_Strobe); //侧按3击进入爆闪
	}

//挡位状态机
void ModeSwitchFSM(void)
	{
	bit IsHoldEvent;
	int ClickCount;
	char TKCount;
	//外部变量声明
	extern volatile bit StrobeFlag;
	extern bit IsForceLeaveTurbo;
	//获取按键状态
	TKCount=GetTailKeyCount();
	IsHoldEvent=getSideKeyLongPressEvent();	
	ClickCount=getSideKeyShortPressCount(0);	//读取按键处理函数传过来的参数
	//挡位记忆参数检查和EEPROM记忆
	if(LastMode==Mode_OFF||LastMode>=ModeTotalDepth)LastMode=Mode_Low;
	//状态机
	IsHalfBrightness=0; //按键灯默认全亮
	switch(CurrentMode->ModeIdx)	
		{
		//出现错误	
		case Mode_Fault:
      IsTacMode=0; //故障后自动取消战术模式			
			if(!IsHoldEvent||IsErrorFatal())break; //用户没有按下按钮或者是致命的错误状态不允许重置
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
			if(ClickCount==1||TKCount) //侧按单击开机进入循环
					{
				  if(Battery>2.9)SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode); //正常开启
					else if(Battery>2.7)SwitchToGear(Mode_Moon);	 //大于2.7V的时候只能开月光
					else LEDMode=LED_RedBlinkFifth; //电池电量严重不足，红色闪五次
					}			
			else if(ClickCount==2)  //双击一键极亮		
					{
					if(IsDisableTurbo)LEDMode=LED_RedBlinkFifth; //手电温度过高锁死极亮，提示无法开启
				  else if(Battery>3.1)SwitchToGear(Mode_Turbo); //电池电量充足正常开启
					else if(Battery>2.7)SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode);  //电池电池电量不足时双击进入普通模式
					else LEDMode=LED_RedBlinkFifth; //电量不足五次闪烁提示
					}
      else if(IsHoldEvent)SwitchToGear(Mode_Moon); //长按开机直接进月光					
			else if(ClickCount==3)//侧按三击进入爆闪 
					{
			    if(Battery>2.7)SwitchToGear(Mode_Strobe);   //进入爆闪
					else LEDMode=LED_RedBlinkFifth; //电量不足五次闪烁提示
					}
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
			 IsHalfBrightness=1; //月光模式按键灯亮度减半
			 BatteryLowAlertProcess(true,Mode_Moon);
		   EnterTurboStrobe(TKCount,0); //尾按模式下，需要可以一键进入极亮或者爆闪所以加上检测
			 if(ClickCount==1)ReturnToOFFState(); //回到关机状态				
			 //电池电压充足，长按进入低亮挡位
		   if((IsHoldEvent||TKCount==1)&&Battery>2.9)  
					{
					SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //长按回到正常挡位模式
					if(TKCount==1)break; //尾按操作直接跳过转换
					if(IsRampEnabled)RestoreToMinimumRampCurrent(); //如果是无极调光则恢复到最低电流
					HoldChangeGearTIM|=0x40; //短时间内禁止长按换挡，确保要用户松开后才能换
					RampCfg.RampMaxDisplayTIM=4; //熄灭0.5秒进行切换
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
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //三击退回到普通模式
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_SOS,TKCount); //长按切换到SOS
		    break;	
    //SOS求救挡位		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    DetectIfNeedsOFF(ClickCount); //执行关机动作检测
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //双击极亮
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //三击退回到普通模式
		    //长按换挡处理
		    SideKeySwitchGearHandler(Mode_Strobe,TKCount); //长按切换到爆闪
		    break;	
		}
  //应用输出电流
	if(DisplayLockedTIM>0)Current=80; //用户进入或者退出锁定，用80mA短暂点亮提示一下
	else if(RampCfg.RampMaxDisplayTIM>0||LowPowerStrobe())Current=-1; //无极调光模式在抵达上下限后短暂熄灭(-1电流表示不关闭防反接FET)
	else switch(CurrentMode->ModeIdx)	
		{
		case Mode_Strobe:Current=StrobeFlag?CurrentMode->Current:-1;break; //爆闪模式根据爆闪flag来回波动
		case Mode_Ramp://无极调光模式取无极调光参数结构体内的电流
			Current=RampCfg.CurrentLimit<RampCfg.Current?RampCfg.CurrentLimit:RampCfg.Current;
		  break;
		case Mode_SOS:Current=SOSFSM();break; //SOS模式，输出电流受SOS状态机调控
		default:Current=CurrentMode->Current;break; //其他挡位使用设置值作为目标电流
		}	
	//清除按键处理
	getSideKeyShortPressCount(1); 
	//ROM挡位记忆控制
	if((IsPOSTKPressed&&TKCount>0)||TailSaveTIM==24) //在挡位停留的时间足够或者开机时按下了挡位开关，保存数据
		{
		IsPOSTKPressed=0; //复位标记为
		TailSaveTIM++;
		TailMemory_Save(CurrentMode->ModeIdx);
		}
	}