#ifndef _LVProt_
#define _LvProt_

#include "stdbool.h"

//��������
#define BatteryAlertDelay 10 //��ؾ����ӳ�	
#define BatteryFaultDelay 2 //��ع���ǿ������/�ػ����ӳ�

//����
void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump); //��ͨ��λ�ľ�������
void RampLowVoltHandler(void); //�޼������ר������
void BattAlertTIMHandler(void); //��ص͵�������������

#endif
