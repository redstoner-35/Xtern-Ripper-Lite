#ifndef _ModeControl_
#define _ModeControl_

#include "stdbool.h"

typedef struct
	{
	int Current;
	int BattThres;
	int CurrentLimit;
	char RampMaxDisplayTIM;
	char CfgSavedTIM;
	}RampConfigDef;	
	
typedef enum
	{
	Mode_OFF=0, //关机
	Mode_Fault, //出现错误
		
	Mode_Ramp, //无极调光
  Mode_Moon, //月光
	Mode_Low, //低亮
	Mode_Mid, //中亮
	Mode_MHigh,   //中高亮
	Mode_High,   //高亮
		
	Mode_Turbo, //极亮
  Mode_Strobe, //爆闪		
	Mode_SOS, //SOS挡位
	}ModeIdxDef;
	
typedef struct
	{
  ModeIdxDef ModeIdx;
  int Current; //挡位电流(mA)
	int MinCurrent; //最小电流(mA)，仅无极调光需要
	int LowVoltThres; //低电压检测电压(mV)
	bool IsModeHasMemory; //是否带记忆
	bool IsNeedStepDown; //是否需要降档
	char Offset; //电流值的偏移量(%)
	}ModeStrDef; 

//外部引用
extern ModeStrDef *CurrentMode; //当前模式结构体
extern xdata ModeIdxDef LastMode; //上一个挡位	
extern RampConfigDef RampCfg; //无极调光配置	
extern bit IsRampEnabled; //是否启用无极调光	
	
//参数配置
#define HoldSwitchDelay 6 // 长按换挡延迟	
#define SleepTimeOut 5 //休眠状态延时	
#define ModeTotalDepth 11 //系统一共有几个挡位			
	
//函数
int QueryCurrentGearILED(void); //获取当前挡位的电流值
void ModeFSMTIMHandler(void);//挡位状态机所需的软件定时器处理
void ModeSwitchFSM();//挡位状态机	
void SwitchToGear(ModeIdxDef TargetMode);//换到指定挡位
void ReturnToOFFState(void);//关机	
void HoldSwitchGearCmdHandler(void); //换挡间隔生成	
void ModeFSMInit(void); //初始化状态机	

#endif
