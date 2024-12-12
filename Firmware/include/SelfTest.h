#ifndef _SelfTest_
#define _SelfTest_

#include "LEDMgmt.h"

//参数
#define FaultBlankingInterval 4

//错误类型枚举
typedef enum
	{
	Fault_None,    //没有错误发生
	Fault_DCDCFailedToStart, //DCDC无法启动 ID:1
	Fault_DCDCENOOC, //DCDC使能信号不受控 ID:2
	Fault_DCDCShort, //DCDC输出短路  ID:3
	Fault_InputOVP, //输入过压保护 ID:4
	Fault_DCDCOpen,  //LED开路 ID:5
	Fault_NTCFailed, //NTC故障 ID:6
	Fault_OverHeat, //过热故障 ID:7
	Fault_StrapResistorError, //配置电阻开路或阻值异常 ID:8
	Fault_StrapMismatch //配置电阻的LED类型和实际不符 ID:9
	}FaultCodeDef;	

//外部引用
extern xdata FaultCodeDef ErrCode; //错误代码
	
//函数
void ReportError(FaultCodeDef Code); //报告错误
void DisplayErrorTIMHandler(void); //显示错误时候用到的计时器处理
void DisplayErrorIDHandler(void); //根据错误ID进行显示的处理
void OutputFaultDetect(void); //输出故障监测函数	
bit IsErrorFatal(void);	//查询错误是否致命
	
#endif
