#include "BattDisplay.h"
#include "ModeControl.h"
#include "LowVoltProt.h"
#include "SideKey.h"

//�ڲ�����
static xdata char BattAlertTimer=0; //��ص͵�ѹ�澯����
static xdata char RampRiseCurrentTIM=0; //�޼�����ָ������ļ�ʱ��	

//�͵�����������
static void StartBattAlertTimer(void)
	{
	//������ʱ��
	if(BattAlertTimer)return;
	BattAlertTimer=1;
	}	

//��ص͵�������������
void BattAlertTIMHandler(void)
	{
	//�޼����⾯����ʱ
	if(RampRiseCurrentTIM>0&&RampRiseCurrentTIM<9)RampRiseCurrentTIM++;
	//��������
	if(BattAlertTimer>0&&BattAlertTimer<(BatteryAlertDelay+1))BattAlertTimer++;
	}	
	
//��ص͵�����������
void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump)
	{
	char Thr;
	bit IsChangingGear;
	//��ȡβ��״̬
	if(!getSideKey1HEvent())IsChangingGear=0;
	else IsChangingGear=getSideKeyHoldEvent();
	//���Ƽ�ʱ����ͣ
	if(!IsBatteryFault) //���û�з�����ѹ����
		{
		Thr=BatteryAlertDelay; //û�й��Ͽ�����һ�㽵��
		//��ǰ�ڻ����׶λ���û�и澯��ֹͣ��ʱ��,��������
		if(!IsBatteryAlert||IsChangingGear)BattAlertTimer=0;
		else StartBattAlertTimer();
		}
  else //������ѹ�澯����������ʱ��
		{
	  Thr=BatteryFaultDelay;
		StartBattAlertTimer(); 
		}
	//��ǰģʽ��Ҫ�ػ�
	if(IsNeedToShutOff||IsChangingGear)
		 {
		 //��ص�ѹ���ڹػ���ֵ�㹻ʱ�䣬�����ر�
		 if(IsBatteryFault&&BattAlertTimer>Thr)ReturnToOFFState(); 
		 }
	//����Ҫ�ػ���������������
	else if(BattAlertTimer>Thr)
		 {
	   BattAlertTimer=0;//���ö�ʱ������ʼֵ
	   SwitchToGear(ModeJump); //��λ��ָ����λ
		 }
	}		

//�޼�����ĵ͵�ѹ����
void RampLowVoltHandler(void)
	{
	if(!IsBatteryAlert&&!IsBatteryFault)//û�и澯
		{
		BattAlertTimer=0;
		if(BattState==Battery_Plenty) //��ص�������������״̬���������ӵ�������
			{
	    if(RampCfg.CurrentLimit<CurrentMode->Current)
				 {
			   if(!RampRiseCurrentTIM)RampRiseCurrentTIM=1; //������ʱ����ʼ��ʱ
				 else if(RampRiseCurrentTIM<9)return; //ʱ��δ��
         RampRiseCurrentTIM=1;
				 if(RampCfg.BattThres>CurrentMode->LowVoltThres)RampCfg.BattThres=CurrentMode->LowVoltThres; //��ѹ���ﵽ���ޣ���ֹ��������
				 else RampCfg.BattThres+=50; //��ѹ����ϵ�50mV
         if(RampCfg.CurrentLimit>CurrentMode->Current)RampCfg.CurrentLimit=CurrentMode->Current;//���ӵ���֮�������ֵ�Ƿ񳬳�����ֵ
				 else RampCfg.CurrentLimit+=250;	//�����ϵ�250mA		 
				 }
			else RampRiseCurrentTIM=0; //�Ѵﵽ�������޽�ֹ��������
			}
		return;
		}
	else RampRiseCurrentTIM=0; //������������λ�������ӵ����Ķ�ʱ��
	//��ѹ�澯������������ʱ��
	StartBattAlertTimer(); //��������������ʱ��
	if(IsBatteryFault&&BattAlertTimer>4)ReturnToOFFState(); //��ص�ѹ���ڹػ���ֵ����0.5�룬�����ر�
	else if(BattAlertTimer>BatteryAlertDelay) //��ص�λ����
		{
		if(RampCfg.CurrentLimit>750)RampCfg.CurrentLimit-=250; //�����µ�250mA
		if(RampCfg.BattThres>2750)RampCfg.BattThres-=25; //����25mV
    BattAlertTimer=1;//���ö�ʱ��
		}
	}
