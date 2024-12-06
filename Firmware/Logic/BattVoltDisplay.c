#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "delay.h"
#include "ModeControl.h"
#include "SideKey.h"
#include "cms8s6990.h"
#include "BattDisplay.h"
#include "Strap.h"

//ƽ������ṹ��
typedef struct
	{
	int Min;
  int Max;
	long AvgBuf;
	int Count;
	}AverageCalcDef;	
	
//��������
void DisplayErrorIDHandler(void);
	
	
//�ڲ�ȫ�ֱ���
static xdata char BattShowTimer=0; //��ص�����ʾ����
xdata BattStatusDef BattState; //��ص������λ
static xdata AverageCalcDef BattVolt;	
xdata float Battery; //��Ч���ڵ�ص�ѹ
bit IsBatteryAlert; //��ص�ѹ���ھ���ֵ	
bit IsBatteryFault; //��ص�ѹ���ڱ���ֵ		
static xdata int VshowTIM=0;
static xdata float VbattSample; //ȡ���ĵ�ص�ѹ
xdata BattVshowFSMDef VshowFSMState; //��ص�ѹ��ʾ����ļ�ʱ����״̬��ת��

//������ص�ѹ��ʾ
void TriggerVshowDisplay(void)	
	{
	if(VshowFSMState!=BattVdis_Waiting)return; //�ǵȴ���ʾ״̬��ֹ����
	VshowFSMState=BattVdis_PrepareDis;
	}		
	
//�����ϸ��ѹ��ʾ����
static void BatVshowFSM(void)
	{
	int buf;
	extern xdata char VbattCellCount;
	//������ʾ״̬��
	switch(VshowFSMState)
		{
		case BattVdis_Waiting:break; //�ȴ���ʾ�׶�
		case BattVdis_PrepareDis: //׼����ʾ
	    VshowTIM=14; //�ӳ�1.75��
			VshowFSMState=BattVdis_DelayBeforeDisplay; //��ʾͷ��
		  break;
		//�ӳٲ���ʾ��ͷ
		case BattVdis_DelayBeforeDisplay:
			if(VshowTIM>12)LEDMode=LED_Green;
      else if(VshowTIM>10)LEDMode=LED_Amber;		
		  else if(VshowTIM>8)LEDMode=LED_Red;	
		  else LEDMode=LED_OFF; //��������˸֮��ȴ�
		  //ͷ����ʾ������ʼ��ʽ��ʾ��ѹ
		  if(VshowTIM>0)break; //ʱ��δ��
			VbattSample=Data.RawBattVolt; //���е�ѹȡ��
	    if(VbattCellCount==2)VbattSample*=10; //2�ڵ��ģʽ����ѹȡ������10
		  if(((int)VbattSample)/100)
				{
				LEDMode=LED_RedBlinkThird;
				VshowFSMState=BattVdis_ShowChargeLvl; //��ѹ������ʾ��Χ���ú�ɫ������ָʾ��
				break;
				}
		  buf=(int)VbattSample;
		  buf/=10; //��ʾʮλ
		  if(buf==0)VshowTIM=-1; //0=˲����һ��
			else VshowTIM=(4*buf)-1; //������ʾ��ʱ��
		  VshowFSMState=BattVdis_Show10V; //��ת��10V��ʾ
		  break;
    //��ʾʮλ
		case BattVdis_Show10V:
			if(VshowTIM==-1)//ͨ������һ�α�ʾ��0
				{
				MakeFastStrobe(LED_Red);
				VshowTIM=0; 
				}
		  buf=VshowTIM%4;
			LEDMode=buf>1?LED_Red:LED_OFF; //�����ɫ��˸ָʾ10V״̬
		  if(VshowTIM<=0) //��ʾ����
				{
				LEDMode=LED_OFF;
				VshowTIM=10;
				VshowFSMState=BattVdis_Gap10to1V; //�ȴ�һ��
				}
		  break;
		//ʮλ�͸�λ֮��ļ��
		case BattVdis_Gap10to1V:
			if(VshowTIM>0)break; //ʱ��δ��
		  buf=(int)VbattSample; 
			buf%=10; //��ʾ��λ
			if(buf==0)VshowTIM=-1; //0=˲����һ��
			else VshowTIM=(4*buf)-1; //������ʾ��ʱ��
			VshowFSMState=BattVdis_Show1V; //��ת��1V��ʾ	
			break;	
		//��ʾ��λ
		case BattVdis_Show1V:
		  if(VshowTIM==-1)//ͨ������һ�α�ʾ��0
				{
				MakeFastStrobe(LED_Amber);
				VshowTIM=0; 
				}
			buf=VshowTIM%4;
			LEDMode=buf>1?LED_Amber:LED_OFF; //�����ɫ��˸ָʾ1V״̬
			if(VshowTIM<=0) //��ʾ����
				{
				LEDMode=LED_OFF;
				VshowTIM=10;
				VshowFSMState=BattVdis_Gap1to0_1V; //�ȴ�һ��
				}
		  break;
		//��λ��ʮ��λ֮��ļ��		
		case BattVdis_Gap1to0_1V:	
			  if(VshowTIM>0)break; //ʱ��δ��
				VbattSample*=(float)10;
				buf=(int)VbattSample; //��10��С�����һλ����Ϊ��λ 
				buf%=10; //�õ�ʮ��λ״̬
				if(buf==0)VshowTIM=-1; //0=˲����һ��
				else VshowTIM=(4*buf)-1; //������ʾ��ʱ��
				VshowFSMState=BattVdis_Show0_1V; //��ת��0.1V��ʾ
				break;
		//��ʾС�����һλ(0.1V)
		case BattVdis_Show0_1V:
		  if(VshowTIM==-1)//ͨ������һ�α�ʾ��0
				{
				MakeFastStrobe(LED_Green);
				VshowTIM=0; 
				}
			buf=VshowTIM%4;
			LEDMode=buf>1?LED_Green:LED_OFF; //������ɫ��˸ָʾ0.1V״̬
		  if(VshowTIM<=0) //��ʾ����
				{
				LEDMode=LED_OFF;
				VshowTIM=12; 
				VshowFSMState=BattVdis_WaitShowChargeLvl; //�ȴ�1���ʼ��ʾ����
				}
			break;
		//�ȴ�һ��ʱ�����ʾ��ǰ����
		case BattVdis_WaitShowChargeLvl:
			if(VshowTIM>0)break;
		  BattShowTimer=12; //�������������ʾ
		  VshowFSMState=BattVdis_ShowChargeLvl; //�ȴ�������ʾ״̬����
      break;
	  //�ȴ����������ʾ����
		case BattVdis_ShowChargeLvl:
			if(BattShowTimer||getSideKeyClickAndHoldEvent())break; //�û���Ȼ���°������ȴ��û��ɿ�
			VshowFSMState=BattVdis_Waiting; //��ʾ�������˻ص��ȴ��׶�
      break;
		}
	}

