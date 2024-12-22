#ifndef _TC_
#define _TC_

//PI环参数和最小电流限制
#define IntegralCurrentTrimValue 1600 //积分器针对输出的电流修调的最大值(mA)
#define IntegralFactor 20 //积分系数(每单位=1/8秒，越大时间常数越高，6=每分钟进行40mA的调整)
#define MinumumILED 1500 //降档系统所能达到的最低电流(mA)

//温度配置
#define ForceOffTemp 75 //过热关机温度
#define ForceDisableTurboTemp 60 //超过此温度无法进入极亮
#define TurboConstantTemperature 52 //极亮挡位的PID维持温度
#define ConstantTemperature 47 //非极亮挡位温控启动后维持的温度
#define ReleaseTemperature 41 //温控释放的温度

/*   积分器满量程自动定义，切勿修改！    */
#define IntegrateFullScale IntegralCurrentTrimValue*IntegralFactor

#if (IntegrateFullScale > 32000)

#error "Error 001:Invalid Integral Configuration,Trim Value or time-factor out of range!"

#endif

#if (IntegrateFullScale <= 0)

#error "Error 002:Invalid Integral Configuration,Trim Value or time-factor must not be zero or less than zero!"

#endif

/*	温控数值监测，切勿修改！    	*/
#if (ForceOffTemp > 85)
#error "Error 003:Emergency Shutdown Temperature must not exceeded 85 Celsius!"
#endif

#if ((ForceOffTemp-15) < ForceDisableTurboTemp)
#error "Error 004:Force Disble Turbo Temperature must less than Emergency Shutdown Temperature for at least 15 Celsius!"
#endif

#if (ForceOffTemp < (TurboConstantTemperature+8))
#error "Error 005:Force Disble Turbo Temperature must higher than Constant Temperature of Turbo Mode for at least 8 Celsius!"
#endif

#if (TurboConstantTemperature <= ConstantTemperature)
#error "Error 006:Constant Temperature of Turbo Mode must lagger than Constant Temperature of other mode!"
#endif

#if (ConstantTemperature < (ReleaseTemperature+5))
#error "Error 007:Constant Temperature of other mode must lagger than Thermal Control Release Temp for 5 Celsius!"
#endif

#if (ReleaseTemperature < 38)
#error "Error 008:Thermal Control Release Temp is too low and will not release at summer!"
#elif (ReleaseTemperature < 41)
#warning "Warning 001:Thermal Control Release Temp is too low and might not be able to release at summer."
#endif

//函数
int ThermalILIMCalc(void); //根据温控模块计算电流限制
void ThermalCalcProcess(void); //温控PI环处理和过热保护
void RecalcPILoop(int LastCurrent); //换挡的时候重新计算PI环路
void ThermalItgCalc(void);	//温控PI环中I项(积分器)的计算

#endif
