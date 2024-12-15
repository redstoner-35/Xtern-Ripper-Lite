#include "cms8s6990.h"
#include "delay.h"
#include "SideKey.h"
#include "PWMCfg.h"
#include "PinDefs.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "BattDisplay.h"
#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "TailKey.h"

//外部引用
extern volatile int SleepTimer;

//禁用/启用所有系统外设
void SystemPeripheralCTRL(bit IsEnable)
	{
	if(IsEnable)
		{
		ADC_Init(); //初始化ADC
		PWM_Init(); //初始化PWM发生器
		LED_Init(); //初始化侧按LED
		OutputChannel_Init(); //初始化输出通道
		TailKey_Init(); //打开比较器
		return;
		}
	//关闭所有外设
	SetSystemHBTimer(0); //禁用心跳定时器
	PWM_DeInit();
	ADC_DeInit(); //关闭PWM和ADC
	OutputChannel_DeInit(); //关闭输出功能
	}

//睡眠管理函数
void SleepMgmt(void)
	{
	//非关机且仍然在显示电池电压的时候定时器复位禁止睡眠
	if(VshowFSMState!=BattVdis_Waiting||CurrentMode->ModeIdx!=Mode_OFF) 
		{
		SleepTimer=8*SleepTimeOut;		
		return;
		}
	//倒计时
	if(SleepTimer>0)
		{
		SleepTimer--;
		return; //时间未到，继续计时
		}
	//立即进入睡眠阶段
	C0CON0=0; //侧按关机后关闭比较器
	SystemPeripheralCTRL(0);//关闭所有外设
	STOP();  //令STOP=1，使单片机进入睡眠
	//系统已唤醒，立即开始检测
	delay_init();	 //延时函数初始化
	SetSystemHBTimer(1); 
	MarkAsKeyPressed(); //立即标记按键按下
	do	
		{
		delay_ms(1);
		SideKey_LogicHandler(); //处理侧按事务
		//侧按按键的监测定时器处理(使用125mS心跳时钟)
		if(!SysHBFlag)continue; 
		SysHBFlag=0;
		SideKey_TIM_Callback();
		}
	while(!IsKeyEventOccurred()); //等待按键唤醒
	//系统已被唤醒，立即进入工作模式			
	SystemPeripheralCTRL(1);
	//所有外设初始化完毕，启动ADC异步处理模式并打开系统中断
	EnableADCAsync(); 
	IRQ_ALL_ENABLE(); //打开所有中断
	}
