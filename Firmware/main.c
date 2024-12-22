#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "SideKey.h"
#include "LEDMgmt.h"
#include "ADCCfg.h"
#include "OutputChannel.h"
#include "PWMCfg.h"
#include "BattDisplay.h"
#include "ModeControl.h"
#include "TempControl.h"
#include "TailKey.h"
#include "Strap.h"
#include "SOS.h"
#include "SelfTest.h"

//函数声明
void SleepMgmt(void);

//主函数
void main()
	{
	//时钟初始化
 	delay_init();	 //延时函数初始化
	SetSystemHBTimer(1);//启用系统心跳8Hz定时器	
	//初始化外设
	OutputChannel_Init(); //启动输出通道	
	ADC_Init(); //初始化ADC
	Strap_Init(); //读取驱动的配置电阻
	TailKey_POR_Init(); //尾部初始化和正向开关检测
	PWM_Init(); //启动PWM定时器
	LED_Init(); //初始化侧按LED
	ModeFSMInit(); //初始化挡位状态机
  SideKeyInit(); //侧按初始化	
	TailMemory_Recall(); //获取尾部上次关机前的挡位
	OutputChannel_TestRun(); //输出通道试运行
	DisplayVBattAtStart(); //显示输出电压
	EnableADCAsync(); //启动ADC的异步模式提高处理速度
	IRQ_ALL_ENABLE(); //打开所有中断
	//主循环	
  while(1)
		{
	  //实时处理
		SystemTelemHandler();//获取电池信息	
		SideKey_LogicHandler(); //处理侧按事务
		TailKey_Handler(); //处理尾按事务
		BatteryTelemHandler(); //处理电池遥测
		ModeSwitchFSM(); //挡位状态机
		ThermalCalcProcess(); //温控PI环路计算和过热保护
		OutputChannel_Calc(); //根据电流进行输出通道控制
		PWM_OutputCtrlHandler(); //处理PWM输出事务	
		//8Hz定时处理
		if(!SysHBFlag)continue; //时间没到，跳过处理
		SysHBFlag=0;
		ThermalItgCalc(); //积分器计算
		SideKey_TIM_Callback();//侧按按键的监测定时器处理
		TailKeyCounter(); //计时器
		BattDisplayTIM(); //电池电量显示TIM
		SOSTIMHandler(); //SOS计时器
		ModeFSMTIMHandler(); //模式状态机
		HoldSwitchGearCmdHandler(); //长按换挡
		DisplayErrorTIMHandler(); //故障代码显示
		SleepMgmt(); //休眠管理
		if(TailKeyTIM<TailKeyRelTime)continue;
		LEDControlHandler();//侧按指示LED控制函数
		OutputFaultDetect();//输出故障检测		
		}
	}
