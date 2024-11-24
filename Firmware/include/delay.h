#ifndef Delay
#define Delay

//�궨��
//#define EnableMicroSecDelay //�Ƿ�����΢����ʱ
//#define EnableHBCheck //�Ƿ����������

//ϵͳ8Hz������ʱ��ʼ��
extern volatile bit SysHBFlag;
void SetSystemHBTimer(bit IsEnable);

//���������ʱ���Ƿ�����
#ifdef EnableHBCheck
void CheckIfHBTIMIsReady(void);
#endif

//��ʱ��ʼ�����ֺͽϳ�����ʱ
void delay_init();
void delay_ms(int ms);
void delay_sec(int sec);

//΢�뼶�����ʱ
#ifdef EnableMicroSecDelay
void delay_us(int us);
#endif

#endif
