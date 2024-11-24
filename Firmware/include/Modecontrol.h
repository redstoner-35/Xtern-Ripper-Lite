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
	Fault_None,    //没有错误发生
	Fault_DCDCFailedToStart, //DCDC无法启动 ID:1
	Fault_DCDCENOOC, //DCDC使能信号不受控 ID:2
	Fault_DCDCShort, //DCDC输出短路  ID:3
	Fault_DCDCOpen,  //LED开路 ID:4
	Fault_NTCFailed, //NTC故障 ID:5
	Fault_OverHeat, //过热故障 ID:6
	Fault_MPUHang, //单片机死机导致看门狗触发复位 ID:7
	Fault_StrapResistorError, //配置电阻开路或阻值异常 ID:8
	Fault_StrapMismatch, //配置电阻的LED类型和实际不符 ID:9
	}FaultCodeDef;	
	
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

typedef enum
	{
	SOSState_Prepare,
	SOSState_3Dot,
	SOSState_3DotWait,
	SOSState_3Dash,
	SOSState_3DashWait,
	SOSState_3DotAgain,
	SOSState_Wait,
	}SOSStateDef;	
	
typedef struct
	{
  ModeIdxDef ModeIdx;
  int Current; //挡位电流(mA)
	int MinCurrent; //最小电流(mA)，仅无极调光需要
	int LowVoltThres; //低电压检测电压(mV)
	bool IsModeHasMemory; //是否带记忆
	bool IsNeedStepDown; //是否需要降档
	}ModeStrDef; 

//外部引用
extern ModeStrDef *CurrentMode; //当前模式结构体
extern xdata ModeIdxDef LastMode; //上一个挡位	
extern RampConfigDef RampCfg; //无极调光配置	
extern bit IsRampEnabled; //是否启用无极调光	
extern xdata FaultCodeDef ErrCode; //错误代码
	
//参数配置
#define BatteryAlertDelay 9 //电池警报延迟	
#define HoldSwitchDelay 6 // 长按换挡延迟	
#define SleepTimeOut 5 //休眠状态延时	
#define ModeTotalDepth 11 //系统一共有几个挡位			
#define SOSDotTime 2 //SOS信号(.)的时间	
#define SOSDashTime 6 //SOS信号(-)的时间	
#define SOSGapTime 7 //SOS信号在每次显示途中等待的时间
#define SOSFinishGapTime 35 //每轮SOS发出结束后的等待时间
	
//函数
void ModeFSMTIMHandler(void);//挡位状态机所需的软件定时器处理
void ModeSwitchFSM();//挡位状态机	
int SwitchToGear(ModeIdxDef TargetMode);//换到指定挡位
void ReturnToOFFState(void);//关机	
void HoldSwitchGearCmdHandler(void); //换挡间隔生成	
void ModeFSMInit(void); //初始化状态机	
void ReportError(FaultCodeDef Code); //报告错误	
	
#endif
