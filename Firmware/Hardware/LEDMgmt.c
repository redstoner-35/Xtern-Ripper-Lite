#include "delay.h"
#include "LEDMgmt.h"
#include "GPIO.h"
#include "PinDefs.h"
#include "cms8s6990.h"

volatile LEDStateDef LEDMode; 
static char timer;
bit IsHalfBrightness;
static xdata char StepDownTIM; 

static void SetLEDBrightNess(void)
	{
	//设置占空比寄存器
	PWMD2H=IsHalfBrightness?(LEDBrightnessHalf>>8)&0xFF:(LEDBrightnessFull>>8)&0xFF;
	PWMD2L=IsHalfBrightness?LEDBrightnessHalf&0xFF:LEDBrightnessFull&0xFF; 
	PWMD3H=IsHalfBrightness?(LEDBrightnessHalf>>8)&0xFF:(LEDBrightnessFull>>8)&0xFF;
	PWMD3L=IsHalfBrightness?LEDBrightnessHalf&0xFF:LEDBrightnessFull&0xFF;  
  //应用占空比
	PWMLOADEN|=0x0C; //加载通道0的PWM值
	}

//LED配置函数
void LED_Init(void)
	{
	GPIOCfgDef LEDInitCfg;
	//设置结构体
	LEDInitCfg.Mode=GPIO_Out_PP;
  LEDInitCfg.Slew=GPIO_Slow_Slew;		
	LEDInitCfg.DRVCurrent=GPIO_High_Current; //配置为低斜率大电流的推挽输出
	//初始化模式设置
	StepDownTIM=0;
	LEDMode=LED_OFF;
	//配置PWM发生器
	PWM23PSC=0x01;  //打开预分频器和计数器时钟 
  PWM2DIV=0xff;   
	PWM3DIV=0xff;   //令Fpwmcnt=Fsys=48MHz(不分频)	
	//配置周期数据
	PWMP2H=0x09;
	PWMP2L=0x5F; 
	PWMP3H=0x09;
	PWMP3L=0x5F; //CNT=(48MHz/20Khz)-1=2399
  //启用PWM
	PWMCNTE|=0x0C; //使能通道2 3的计数器，PWM开始运作
	//配置占空比数据
	SetLEDBrightNess();
  while(PWMLOADEN&0x0C); //等待PWM完成应用
	//配置GPIO（将配置GPIO拉到最后是因为避免PWM发生器工作异常引起闪烁）
	GPIO_ConfigGPIOMode(RedLEDIOG,GPIOMask(RedLEDIOx),&LEDInitCfg); //红色LED(推挽输出)
	GPIO_ConfigGPIOMode(GreenLEDIOG,GPIOMask(GreenLEDIOx),&LEDInitCfg); //绿色LED(推挽输出)
	GPIO_SetMUXMode(RedLEDIOG,RedLEDIOx,GPIO_AF_PWMCH2);
	GPIO_SetMUXMode(GreenLEDIOG,GreenLEDIOx,GPIO_AF_PWMCH3); //为了控制侧按LED的亮度改为PWM模式
	}	
	
//设置LED亮度	
static void SetLEDONOFF(bit RLED,bit GLED)
	{
	unsigned char buf;
  buf=PWMMASKE;
	if(RLED)buf&=0xFB;
	else buf|=0x04; //控制PWM通道2是否正常输出来打开关闭红色LED
	if(GLED)buf&=0xF7;
	else buf|=0x08; //控制PWM通道3是否正常输出来打开关闭绿色LED
	PWMMASKE=buf;
	}		
	
//LED控制函数
void LEDControlHandler(void)
	{
	char buf;
	bit IsLEDON,RLED,GLED;
	extern bit IsThermalStepDown;
	//短时间熄灭提示降档
	if(IsThermalStepDown)
		{
		if(StepDownTIM<12)StepDownTIM++; //时间没到，继续累加时间
			else //时间到，本周期侧按LED强制熄灭
			{
			StepDownTIM=0;
			SetLEDONOFF(0,0); 
			return;
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
	//设置LED亮度
  SetLEDBrightNess();
	SetLEDONOFF(RLED,GLED);
	}
	
//制造一次快闪
void MakeFastStrobe(LEDStateDef LEDMode)
	{
	bit RLED,GLED;
	//打开LED
	switch(LEDMode)
		{
		case LED_Green:RLED=0;GLED=1;break;//绿色LED
		case LED_Red:RLED=1;GLED=0;break;//红色LED
		case LED_Amber:RLED=1;GLED=1;break;//黄色LED
		}
	SetLEDONOFF(RLED,GLED);
	delay_ms(20);
	//关闭LED
	SetLEDONOFF(0,0);
	}	
