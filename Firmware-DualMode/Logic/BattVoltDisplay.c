#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "delay.h"
#include "ModeControl.h"
#include "SideKey.h"
#include "cms8s6990.h"
#include "BattDisplay.h"
#include "PinDefs.h"

//平均计算结构体
typedef struct
	{
	int Min;
  int Max;
	long AvgBuf;
	int Count;
	}AverageCalcDef;	
	
//函数声明
void DisplayErrorIDHandler(void);
	
	
//内部全局变量
static xdata char BattShowTimer=0; //电池电量显示命令
xdata BattStatusDef BattState; //电池电量标记位
static xdata AverageCalcDef BattVolt;	
xdata float Battery; //等效单节电池电压
xdata char VbattCellCount=3; //系统的电池节数
bit IsBatteryAlert; //电池电压低于警告值	
bit IsBatteryFault; //电池电压低于保护值		
static xdata int VshowTIM=0;
static xdata float VbattSample; //取样的电池电压
xdata BattVshowFSMDef VshowFSMState; //电池电压显示所需的计时器和状态机转移

//内部sbit
sbit CSPin=BATTSELIOP^BATTSELIOx;	
	
//启动电池电压显示
void TriggerVshowDisplay(void)	
	{
	if(VshowFSMState!=BattVdis_Waiting)return; //非等待显示状态禁止操作
	VshowFSMState=BattVdis_PrepareDis;
	}		
	
//电池详细电压显示处理
static void BatVshowFSM(void)
	{
	int buf;
	//电量显示状态机
	switch(VshowFSMState)
		{
		case BattVdis_Waiting:break; //等待显示阶段
		case BattVdis_PrepareDis: //准备显示
			LEDMode=LED_OFF; //关闭LED
			VbattSample=Data.RawBattVolt; //进行电压取样
	    VshowTIM=14; //延迟1.75秒
			VshowFSMState=BattVdis_DelayBeforeDisplay; //进行简单的延迟
		//延迟并显示开头
		case BattVdis_DelayBeforeDisplay:
			if(VshowTIM>12)LEDMode=LED_Green;
      else if(VshowTIM>10)LEDMode=LED_Amber;		
		  else if(VshowTIM>8)LEDMode=LED_Red;	
		  else LEDMode=LED_OFF; //红绿蓝闪烁之后等待
		  //头部显示结束后开始正式显示电压
		  if(VshowTIM>0)break; //时间未到
		  buf=(int)VbattSample%100; //去除掉99以上的
		  buf/=10; //显示十位
		  if(buf==0)VshowTIM=-1; //0=瞬间闪一下
			else VshowTIM=(4*buf)-1; //配置显示的时长
		  VshowFSMState=BattVdis_Show10V; //跳转到10V显示
		  break;
    //显示十位
		case BattVdis_Show10V:
			if(VshowTIM==-1)//通过快闪一次表示是0
				{
				MakeFastStrobe(LED_Red);
				VshowTIM=0; 
				}
		  buf=VshowTIM%4;
			LEDMode=buf>1?LED_Red:LED_OFF; //制造红色闪烁指示10V状态
		  if(VshowTIM<=0) //显示结束
				{
				LEDMode=LED_OFF;
				VshowTIM=10;
				VshowFSMState=BattVdis_Gap10to1V; //等待一会
				}
		  break;
		//十位和个位之间的间隔
		case BattVdis_Gap10to1V:
			if(VshowTIM>0)break; //时间未到
		  buf=(int)VbattSample; 
			buf%=10; //显示个位
			if(buf==0)VshowTIM=-1; //0=瞬间闪一下
			else VshowTIM=(4*buf)-1; //配置显示的时长
			VshowFSMState=BattVdis_Show1V; //跳转到1V显示	
			break;	
		//显示个位
		case BattVdis_Show1V:
		  if(VshowTIM==-1)//通过快闪一次表示是0
				{
				MakeFastStrobe(LED_Amber);
				VshowTIM=0; 
				}
			buf=VshowTIM%4;
			LEDMode=buf>1?LED_Amber:LED_OFF; //制造红色闪烁指示1V状态
			if(VshowTIM<=0) //显示结束
				{
				LEDMode=LED_OFF;
				VshowTIM=10;
				VshowFSMState=BattVdis_Gap1to0_1V; //等待一会
				}
		  break;
		//个位和十分位之间的间隔		
		case BattVdis_Gap1to0_1V:	
			  if(VshowTIM>0)break; //时间未到
				VbattSample*=(float)10;
				buf=(int)VbattSample; //乘10将小数点后一位缩放为个位 
				buf%=10; //得到十分位状态
				if(buf==0)VshowTIM=-1; //0=瞬间闪一下
				else VshowTIM=(4*buf)-1; //配置显示的时长
				VshowFSMState=BattVdis_Show0_1V; //跳转到0.1V显示
				break;
		//显示小数点后一位(0.1V)
		case BattVdis_Show0_1V:
		  if(VshowTIM==-1)//通过快闪一次表示是0
				{
				MakeFastStrobe(LED_Green);
				VshowTIM=0; 
				}
			buf=VshowTIM%4;
			LEDMode=buf>1?LED_Green:LED_OFF; //制造绿色闪烁指示0.1V状态
		  if(VshowTIM<=0) //显示结束
				{
				LEDMode=LED_OFF;
				VshowTIM=12; 
				VshowFSMState=BattVdis_WaitShowChargeLvl; //等待1秒后开始显示电量
				}
			break;
		//等待一段时间后显示当前电量
		case BattVdis_WaitShowChargeLvl:
			if(VshowTIM>0)break;
		  BattShowTimer=0x80; //启动总体电量显示
		  VshowFSMState=BattVdis_ShowChargeLvl; //等待电量显示状态结束
      break;
	  //等待总体电量显示结束
		case BattVdis_ShowChargeLvl:
			if(BattShowTimer&0x80||getSideKeyClickAndHoldEvent())break; //用户仍然按下按键，等待用户松开
			VshowFSMState=BattVdis_Waiting; //显示结束，退回到等待阶段
      break;
		}
	}

