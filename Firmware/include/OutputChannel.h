#ifndef _OC_
#define _OC_

//���ͨ����������
#define AUXChannelShuntmOhm 500 //����ͨ���ļ���������ֵ(mR)
#define MainChannelShuntmOhm 1.5 //��ͨ���ļ���������ֵ(mR)


//PWMDAC��������
#define VdivUpResK 100 //�˷ŷ�ѹ���ֵ��϶˵���(K��)
#define PWMDACResK 10 //PWMDAC�ĵ�����ֵ(K��)
#define VdivDownResK 1.5 //�˷ŷ�ѹ���ֵ��¶˵���(K��)
#define LowShuntIOffset 1.00 //�͵���ͨ���ĵ���ƫ��ֵ
#define HighShuntIOffset 1.05 //�ߵ���ͨ���µĵ���ƫ��ֵ
#define HichshuntLowOffset 0.97 //�͵��������µĸߵ���ͨ��ƫ��


#define AUXChannelImax 120 //����ͨ���ĵ�������(mA)
//�ⲿ�ο�
extern xdata volatile int Current; //����ֵ
extern xdata int CurrentBuf; //��ǰ��Ӧ�õĵ���ֵ
extern volatile bit IsDCDCEnabled;

//����
void OutputChannel_Init(void);
void OutputChannel_Calc(void);
void OutputChannel_TestRun(void);

#endif
