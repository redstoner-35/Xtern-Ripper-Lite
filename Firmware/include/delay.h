#ifndef Delay
#define Delay

//系统8Hz心跳定时初始化
extern volatile bit SysHBFlag;
void SetSystemHBTimer(bit IsEnable);
void CheckIfHBTIMIsReady(void);

//延时部分
void delay_init();
void delay_ms(int ms);
void delay_sec(int sec);
void delay_us(int us);

#endif
