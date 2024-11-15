#include "LEDMgmt.h"
#include "delay.h"
#include "ADCCfg.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "TailKey.h"

//�ڲ�����
static xdata int ErrDisplayIndex;
static xdata char ShortDetectTIM;
xdata float VBattBeforeTurbo;
xdata char InputDetectTIM;

//�������
void ReportError(FaultCodeDef Code)
	{
	ErrCode=Code;
	if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
	}

//����ID��ʾ��ʱ����	
void DisplayErrorTIMHandler(void)	
	{
	//û�д���������λ��ʱ��
	if(ErrCode==Fault_None)ErrDisplayIndex=0;
	else //�������󣬿�ʼ��ʱ
		{
		ErrDisplayIndex++;
    if(ErrDisplayIndex>=(5+(6*(int)ErrCode)+10))ErrDisplayIndex=0; //���޵��ˣ���ʼ��ת
		}
	}

//���ִ���ʱ��ʾDCDC�Ĵ���ID
void DisplayErrorIDHandler(void)
	{
	int buf;
	//�ȵ���ʾ����̽�����
  if(ErrDisplayIndex<5)switch(ErrDisplayIndex) 
		{
		case 0:LEDMode=LED_Green;break;
		case 1:LEDMode=LED_Amber;break;
		case 2:LEDMode=LED_Red;break;
		default:LEDMode=LED_OFF;
		}
	else if(ErrDisplayIndex<(5+(6*(int)ErrCode)))
		{
		buf=(ErrDisplayIndex-5)/3; 
		if(!(buf%2))LEDMode=LED_Red;
		else LEDMode=LED_OFF;  //���մ���ID��˸ָ������
		}
  else LEDMode=LED_OFF; //LEDϨ��
	}
	
//������ϼ��
void OutputFaultDetect(void)
	{
	char buf;
	if(CurrentMode->ModeIdx==Mode_OFF||TailKeyTIM<(TailKeyRelTime+1))ShortDetectTIM=0; //�ػ�״̬��λ���
	else if(Current>0) //��ʼ���
		{
		buf=ShortDetectTIM&0x7F; //ȡ����ʱ��ֵ
		if(Data.OutputVoltage<4.5) //�����·
			{
			buf=buf<8?buf+2:8; //��ʱ���ۼ�
			ShortDetectTIM&=0x7F;
			}
		else if(Data.OutputVoltage>7.15) //�����·
			{
			buf=buf<8?buf+1:8;  //��ʱ���ۼ�
			ShortDetectTIM|=0x80;
			}
			else buf=buf>0?buf-1:0; //û�з����������������
		//���ж�ʱ����ֵ�Ļ�д
		ShortDetectTIM&=0x80;
		ShortDetectTIM|=buf;
		//״̬���
		if(buf<8)return; //û�й���
		ReportError(ShortDetectTIM&0x80?Fault_DCDCOpen:Fault_DCDCShort); //ʱ�䵽���������
		}
	}