#ifndef _TC_
#define _TC_

//PI环参数和最小电流限制
#define IntegrateFullScale 25000 //积分的Full Scale
#define IntegralFactor 100 //积分系数(越大时间常数越高)
#define MinumumILED 1500 //降档系统所能达到的最低电流(mA)

//温度配置
#define ForceOffTemp 75 //过热关机温度
#define ForceDisableTurboTemp 60 //超过此温度无法进入极亮
#define ConstantTemperature 52 //温控启动后维持的温度
#define ReleaseTemperature 42 //温控释放的温度

//函数
int ThermalILIMCalc(void); //根据温控模块计算电流限制
void ThermalCalcProcess(void); //温控PI环处理和过热保护
void RecalcPILoop(int LastCurrent); //换挡的时候重新计算PI环路

#endif
