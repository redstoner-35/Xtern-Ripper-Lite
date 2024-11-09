#ifndef _PWM_
#define _PWM_

#define SysFreq 48000000 //系统时钟频率(单位Hz)
#define PWMFreq 8000 //PWM频率(单位Hz)	
	
#define PWMStepConstant (SysFreq/PWMFreq)-1 //PWM周期自动定义
#define iabsf(x) x>0?x:-x //整数绝对值
	
//PWM输出配置结构体
extern xdata float PWMDuty;	
extern bit IsNeedToUploadPWM; //需要更新PWM寄存器应用输出
	
//函数
void PWM_Init(void);
void PWM_DeInit(void);
void PWM_OutputCtrlHandler(void);	
void PWM_ForceSetDuty(bit IsEnable);	
	
#endif
