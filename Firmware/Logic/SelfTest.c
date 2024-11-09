#include "LEDMgmt.h"
#include "delay.h"
#include "ADCCfg.h"
#include "ModeControl.h"
#include "OutputChannel.h"

//�ڲ�����
static xdata int ErrDisplayIndex;
static xdata char ShortDetectTIM;
xdata float VBattBeforeTurbo;
xdata char InputDetectTIM;

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
	
//����ѹ���Ӵ��������	
void InputFaultDetect(void)
	{
	if(CurrentMode->ModeIdx!=Mode_Turbo)InputDetectTIM=0; //�Ǽ���ģʽ���رոù���
	else if(InputDetectTIM<10) //���в���
		{
		InputDetectTIM++;
		if((VBattBeforeTurbo-Data.RawBattVolt)<3.2)return; //���ѹ���������
		ErrCode=Fault_InputConnectFailed; //���ѹ������˵��������ܴ�����ӣ���������
    if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
		}
	}	
	
//������ϼ��
void OutputFaultDetect(void)
	{
	char buf;
	if(CurrentMode->ModeIdx==Mode_OFF)ShortDetectTIM=0; //�ػ�״̬��λ���
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
		ErrCode=ShortDetectTIM&0x80?Fault_DCDCOpen:Fault_DCDCShort; //��д�������
    if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
		}
	}