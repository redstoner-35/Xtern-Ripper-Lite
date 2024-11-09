#include "cms8s6990.h"
#include "Watchdog.h"

//看门狗初始化
#pragma OT(0)      //这部分代码对时序敏感需要采用0级别优化
void WDog_Init(void)
	{
	unsigned char buf;
	//设置看门狗溢出时间为48MHz/2^24=0.3495S
	buf=CKCON&0x1F;
	buf|=0xC0;  //令WTS[2:0]=110,选择2^24分频比
	CKCON = buf;
	//启动看门狗
	EA = 0;			//关闭中断
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	WDCON |= 0x02; //令WDTRE=1，看门狗启动
	_nop_();
	EA = 1;  //重新打开中断
	}

//关闭看门狗
#pragma OT(0)      //这部分代码对时序敏感需要采用0级别优化
void WDog_DeInit(void)
	{
	EA = 0;			//关闭中断
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	WDCON &= 0xFD; //令WDTRE=0，看门狗禁止
	_nop_();
	EA = 1;  //重新打开中断
	}
	
//喂狗
#pragma OT(0)      //这部分代码对时序敏感需要采用0级别优化
void WDog_Feed(void)
	{
	EA = 0;			//关闭中断
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	WDCON |= 0x01; //令WDTCLR=1清除计数器
	_nop_();
	EA = 1;  //重新打开中断
	}
	
//获取是否是看门狗导致的复位
#pragma OT(0)      //这部分代码对时序敏感需要采用0级别优化
bit GetIfWDogCauseRST(void)
	{
	unsigned char buf;
	//初始化	
	EA = 0;			//关闭中断
	_nop_();
	TA = 0xAA;
	TA = 0x55;
	buf=WDCON; //读取看门狗寄存器
	_nop_();
	EA = 1;  //重新打开中断
	//返回结果
	return buf&0x04?1:0;
	}
