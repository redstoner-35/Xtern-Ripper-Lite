#ifndef _PWM_
#define _PWM_

#define SysFreq 48000000 //ϵͳʱ��Ƶ��(��λHz)
#define PWMFreq 8000 //PWMƵ��(��λHz)	
	
#define PWMStepConstant (SysFreq/PWMFreq)-1 //PWM�����Զ�����
#define iabsf(x) x>0?x:-x //��������ֵ
	
//PWM������ýṹ��
extern xdata float PWMDuty;	
extern bit IsNeedToUploadPWM; //��Ҫ����PWM�Ĵ���Ӧ�����
	
//����
void PWM_Init(void);
void PWM_DeInit(void);
void PWM_OutputCtrlHandler(void);	
void PWM_ForceSetDuty(bit IsEnable);	
	
#endif