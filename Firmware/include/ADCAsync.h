#ifndef _ADAsync_
#define _ADAsync_

//ADC�첽�������ö��ֵ
typedef enum
	{
	ADC_SubmitQueue, //�ύת������	
  ADC_SubmitChFromQueue, //��ADCת���߳��ύ�����ڵ�����
	ADC_WaitMissionDone, //�ȴ��������
	ADC_ConvertComplete //ת�����	
	}ADCAsyncStateDef; //ADC�첽ת��״̬������

typedef struct
	{
	long avgbuf;
	int Count;
	char Ch;
	bool IsMissionProcessing; //�Ƿ����ڴ�������
	}ADCConvertTemp;

//ADC�첽��������
#define ADCConvertQueueDepth 6 //ADCת������������	
#define ADCAverageCount 10 //ADC����ÿ��ת�������ƽ������	

//ADCת������Ķ�������
#include "Pindefs.h"
#include "ADCCfg.h"	
	
code char ADCChQueue[ADCConvertQueueDepth]=
	{
	ADC_INTVREFCh, //��ת��VREF
	NTCInputAIN,//Ȼ��ת���¶�	
	BATTSELAIN, //ת������strap
	OPFBAIN, //FBע������˷ŵ������ѹ
	VBATInputAIN, //��ص�ѹ
	VOUTFBAIN //���ת�������ѹ
	};
	
#endif
