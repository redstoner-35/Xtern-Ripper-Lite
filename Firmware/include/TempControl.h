#ifndef _TC_
#define _TC_

//PI����������С��������
#define IntegrateFullScale 25000 //���ֵ�Full Scale
#define IntegralFactor 100 //����ϵ��(Խ��ʱ�䳣��Խ��)
#define MinumumILED 1500 //����ϵͳ���ܴﵽ����͵���(mA)

//�¶�����
#define ForceOffTemp 75 //���ȹػ��¶�
#define ForceDisableTurboTemp 60 //�������¶��޷����뼫��
#define ConstantTemperature 52 //�¿�������ά�ֵ��¶�
#define ReleaseTemperature 42 //�¿��ͷŵ��¶�

//����
int ThermalILIMCalc(void); //�����¿�ģ������������
void ThermalCalcProcess(void); //�¿�PI������͹��ȱ���
void RecalcPILoop(int LastCurrent); //������ʱ�����¼���PI��·

#endif
