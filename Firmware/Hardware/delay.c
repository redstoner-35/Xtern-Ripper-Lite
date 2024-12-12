#include "cms8s6990.h"
#include "delay.h"
#include "PinDefs.h"
#include "SideKey.h"
#include "GPIO.h"

volatile bit SysHBFlag=0; //系统心跳Flag
volatile bit IsT0OVF=0; //T0已溢出
volatile bit StrobeFlag=0; //爆闪Flag
volatile char HBcounter; //心跳定时器计数

//8Hz定时器初始化
void SetSystemHBTimer(bit IsEnable)
	{
	if(!IsEnable)	
		{
		T2CON=0x00; //禁用T2定时器
		IE&=~0x20; //禁用T2中断
		return;
		}
  //配置定时器模式			
  CCEN=0x00; //关闭比较和捕获
	RLDH=0x0B;
	RLDL=0xDB; //将重装载值设置为产生31.25mS延迟(1/32秒)，计算公式为65535-(48/24(0.5uS)=2000*31.25mS)=3035[0x0BDB]
  TH2=0x5D;
  TL2=0x66; //将计数器设置为产生31.25mS延迟的初值
	//启用中断
  IE|=0x20;   //令ET2=1，启用T2中断
	T2IF=0x00; //清零T2中断
	T2IE=0x80; //令T2OVIE=1，启用T2 OVF中断
	//启动定时器
	HBcounter=4; //对分频计数器进行初始化
	T2CON=0x91; //设置T2时钟源为fSys/24=1MHz，定时器立即启动
	}

#ifdef EnableHBCheck
//检查心跳定时器是否就绪
void CheckIfHBTIMIsReady(void)
	{
	int retry=255;
	SysHBFlag=0;
	do
		{
	  delay_ms(1);
		if(SysHBFlag)return; //定时器已启动，退出
		retry--;
		}
	while(retry);
	//定时器等待超时，点亮红色LED
	while(1); 	
	}
#endif
	
//系统心跳定时器的中断处理	
void Timer2_IRQHandler(void) interrupt TMR2_VECTOR
{ 
	T2IF=0x00; //清零T2中断
  //进行四分频
	HBcounter--;
  StrobeFlag=~StrobeFlag;
	if(HBcounter)return; //时间未到直接退出
	//时间到，复位分频器并置起Flag
	HBcounter=4;
	SysHBFlag=1; //置起flag
}		
	
//软件延时定时器的中断处理
void Timer0_IRQHandler(void) interrupt TMR0_VECTOR  //0x0B 
{
  TCON&=0xEF; //清除溢出标记位
	IsT0OVF=1;
} 	
	
//延时初始化
void delay_init()
	{
	TCON&=0xCF; //清除溢出标记位，关闭定时器
	TMOD&=0xF0;
	TMOD|=0x01; //T0设置为使用Fext,16bit向上计数模式
	TH0=0x00;
	TL0=0x00; //初始化数值
	IE=0x02; //令ET0=1，启用定时中断
	IE|=0x80; //EA=1，启用全局总中断
	}
#ifdef EnableMicroSecDelay
//uS延迟
void delay_us(int us)
	{
	bit IsEA=EA;
	us<<=2; //左移两位,将uS*4得到总周期值
	us=0xFFFF-us; //得到计数器值
	//装载定时器值
	TH0=(us>>8)&0xFF;
	TL0=us&0xFF; 
	IE&=0x7D; //令ET0,EA=0，关闭定时中断和全局总中断开关
	//启动定时器开始倒计时
	TCON|=0x10; //TR0=1,定时器开始计时	
	while(!(TCON&0x20)); //等待直到T0溢出
	//计时结束，复位所有标志位并重新打开中断
	TCON&=0xCF; //清除溢出标记位，关闭定时器
  if(IsEA)IE|=0x82;
	else IE|=0x02;
	}
#endif
//1ms延时
void delay_ms(int ms)
	{
	unsigned long CNT;
	int repcounter=0;
	//计算定时器重装值
	if(ms==0)return;
  do
	  {
		repcounter++; //重复计数器+1
		CNT=(long)ms*4000; //T0一个周期是48/12=4MHz=0.25uS
		CNT/=(long)repcounter; //除以重复次数得到单次计数值
		}
  while(CNT>=65535); //反复循环确保定时器值小于65535
	CNT=0xFFFF-CNT; //计算结束，将16bit计数器满的值加载到定时器内
	//开始进行单次或多次倒计时
	do
		{			
		//装载定时器值
		TH0=(CNT>>8)&0xFF;
	  TL0=CNT&0xFF; 
		IsT0OVF=0; //复位标志位
		//启动定时器开始倒计时
		TCON|=0x10; //TR0=1,定时器开始计时	
		while(!(TCON&0x20)&&!IsT0OVF); //等待直到T0溢出
		//计时结束，准备进行下一轮
		TCON&=0xCF; //清除溢出标记位，关闭定时器
		repcounter--; //重复次数-1
		}
	while(repcounter);
	}
