#ifndef _PWM_
#define _PWM_

//PWM��������
#define SysFreq 48000000 //ϵͳʱ��Ƶ��(��λHz)
#define PWMFreq 6000 //PWMƵ��(��λHz)	
	
//PWM����������	
#define PWMStepConstant (SysFreq/PWMFreq)-1 //PWM�����Զ�����
#define iabsf(x) x>0?x:-x //��������ֵ

//PWMʹ�ܲ���
#define PWM_Enable() 	PWMFBKC=0x00;PWMCNTE=0x0D //ʹ��ͨ��0�ļ�������PWM��ʼ����
	 
	
//PWM������ýṹ��
extern xdata float PWMDuty;	
extern bit IsNeedToUploadPWM; //��Ҫ����PWM�Ĵ���Ӧ�����
	
//����
void PWM_Init(void);
void PWM_DeInit(void);
void PWM_OutputCtrlHandler(void);	
void PWM_ForceSetDuty(bit IsEnable);	
	
#endif
