#ifndef _OC_
#define _OC_

//���ͨ����������
#define AUXChannelImax 500 //����ͨ���ĵ�������(mA)
#define AUXChannelShuntmOhm 100 //����ͨ���ļ���������ֵ(mR)
#define MainChannelShuntmOhm 3 //��ͨ���ļ���������ֵ(mR)

//PWMDAC��������
#define VdivUpResK 100 //�˷ŷ�ѹ���ֵ��϶˵���(K��)
#define PWMDACResK 10 //PWMDAC�ĵ�����ֵ(K��)
#define VdivDownResK 1.5 //�˷ŷ�ѹ���ֵ��¶˵���(K��)
#define LowShuntIOffset 1.00 //�͵���ͨ���ĵ���ƫ��ֵ
#define HighShuntIOffset 1.07 //�ߵ���ͨ���µĵ���ƫ��ֵ
#define PWMDACSettleDelay 250 //�ڴ�PWMDAC֮��ȴ�DAC��ѹ����ʱ

//�ⲿ�������òο�(mA)
extern xdata int Current;

//����
void OutputChannel_Init(void);
void OutputChannel_Calc(void);
void OutputChannel_DeInit(void);
void OutputChannel_TestRun(void);

#endif
