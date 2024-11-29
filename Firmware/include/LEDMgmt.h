#ifndef LEDMgmt_
#define LEDMgmt_

typedef enum
	{
	LED_OFF, //关闭
	LED_Red, //红色常亮
	LED_Amber, //黄色常亮
	LED_Green, //绿色常亮
	LED_RedBlink, //红色闪烁
	LED_RedBlink_Fast, //红色快闪
	LED_RedBlinkFifth, //红色快闪五次
	LED_AmberBlinkFast, //黄色快速闪烁
	LED_GreenBlinkThird, //绿色快闪三次
	LED_RedBlinkThird //红色快闪三次
	}LEDStateDef;

//外部设置index	
extern volatile LEDStateDef LEDMode;
extern xdata int LEDBrightNess;

//LED控制函数
void LED_Init(void);
void LEDControlHandler(void);	
void MakeFastStrobe(LEDStateDef LEDMode);	
	
#endif
