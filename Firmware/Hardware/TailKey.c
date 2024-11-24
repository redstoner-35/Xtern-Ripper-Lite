#include "PinDefs.h"
#include "cms8s6990.h"
#include "GPIO.h"
#include "delay.h"
#include "TailKey.h"
#include "ADCCfg.h"
#include "PWMCfg.h"

//是否启用驱动的尾按输入
#define EnableMechTailKey

//内部全局变量
static xdata char TailKeyCount=0; //尾部按键按下的次数
static char TailSenTIM=0; //延时启用尾部检测的定时器
static bit IsTailKeyPressed=0;
static volatile unsigned char IsEnterLowPowerMode=0xFF;

//外部引用变量
char TailKeyTIM=TailKeyRelTime+1;

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

//尾部开关所需要的模拟比较器初始化
void TailKey_Init(void)
	{
  C0CON0=0x09; //比较器调节模式禁止，正输入为C0P1，负输入为内部REF	
	C0CON2=0x19; //比较器配置为使能滤波功能，滤波时间常数为256*1/48MHz=5.33uS，正输出极性
	C0HYS=0x00; //禁用迟滞
	CNVRCON=0x38; //比较器负向输入的基准电压为内部1.2V带隙基准按照10/20比例分压得到0.6V
	CNFBCON=0x05; //使能比较器0的刹车功能，在负边沿时禁止PWM输出
	//配置中断参数
	EIP1=0x80; //比较器中断必须实时响应所以设置为极高优先级
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
	//手电开启状态下才进行尾按检测
	#ifdef EnableMechTailKey
	if(CurrentMode->ModeIdx==Mode_OFF||CurrentMode->ModeIdx==Mode_Fault)
		{
	  CNIE=0x00; //关闭比较器中断
	  C0CON0&=0x7F; //令C0EN=0，比较器停止运行
		TailSenTIM=0;			
		}
	else if(TailSenTIM<3)TailSenTIM++;
	else if(TailSenTIM==3)
		{
		C0CON0|=0x80; //令C0EN=1，比较器开始运行
		delay_ms(20); 
		CNIF=0; //延迟20mS后再清除Flag（打开比较器中断前需要清除Flag）
		CNIE=0x01; //使能ACMP0中断，尾按检测激活
		TailSenTIM++;
		}
	#endif
	//尾按开关按下计时
	if(TailKeyTIM<TailKeyRelTime)TailKeyTIM++;
	else if(TailKeyTIM==TailKeyRelTime)
		{
		TailKeyTIM++;
		if(TailKeyCount>0)IsTailKeyPressed=1;
		}
	}	
	
//尾按逻辑处理	
void TailKey_Handler(void)
	{
  if(IsEnterLowPowerMode)return;
	//循环等待按钮重新接通恢复供电
	do
		{
		delay_ms(5);
		IsEnterLowPowerMode<<=1;
		if(C0CON1&0x80)IsEnterLowPowerMode++;
		}			
	while(IsEnterLowPowerMode!=0xFF);
  //恢复供电，进行按键逻辑处理
  PWM_Enable(); //重新使能PWM		
	TailKeyCount++;
	TailKeyTIM=0;  //尾按按键按下，发生事件复位计时器
	}
