#ifndef _SelfTest_
#define _SelfTest_

#include "LEDMgmt.h"

//����
#define FaultBlankingInterval 4

//��������ö��
typedef enum
	{
	Fault_None,    //û�д�����
	Fault_DCDCFailedToStart, //DCDC�޷����� ID:1
	Fault_DCDCENOOC, //DCDCʹ���źŲ��ܿ� ID:2
	Fault_DCDCShort, //DCDC�����·  ID:3
	Fault_InputOVP, //�����ѹ���� ID:4
	Fault_DCDCOpen,  //LED��· ID:5
	Fault_NTCFailed, //NTC���� ID:6
	Fault_OverHeat, //���ȹ��� ID:7
	Fault_StrapResistorError, //���õ��迪·����ֵ�쳣 ID:8
	Fault_StrapMismatch //���õ����LED���ͺ�ʵ�ʲ��� ID:9
	}FaultCodeDef;	

//�ⲿ����
extern xdata FaultCodeDef ErrCode; //�������
	
//����
void ReportError(FaultCodeDef Code); //�������
void DisplayErrorTIMHandler(void); //��ʾ����ʱ���õ��ļ�ʱ������
void DisplayErrorIDHandler(void); //���ݴ���ID������ʾ�Ĵ���
void OutputFaultDetect(void); //������ϼ�⺯��	
bit IsErrorFatal(void);	//��ѯ�����Ƿ�����
	
#endif
