#include "LEDMgmt.h"
#include "delay.h"
#include "ADCCfg.h"
#include "BattDisplay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "TailKey.h"
#include "Strap.h"

//�ڲ�����
static xdata int ErrDisplayIndex;
static xdata char ShortDetectTIM;
xdata char InputDetectTIM;

//�������
void ReportError(FaultCodeDef Code)
	{
	ErrCode=Code;
	if(CurrentMode->ModeIdx==Mode_Fault)return;
	TailMemory_Save(Mode_Fault); //���Ϸ��������õ�λ�������ػ�ģʽ
	SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
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
  if(ErrDisplayIndex<5)
		{
		if(ErrDisplayIndex<3)LEDMode=(LEDStateDef)(ErrDisplayIndex+1);	
		else LEDMode=LED_OFF;
		}
	//��˸ָ��������ʾErr ID
	else if(ErrDisplayIndex<(5+(6*(int)ErrCode)))
		{
		buf=(ErrDisplayIndex-5)/3; 
		if(!(buf%2))LEDMode=LED_Red;
		else LEDMode=LED_OFF;  //���մ���ID��˸ָ������
		}
  else LEDMode=LED_OFF; //LEDϨ��
	}
//�ڲ����������ϼ�����
static char ErrTIMCounter(char buf,char Count)
	{
	//�ۼӼ�����
	return buf<8?buf+Count:8;
	}

//������ϼ��
void OutputFaultDetect(void)
	{
	char buf,OErrID;
	bit IsLED6V;
	if(CurrentMode->ModeIdx==Mode_OFF||TailKeyTIM<(TailKeyRelTime+1))ShortDetectTIM=0; //�ػ�״̬��λ���
	else if(Current>0) //��ʼ���
		{		
		buf=ShortDetectTIM&0x0F; //ȡ����ʱ��ֵ					
		IsLED6V=Data.OutputVoltage>4.5?1:0;//LED���ͼ��
		//�����ѹ����
		if(Data.BatteryVoltage>4.4)ReportError(Fault_InputOVP); 
		//��·���	
		if(Data.OutputVoltage<2.1||Data.FBInjectVolt>4.8) //�����·
			{
			buf=ErrTIMCounter(buf,2); //��ʱ���ۼ�
			OErrID=0;
			}
		//�����·���
		else if(Data.FBInjectVolt<0.25||Data.OutputVoltage>7.5) 
			{
			buf=ErrTIMCounter(buf,1); //��ʱ���ۼ�
			OErrID=1;
			}
		//LED���ʹ�����
		else if(Is6VLED!=IsLED6V)
			{
			buf=ErrTIMCounter(buf,1); //��ʱ���ۼ�
			OErrID=2;
			}
		else buf=buf>0?buf-1:0; //û�з����������������
		//���ж�ʱ����ֵ�Ļ�д
		ShortDetectTIM=buf|(OErrID<<4);
		//״̬���
		if(buf<8)return; //û�й���,����ִ��
		switch((ShortDetectTIM>>4)&0x0F)	
			{
			case 1:ReportError(Fault_DCDCOpen);break;
			case 2:ReportError(Fault_StrapMismatch);break;
			default:ReportError(Fault_DCDCShort);
			}
		}
	}