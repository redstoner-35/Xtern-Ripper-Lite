#ifndef Delay
#define Delay

//ϵͳ8Hz������ʱ��ʼ��
extern volatile bit SysHBFlag;
void SetSystemHBTimer(bit IsEnable);
void CheckIfHBTIMIsReady(void);

//��ʱ����
void delay_init();
void delay_ms(int ms);
void delay_sec(int sec);
void delay_us(int us);

#endif
