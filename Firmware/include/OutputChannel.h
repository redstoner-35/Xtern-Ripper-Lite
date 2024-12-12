#ifndef _OC_
#define _OC_

//输出通道参数设置
#define AUXChannelImax 500 //辅助通道的电流上限(mA)
#define AUXChannelShuntmOhm 500 //辅助通道的检流电阻阻值(mR)
#define MainChannelShuntmOhm 1.5 //主通道的检流电阻阻值(mR)

//PWMDAC参数配置
#define VdivUpResK 100 //运放分压部分的上端电阻(KΩ)
#define PWMDACResK 10 //PWMDAC的电阻阻值(KΩ)
#define VdivDownResK 1.5 //运放分压部分的下端电阻(KΩ)
#define LowShuntIOffset 1.00 //低电流通道的电流偏差值
#define HighShuntIOffset 1.05 //高电流通道下的电流偏差值
#define HichshuntLowOffset 0.97 //低电流条件下的高电流通道偏差

//外部参考
extern xdata volatile int Current; //电流值
extern xdata int CurrentBuf; //当前已应用的电流值
extern volatile bit IsDCDCEnabled;

//函数
void OutputChannel_Init(void);
void OutputChannel_Calc(void);
void OutputChannel_DeInit(void);
void OutputChannel_TestRun(void);

#endif
