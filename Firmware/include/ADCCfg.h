#ifndef ADC
#define ADC

#include "stdbool.h"

//�ṹ��
typedef struct
	{
  char Systemp; //ϵͳ�¶�
	float OutputVoltage; //DCDC�����ѹ(V)
	float BatteryVoltage; //��Ч���ڵ�ص�ѹ(V)
	float RawBattVolt; //ԭʼ�ĵ�ص�ѹ(V)
	float MCUVDD; //��Ƭ����VDD
	float FBInjectVolt; //FBע���˷ŵ������ѹ(�����ж��Ƿ����)
	int CfgStrapRes; //���õ�����ֵ
	char VOUTRatio; //���������ѹ֮��İٷֱ�
	bool IsNTCOK; //NTC�Ƿ�OK
	}ADCResultStrDef;

//ADC��׼��ѹ�������׼ͨ������
#define ADCVREF 2.00 //ADCƬ�ڻ�׼LDO�ĵ�ѹ
#define ADC_INTVREFCh 31 //ADC��ͨ��Ƭ�ڴ�϶��׼������ͨ������	
#define ADCBGVREF 1.20 //ADC����ͨ����϶��׼�ĵ�ѹ	
	
//ADC�Ĵ��������궨��	
#define ADC_StartConv() ADCON0|=0x02 //ADC����ת��
#define ADC_GetIfStillConv()	ADCON0&0x02  //���ADC�Ƿ���Ȼ��ת����Ҫ������
#define ADC_ReadConvResult()	(ADRESL|(ADRESH<<8)) //��ȡADCת���ļĴ������
#define ADC_EnableCmd() ADCON1|=0x80  //ʹ��ADC IP
#define ADC_DisableCmd() ADCON1&=0x7F  //�ر�ADC IP	
#define ADC_SetVREFReg(IsVDD) ADCLDO=(!IsVDD?0xA0:0x00) //���û�׼
#define ADC_IsUsingIVREF() ADCLDO&0x80 //���ADC�Ƿ���ʹ��Ƭ�ڻ�׼	
#define ADC_CheckIfChInvalid(Ch) (Ch<0||(Ch>22&&Ch<ADC_INTVREFCh)) //���ͨ�������Ƿ�Ϸ�	
	
//ADC��������궨��
#define EnableADCAsync() IsNotAllowAsync=false
#define DisableADCAsync() IsNotAllowAsync=true

//ADC�ⲿ�ɼ��Ĳ�������
#define VoutUpperResK 470
#define VoutLowerResK 100 //�������ѹ������������
#define VBattUpperResK 680
#define VBattLowerResK 100 //��ؼ���ѹ������������
#define NTCUpperResValueK 330 //NTC���������������ֵ
#define VStrapUpperResValueK 200 //���õ����������ֵ

//��ؼ������
#define VBattAvgCount 300 //��Ч���ڵ�ص�ѹ���ݵ�ƽ������(�����ڲ��߼��ĵ�ѹ����,������ʾ�͵�����������)

//����ֵ��
#define fabs(x) x>0?x:x*-1  //���������ֵ

//�ⲿADC��������
extern ADCResultStrDef Data;
extern xdata bool IsNotAllowAsync; //�Ƿ������첽ת��

//�ⲿ����
void ADC_Init(void);
void ADC_DeInit(void);
void SystemTelemHandler(void);
void BatteryTelemHandler(void);

#endif