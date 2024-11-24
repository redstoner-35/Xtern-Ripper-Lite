#ifndef PINDEFS
#define PINDEFS

//�ڲ�����
#include "GPIOCfg.h"


//�����ӿ���MOSFET(P0.1)
#define RevProtIOP GPIO_PORT_0
#define RevProtIOG 0
#define RevProtIOx GPIO_PIN_1 

//DCDCʹ�����(P0.4)
#define DCDCENIOP GPIO_PORT_0
#define DCDCENIOG 0
#define DCDCENIOx GPIO_PIN_4 

//�����̼����������ѡ��(P0.3)
#define LShuntSelIOP GPIO_PORT_0
#define LShuntSelIOG 0
#define LShuntSelIOx GPIO_PIN_3 

//�����̼����������ѡ��(P0.5)
#define HShuntSelIOP GPIO_PORT_0
#define HShuntSelIOG 0
#define HShuntSelIOx GPIO_PIN_5 


//NTC����(P3.1,AN13)
#define NTCInputIOG 3
#define NTCInputIOx GPIO_PIN_1 
#define NTCInputAIN 13

//�����ѹ��������(P1.3,AN6)
#define VOUTFBIOG 1
#define VOUTFBIOx GPIO_PIN_3
#define VOUTFBAIN 6

//�˷ź���״̬��������(P3.2,AN14)
#define OPFBIOG 3
#define OPFBIOx GPIO_PIN_2
#define OPFBAIN 14

//��ص�ѹ�������(P0.0,AN0)
#define VBATInputIOG 0
#define VBATInputIOx GPIO_PIN_0
#define VBATInputAIN 0

//NTC���ʹ������(P3.0)
#define NTCENIOG 3
#define NTCENIOx GPIO_PIN_0

//��ؽ�����������(P2.2,AN8)
#define BATTSELIOG 2
#define BATTSELIOx GPIO_PIN_2
#define BATTSELAIN 8

//��ɫָʾ��(P2.4)	
#define RedLEDIOP GPIO_PORT_2
#define RedLEDIOG 2
#define RedLEDIOx GPIO_PIN_4

//��ɫָʾ��(P2.5)
#define GreenLEDIOP GPIO_PORT_2
#define GreenLEDIOG 2
#define GreenLEDIOx GPIO_PIN_5

//�ఴ����(P2.3)
#define SideKeyGPIOP GPIO_PORT_2
#define SideKeyGPIOG 2
#define SideKeyGPIOx GPIO_PIN_3


//PWMDACͨ��(P0.2)
#define PWMDACIOG 0
#define PWMDACIOx GPIO_PIN_2

#endif
