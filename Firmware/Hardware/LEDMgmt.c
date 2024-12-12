#include "delay.h"
#include "LEDMgmt.h"
#include "GPIO.h"
#include "PinDefs.h"
#include "cms8s6990.h"

volatile LEDStateDef LEDMode; 
static char timer=0;
static xdata char StepDownTIM=0; 

sbit RLED=RedLEDIOP^RedLEDIOx;
sbit GLED=GreenLEDIOP^GreenLEDIOx;

//LED配置函数
void LED_Init(void)
	{
	GPIOCfgDef LEDInitCfg;
	//设置结构体
	LEDInitCfg.Mode=GPIO_Out_PP;
  LEDInitCfg.Slew=GPIO_Slow_Slew;		
	LEDInitCfg.DRVCurrent=GPIO_High_Current; //配置为低斜率大电流的推挽输出
	//初始化寄存器
	RLED=0;
	GLED=0;
	//配置GPIO
	GPIO_ConfigGPIOMode(RedLEDIOG,GPIOMask(RedLEDIOx),&LEDInitCfg); //红色LED(推挽输出)
	GPIO_ConfigGPIOMode(GreenLEDIOG,GPIOMask(GreenLEDIOx),&LEDInitCfg); //绿色LED(推挽输出)
	//初始化模式设置
	LEDMode=LED_OFF;
	}
//LED控制函数
void LEDControlHandler(void)
	{
	char buf;
	bit IsLEDON;
	extern bit IsThermalStepDown;
	//短时间熄灭提示降档
	if(IsThermalStepDown)
		{
		if(StepDownTIM<12)StepDownTIM++;
		else
			{
			StepDownTIM=0;
			RLED=0;
			GLED=0; 
			return; //本周期强迫熄灭
			}
		}
	//据目标模式设置LED状态
	switch(LEDMode)
		{
		case LED_OFF:RLED=0;GLED=0;timer=0;break; //LED关闭
		case LED_Amber:RLED=1;GLED=1;break;//黄色LED
		case LED_Green:RLED=0;GLED=1;break;//绿色LED
		case LED_Red:RLED=1;GLED=0;break;//红色LED
		case LED_RedBlink_Fast: //红色快闪	
		case LED_RedBlink: //红色闪烁
			GLED=0;
		  buf=timer&0x7F; //读取当前定时器的控制位
			if(buf<(LEDMode==LED_RedBlink?3:0))
				{
				buf++;
			  timer&=0x80;
				timer|=buf; //时间没到，继续计时
				}
			else timer=timer&0x80?0x00:0x80; //翻转bit 7并重置定时器
			RLED=timer&0x80?1:0; //根据bit 7载入LED控制位
			break;
		case LED_GreenBlinkThird:
		case LED_RedBlinkThird: //LED红色闪烁3次
		case LED_RedBlinkFifth: //LED红色闪烁5次
			timer&=0x7F; //去掉最上面的位
			if(timer>((LEDMode==LED_RedBlinkThird||LEDMode==LED_GreenBlinkThird)?6:10))
				{
				GLED=0; //绿色LED持续关闭
				RLED=0;
				LEDMode=LED_OFF; //时间到，关闭识别
				}
			else //继续计时
				{
				IsLEDON=(timer%2)?1:0; //通过余2判断实现检测
				RLED=LEDMode==LED_GreenBlinkThird?0:IsLEDON;
				GLED=LEDMode==LED_GreenBlinkThird?IsLEDON:0;
				timer++;
				}		
		  break;
		}
	}
	
//制造一次快闪
void MakeFastStrobe(LEDStateDef LEDMode)
	{
	//打开LED
	switch(LEDMode)
		{
		case LED_Green:RLED=0;GLED=1;break;//绿色LED
		case LED_Red:RLED=1;GLED=0;break;//红色LED
		case LED_Amber:RLED=1;GLED=1;break;//黄色LED
		}
	delay_ms(20);
	//关闭LED
  RLED=0;
	GLED=0;
	}	