//在启动时显示电池电压
void DisplayVBattAtStart(void)
	{
	int i;
	#ifdef EnableStrapConfig
	GPIOCfgDef CSInitCfg;
	//检测电池节数并刷新等效单节电池的电压
	CSInitCfg.Mode=GPIO_IPU;
  CSInitCfg.Slew=GPIO_Slow_Slew;		
	CSInitCfg.DRVCurrent=GPIO_High_Current; //配置为上拉输入
	GPIO_SetMUXMode(BATTSELIOG,BATTSELIOx,GPIO_AF_GPIO);
	GPIO_ConfigGPIOMode(BATTSELIOG,GPIOMask(BATTSELIOx),&CSInitCfg); //配置为上拉输入
	delay_ms(5);	
	VbattCellCount=CSPin?3:2; //根据外部strap的状态选择电池节数
	CSPin=0;	
	CSInitCfg.Mode=GPIO_Out_PP;	
	GPIO_ConfigGPIOMode(BATTSELIOG,GPIOMask(BATTSELIOx),&CSInitCfg); //检测完毕，配置为推挽输出	
	#else
	VbattCellCount=ManualCellCount; //手动指定CELL数目，无视Strap配置
	#endif
	//提前更新电池电量状态
	SystemTelemHandler();
	if(Data.BatteryVoltage<2.9)BattState=Battery_VeryLow; //电池电压低于2.8，直接报告严重不足
	else if(Data.BatteryVoltage<3.2)BattState=Battery_Low; //电池电压低于3.2则切换到电量低的状态
	else if(Data.BatteryVoltage<3.6)BattState=Battery_Mid; //电池电量低于3.5则表示为中等
	else BattState=Battery_Plenty; //电量充足
	//清除电池故障和警告位	
	IsBatteryAlert=0;
	IsBatteryFault=0;
	//初始化平均值缓存
	BattVolt.Min=32766;
	BattVolt.Max=-32766; //复位最大最小捕获器
	BattVolt.Count=0;
  BattVolt.AvgBuf=0; //清除平均计数器和缓存
	Battery=Data.BatteryVoltage; //更新电池电压
  //复位电池电压显示状态机		
	VbattSample=Data.RawBattVolt; 
	VshowFSMState=BattVdis_Waiting; //显示状态设置为等待
	//启动电池电量显示(仅无错误且上次断电之前是关机状态的情况下)
	if(CurrentMode->ModeIdx==Mode_OFF)
		{
	  for(i=0;i<VbattCellCount;i++)
			 {
			 MakeFastStrobe(LED_Amber);
			 delay_ms(160);
			 }
		delay_ms(400);
	  BattShowTimer=0x80;
		}
	}
//电池电量显示延时的处理
void BattDisplayTIM(void)
	{
	char buf;
	//电池电压显示的计时器处理	
	if(VshowTIM>0)VshowTIM--;
	//电池显示定时器
	if(BattShowTimer&0x80)	
		{
		buf=BattShowTimer&0x7F; //取出TIM值
		BattShowTimer&=0x80; //去除掉原始的TIM值
		if(buf<12)
			{
			buf++;
			BattShowTimer|=buf; //把数值写回去
			}
		else BattShowTimer=0; //定时器到时间了自动停止
		}
	else BattShowTimer=0; //清除buf		
	}	
	