//��λ��ص�ѹ��⻺��
static void ResetBattAvg(void)	
	{
	BattVolt.Min=32766;
	BattVolt.Max=-32766; //��λ�����С������
	BattVolt.Count=0;
  BattVolt.AvgBuf=0; //���ƽ���������ͻ���
	}

//������ʱ��ʾ��ص�ѹ
void DisplayVBattAtStart(void)
	{
	int i;
	//��ǰ���µ�ص���״̬
	if(Data.BatteryVoltage<2.9)BattState=Battery_VeryLow; //��ص�ѹ����2.8��ֱ�ӱ������ز���
	else if(Data.BatteryVoltage<3.2)BattState=Battery_Low; //��ص�ѹ����3.2���л��������͵�״̬
	else if(Data.BatteryVoltage<3.6)BattState=Battery_Mid; //��ص�������3.5���ʾΪ�е�
	else BattState=Battery_Plenty; //��������
	//�����ع��Ϻ;���λ	
	IsBatteryAlert=0;
	IsBatteryFault=0;
	//��ʼ��ƽ��ֵ����
	ResetBattAvg();
	Battery=Data.BatteryVoltage; //���µ�ص�ѹ
  //��λ��ص�ѹ��ʾ״̬��		
	VshowFSMState=BattVdis_Waiting;
	//������ص�����ʾ(���޴������ϴζϵ�֮ǰ�ǹػ�״̬�������)
	if(CurrentMode->ModeIdx==Mode_OFF)
		{
	  for(i=0;i<VbattCellCount;i++)
			 {
			 MakeFastStrobe(LED_Amber);
			 delay_ms(160);
			 }
		delay_ms(400);
	  BattShowTimer=12;
		}
	}
