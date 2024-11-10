#include "delay.h"
#include "LEDMgmt.h"
#include "GPIO.h"
#include "PinDefs.h"
#include "cms8s6990.h"

volatile LEDStateDef LEDMode; 
static char timer=0;
static xdata char StepDownTIM; 
xdata int LEDBrightNess; //LED亮度占空比数据

sbit RedLED=RedLEDIOP^RedLEDIOx;
sbit GreenLED=GreenLEDIOP^GreenLEDIOx;

//LED配置函数
void LED_Init(void)
	{
	GPIOCfgDef LEDInitCfg;
	//设置结构体
	LEDInitCfg.Mode=GPIO_Out_PP;
  LEDInitCfg.Slew=GPIO_Slow_Slew;		
	LEDInitCfg.DRVCurrent=GPIO_High_Current; //配置为低斜率大电流的推挽输出
	//初始化寄存器
	RedLED=0;
	GreenLED=0;
	//配置GPIO
	GPIO_ConfigGPIOMode(RedLEDIOG,GPIOMask(RedLEDIOx),&LEDInitCfg); //红色LED(推挽输出)
	GPIO_ConfigGPIOMode(GreenLEDIOG,GPIOMask(GreenLEDIOx),&LEDInitCfg); //绿色LED(推挽输出)
	//初始化模式设置
	LEDMode=LED_OFF;
	}

//LED管理器切换到PWM模式
void LEDMgmt_SwitchToPWM(void)
	{
	//启用复用功能
  GPIO_SetMUXMode(RedLEDIOG,RedLEDIOx,GPIO_AF_PWMCH2);
	GPIO_SetMUXMode(GreenLEDIOG,GreenLEDIOx,GPIO_AF_PWMCH3);
	//配置PWM发生器
	PWMOE|=0x0C; //打开PWM输出通道2 3
	PWM23PSC=0x01;  //打开预分频器和计数器时钟 
  PWM2DIV=0xff;   
	PWM3DIV=0xff;   //令Fpwmcnt=Fsys=48MHz(不分频)	
	PWMCNTM|=0x0C; //通道2 3配置为自动加载模式
	PWMCNTCLR=0x0C; //初始化PWM的时候复位通道2和3定时器
	PWMMASKE|=0x0C; //PWM掩码功能启用禁止通道2 3输出
	//配置周期数据
	PWMP2H=0x09;
	PWMP2L=0x5F; 
	PWMP3H=0x09;
	PWMP3L=0x5F; //CNT=(48MHz/20Khz)-1=2399
	//配置占空比数据
	if(LEDBrightNess>2399)LEDBrightNess=2399;
	if(LEDBrightNess<50)LEDBrightNess=50; //限制传入的占空比数据范围
	PWMD2H=(LEDBrightNess>>8)&0xFF;
	PWMD2L=LEDBrightNess&0xFF; 
	PWMD3H=(LEDBrightNess>>8)&0xFF;
	PWMD3L=LEDBrightNess&0xFF; 
	//启用PWM
	PWMCNTE|=0x0C; //使能通道0的计数器，PWM开始运作
	PWMLOADEN|=0x0C; //加载通道0的PWM值
	while(LEDMgmt_WaitSubmitDuty()); //等待加载结束
	}	

//调试模式，是否使能降档提示
#define EnableStepDownInfo	
	
//设置LED开关	
static void SetLEDONOFF(bit RLED,bit GLED)
	{
	unsigned char buf;
	//非PWM模式直接设置对应SFR
	if(!(PWMCNTE&0x0C))
		{
		RedLED=RLED;
		GreenLED=GLED;
		}
	//PWM模式设置输出mask寄存器
	else
		{
		buf=PWMMASKE;
		if(RLED)buf&=0xFB;
		else buf|=0x04; //控制PWM通道2是否正常输出来打开关闭红色LED
		if(GLED)buf&=0xF7;
		else buf|=0x08; //控制PWM通道3是否正常输出来打开关闭绿色LED
		PWMMASKE=buf;
		}
	}	
	
//LED管理器实时设置亮度	
void LEDMgmt_SetBrightness(void)
	{
	//亮度限幅
	if(LEDBrightNess>2399)LEDBrightNess=2399;
	if(LEDBrightNess<50)LEDBrightNess=50;
	//设置寄存器
	PWMD2H=(LEDBrightNess>>8)&0xFF;
	PWMD2L=LEDBrightNess&0xFF; 
	PWMD3H=(LEDBrightNess>>8)&0xFF;
	PWMD3L=LEDBrightNess&0xFF; 
	//编程参数值
	PWMLOADEN|=0x0C; //加载对应通道的PWM值
	}
	
//LED控制函数
void LEDControlHandler(void)
	{
	char buf;
	bit IsLEDON,RLED,GLED;
	#ifdef EnableStepDownInfo
	extern bit IsTempLIMActive;
	//降档之后每隔一段时间闪一下侧按	
	if(!IsTempLIMActive)StepDownTIM=0;
	else if(StepDownTIM>8)
		 {
	   RLED=0;
		 GLED=0;
		 timer=0;	 
		 StepDownTIM=0;
		 return;
		 }	
	else StepDownTIM++;
	#endif
	//据目标模式设置LED状态
	switch(LEDMode)
		{
		case LED_OFF:RLED=0;GLED=0;timer=0;break; //LED关闭
		case LED_Green:RLED=0;GLED=1;break;//绿色LED
		case LED_Red:RLED=1;GLED=0;break;//红色LED
		case LED_Amber:RLED=1;GLED=1;break;//黄色LED
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
		case LED_RedBlinkThird: //LED红色闪烁3次
		case LED_RedBlinkFifth: //LED红色闪烁5次
			GLED=0; //绿色LED持续关闭
			timer&=0x7F; //去掉最上面的位
			if(timer>(LEDMode==LED_RedBlinkThird?6:10))
				{
				RLED=0;
				LEDMode=LED_OFF; //时间到，关闭识别
				}
			else //继续计时
				{
				IsLEDON=(timer%2)?1:0; //通过余2判断实现检测
				RLED=IsLEDON;
				timer++;
				}		
		  break;
		case LED_GreenBlinkThird: //LED绿色闪烁3次
			RLED=0; //红色LED持续关闭
			timer&=0x7F; //去掉最上面的位
			if(timer>6)
				{
				GLED=0;
				LEDMode=LED_OFF; //时间到，关闭识别
				}
			else //继续计时
				{
				IsLEDON=(timer%2)?1:0; //通过余2判断实现检测
				GLED=IsLEDON;
				timer++;
				}		
		  break;
		}
	//LED运算完毕，提交到寄存器控制亮灭
	SetLEDONOFF(RLED,GLED);
	}
	
//制造一次快闪
void MakeFastStrobe(LEDStateDef LEDMode)
	{
	bit RLED=0,GLED=0;
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