//电池参数测量和指示灯控制
void BatteryTelemHandler(void)
	{
	float AlertThr;
	long buf;
	//电量平均模块计算
  if(BattVolt.Count<VBattAvgCount)		
		{
		buf=(long)(Data.BatteryVoltage*1000);
		BattVolt.Count++;
		BattVolt.AvgBuf+=buf;
		if(BattVolt.Min>buf)BattVolt.Min=buf;
		if(BattVolt.Max<buf)BattVolt.Max=buf; //极值读取
		}
	else //平均次数到，更新电压
		{
		BattVolt.AvgBuf-=(long)BattVolt.Min+(long)BattVolt.Max; //去掉最高最低
		BattVolt.AvgBuf/=(long)(BattVolt.Count-2); //求平均值
		Battery=(float)BattVolt.AvgBuf/(float)1000; //得到最终的电池电压
		BattVolt.Min=32766;
		BattVolt.Max=-32766; //复位最大最小捕获器
		BattVolt.Count=0;
		BattVolt.AvgBuf=0; //清除平均计数器和缓存		
		}
	//根据电池电压控制flag实现低电压降档和关机保护
	if(CurrentMode->LowVoltThres==0) //当前挡位电池电压警报关闭
		 {
		 IsBatteryAlert=0;
		 IsBatteryFault=0; 
		 }
	else //正常进行警报
		 {
		 if(CurrentMode->ModeIdx==Mode_Ramp)AlertThr=(float)RampCfg.BattThres/(float)1000; //无极调光模式下，使用结构体内的动态阈值
		 else AlertThr=(float)(CurrentMode->LowVoltThres)/(float)1000; //从当前目标挡位读取模式值  
		 if((Data.OutputVoltage/Data.RawBattVolt)>0.87)IsBatteryAlert=1; //输出输入比值大于86%，DCDC芯片已经饱和，强制降档
		 else IsBatteryAlert=Battery>AlertThr?0:1; //警报bit
		 IsBatteryFault=Battery>2.7?0:1; //故障bit
		 if(IsBatteryFault)IsBatteryAlert=0; //故障bit置起后强制清除警报bit
		 }
	//电池电量指示状态机
	switch(BattState) 
		 {
		 //电池电量充足
		 case Battery_Plenty: 
				if(Battery<3.6)BattState=Battery_Mid; //电池电压小于3.7，回到电量较低状态
			  break;
		 //电池电量较为充足
		 case Battery_Mid:
			  if(Battery>3.8)BattState=Battery_Plenty; //电池电压大于3.8，回到充足状态
				if(Battery<3.2)BattState=Battery_Low; //电池电压低于3.2则切换到电量低的状态
		    if(Battery<2.8)BattState=Battery_VeryLow; //电池电压低于2.8，直接报告严重不足
				break;
		 //电池电量不足
		 case Battery_Low:
			  if(Battery>3.85)BattState=Battery_Plenty; //电池电压大于3.8，回到充足状态
		    if(Battery>3.5)BattState=Battery_Plenty; //电池电压高于3.5，切换到电量充足的状态
			  if(Battery<2.9)BattState=Battery_VeryLow; //电池电压低于2.8，报告严重不足
		    break;
		 //电池电量严重不足
		 case Battery_VeryLow:
			  if(Battery>3.5)BattState=Battery_Plenty; //电池电压大于3.5，直接跳到充足
			  if(Battery>3.0)BattState=Battery_Low; //电池电压回升到3.0，跳转到电量不足阶段
		    break;
		 }
	//LED控制
	if(LEDMode==LED_RedBlinkFifth||LEDMode==LED_GreenBlinkThird||LEDMode==LED_RedBlinkThird)return; //频闪指示下不执行控制 
	if(ErrCode!=Fault_None)DisplayErrorIDHandler(); //有故障发生，显示错误
	else if(CurrentMode->ModeIdx!=Mode_OFF||BattShowTimer&0x80)switch(BattState) //用户长按按键查询电量或者手电开机，指示电量
		 {
		 case Battery_Plenty:LEDMode=LED_Green;break; //电池电量充足绿色常亮
		 case Battery_Mid:LEDMode=LED_Amber;break; //电池电量中等黄色常亮
		 case Battery_Low:LEDMode=LED_Red;break;//电池电量不足
		 case Battery_VeryLow:LEDMode=LED_RedBlink;break; //电池电量严重不足红色慢闪
		 }
	else if(VshowFSMState!=BattVdis_Waiting)BatVshowFSM();//电池电压显示启动，执行状态机	
  else LEDMode=LED_OFF; //手电处于关闭状态，且没有按键按下的动静，故LED设置为关闭
	}
	