//��ص�����ʾ��ʱ�Ĵ���
void BattDisplayTIM(void)
	{
	//��ص�ѹ��ʾ�ļ�ʱ������	
	if(VshowTIM>0)VshowTIM--;
	//�����ʾ��ʱ��
	if(BattShowTimer>0)BattShowTimer--;
	}
	
//��ز���������ָʾ�ƿ���
void BatteryTelemHandler(void)
	{
	float AlertThr;
	long buf;
	//����ƽ��ģ�����
  if(BattVolt.Count<VBattAvgCount)		
		{
		buf=(long)(Data.BatteryVoltage*1000);
		BattVolt.Count++;
		BattVolt.AvgBuf+=buf;
		if(BattVolt.Min>buf)BattVolt.Min=buf;
		if(BattVolt.Max<buf)BattVolt.Max=buf; //��ֵ��ȡ
		}
	else //ƽ�������������µ�ѹ
		{
		BattVolt.AvgBuf-=(long)BattVolt.Min+(long)BattVolt.Max; //ȥ��������
		BattVolt.AvgBuf/=(long)(BattVolt.Count-2); //��ƽ��ֵ
		Battery=(float)BattVolt.AvgBuf/(float)1000; //�õ����յĵ�ص�ѹ
		ResetBattAvg(); //��λ����
		}
	//���ݵ�ص�ѹ����flagʵ�ֵ͵�ѹ�����͹ػ�����
	if(CurrentMode->ModeIdx==Mode_Ramp)AlertThr=(float)RampCfg.BattThres/(float)1000; //�޼�����ģʽ�£�ʹ�ýṹ���ڵĶ�̬��ֵ
	else AlertThr=(float)(CurrentMode->LowVoltThres)/(float)1000; //�ӵ�ǰĿ�굲λ��ȡģʽֵ  
	IsBatteryFault=Battery>2.7?0:1; //����bit
	if(IsBatteryFault)IsBatteryAlert=0; //����bit�����ǿ���������bit
	else if(Data.VOUTRatio>75)IsBatteryAlert=1; //��������ֵ����86%��DCDCоƬ�Ѿ����ͣ�ǿ�ƽ���
	else IsBatteryAlert=Battery>AlertThr?0:1; //����bit
	//��ص���ָʾ״̬��
	switch(BattState) 
		 {
		 //��ص�������
		 case Battery_Plenty: 
				if(Battery<3.7)BattState=Battery_Mid; //��ص�ѹС��3.7���ص������ϵ�״̬
			  break;
		 //��ص�����Ϊ����
		 case Battery_Mid:
			  if(Battery>3.9)BattState=Battery_Plenty; //��ص�ѹ����3.8���ص�����״̬
				if(Battery<3.2)BattState=Battery_Low; //��ص�ѹ����3.2���л��������͵�״̬
				break;
		 //��ص�������
		 case Battery_Low:
		    if(Battery>3.5)BattState=Battery_Mid; //��ص�ѹ����3.5���л��������еȵ�״̬
			  if(Battery<2.9)BattState=Battery_VeryLow; //��ص�ѹ����2.8���������ز���
		    break;
		 //��ص������ز���
		 case Battery_VeryLow:
			  if(Battery>3.2)BattState=Battery_Low; //��ص�ѹ������3.0����ת����������׶�
		    break;
		 }
	//LED����
	if(LEDMode==LED_RedBlinkFifth||LEDMode==LED_GreenBlinkThird||LEDMode==LED_RedBlinkThird)return; //Ƶ��ָʾ�²�ִ�п��� 
	if(ErrCode!=Fault_None)DisplayErrorIDHandler(); //�й��Ϸ�������ʾ����
	else if(CurrentMode->ModeIdx!=Mode_OFF||BattShowTimer)switch(BattState) //�û�����������ѯ���������ֵ翪����ָʾ����
		 {
		 case Battery_Plenty:LEDMode=LED_Green;break; //��ص���������ɫ����
		 case Battery_Mid:LEDMode=LED_Amber;break; //��ص����еȻ�ɫ����
		 case Battery_Low:LEDMode=LED_Red;break;//��ص�������
		 case Battery_VeryLow:LEDMode=LED_RedBlink;break; //��ص������ز����ɫ����
		 }
	else if(VshowFSMState!=BattVdis_Waiting)BatVshowFSM();//��ص�ѹ��ʾ������ִ��״̬��	
  else LEDMode=LED_OFF; //�ֵ紦�ڹر�״̬����û�а������µĶ�������LED����Ϊ�ر�
	}
	