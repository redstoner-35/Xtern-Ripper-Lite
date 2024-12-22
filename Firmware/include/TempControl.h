#ifndef _TC_
#define _TC_

//PI����������С��������
#define IntegralCurrentTrimValue 1600 //�������������ĵ����޵������ֵ(mA)
#define IntegralFactor 20 //����ϵ��(ÿ��λ=1/8�룬Խ��ʱ�䳣��Խ�ߣ�6=ÿ���ӽ���40mA�ĵ���)
#define MinumumILED 1500 //����ϵͳ���ܴﵽ����͵���(mA)

//�¶�����
#define ForceOffTemp 75 //���ȹػ��¶�
#define ForceDisableTurboTemp 60 //�������¶��޷����뼫��
#define TurboConstantTemperature 52 //������λ��PIDά���¶�
#define ConstantTemperature 47 //�Ǽ�����λ�¿�������ά�ֵ��¶�
#define ReleaseTemperature 41 //�¿��ͷŵ��¶�

/*   �������������Զ����壬�����޸ģ�    */
#define IntegrateFullScale IntegralCurrentTrimValue*IntegralFactor

#if (IntegrateFullScale > 32000)

#error "Error 001:Invalid Integral Configuration,Trim Value or time-factor out of range!"

#endif

#if (IntegrateFullScale <= 0)

#error "Error 002:Invalid Integral Configuration,Trim Value or time-factor must not be zero or less than zero!"

#endif

/*	�¿���ֵ��⣬�����޸ģ�    	*/
#if (ForceOffTemp > 85)
#error "Error 003:Emergency Shutdown Temperature must not exceeded 85 Celsius!"
#endif

#if ((ForceOffTemp-15) < ForceDisableTurboTemp)
#error "Error 004:Force Disble Turbo Temperature must less than Emergency Shutdown Temperature for at least 15 Celsius!"
#endif

#if (ForceOffTemp < (TurboConstantTemperature+8))
#error "Error 005:Force Disble Turbo Temperature must higher than Constant Temperature of Turbo Mode for at least 8 Celsius!"
#endif

#if (TurboConstantTemperature <= ConstantTemperature)
#error "Error 006:Constant Temperature of Turbo Mode must lagger than Constant Temperature of other mode!"
#endif

#if (ConstantTemperature < (ReleaseTemperature+5))
#error "Error 007:Constant Temperature of other mode must lagger than Thermal Control Release Temp for 5 Celsius!"
#endif

#if (ReleaseTemperature < 38)
#error "Error 008:Thermal Control Release Temp is too low and will not release at summer!"
#elif (ReleaseTemperature < 41)
#warning "Warning 001:Thermal Control Release Temp is too low and might not be able to release at summer."
#endif

//����
int ThermalILIMCalc(void); //�����¿�ģ������������
void ThermalCalcProcess(void); //�¿�PI������͹��ȱ���
void RecalcPILoop(int LastCurrent); //������ʱ�����¼���PI��·
void ThermalItgCalc(void);	//�¿�PI����I��(������)�ļ���

#endif
