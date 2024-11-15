#include "PinDefs.h"
#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "TailKey.h"
#include "ADCCfg.h"

//外部声明
void SystemPeripheralCTRL(bit IsEnable);//禁用/启用所有系统外设
static xdata char TailKeyCount=0;
xdata char TailKeyTIM=TailKeyRelTime+1;
static xdata bool IsTailKeyPressed=false;

//内部全局
static volatile unsigned char IsEnterLowPowerMode=0xFF;

//比较器中断
void ACMP_IRQHandler(void)  interrupt ACMP_VECTOR 
{
	HShuntSelIOP&=~(0x01<<HShuntSelIOx);
	LShuntSelIOP&=~(0x01<<LShuntSelIOx);
	RevProtIOP&=~(0x01<<RevProtIOx);
	DCDCENIOP&=~(0x01<<DCDCENIOx); //立即拉掉DCDCEN
	RedLEDIOP&=~(0x01<<RedLEDIOx);
  GreenLEDIOP&=~(0x01<<GreenLEDIOx); //令LED立即熄灭
	IsEnterLowPowerMode=0x00;
	//响应结束
	CNIF=0;	
}	

//尾部开关初始化
void TailKey_Init(void)
	{
  C0CON0=0x09; //比较器调节模式禁止，正输入为C0P1，负输入为内部REF	
	C0CON2=0x00; //比较器配置为禁止滤波功能，正输出极性
	C0HYS=0x00; //禁用迟滞
	CNVRCON=0x39; //比较器负向输入的基准电压为内部1.2V带隙基准按照11/20比例分压得到0.66V
	CNFBCON=0x00; //关闭所有比较器的PWM刹车功能
	C0CON0|=0x80; //令C0EN=1，比较器开始运行
	
	//使能中断
	CNIF=0; //打开比较器中断
	EIP1=0x80; //比较器中断必须实时响应所以设置为极高优先级
	CNIE=0x01; //使能比较器中断
	}

//获取尾按按下的次数
char GetTailKeyCount(void)
	{
	char buf;
	if(!IsTailKeyPressed)return 0;
	else 
		{
		buf=TailKeyCount;
		TailKeyCount=0;
		IsTailKeyPressed=false;
		}
	return buf;
	}	

//尾按计时器
void TailKeyCounter(void)
	{
	if(TailKeyTIM<TailKeyRelTime)TailKeyTIM++;
	else if(TailKeyTIM==TailKeyRelTime)
		{
		TailKeyTIM++;
		if(TailKeyCount>0)IsTailKeyPressed=true;
		}
	}	
	
//尾按逻辑处理	
void TailKey_Handler(void)
	{
  if(IsEnterLowPowerMode)return;
	//系统已唤醒，立即开始检测
	do
		{
		delay_ms(5);
		IsEnterLowPowerMode<<=1;
		if(C0CON1&0x80)IsEnterLowPowerMode++;
		}			
	while(IsEnterLowPowerMode!=0xFF);
	TailKeyCount++;
	TailKeyTIM=0;  //尾按按键按下，发生事件复位计时器
	}
