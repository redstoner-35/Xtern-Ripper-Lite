#include "SpecialMode.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"
#include "TempControl.h"
#include "LowVoltProt.h"
#include "TailKey.h"
#include "Strap.h"
#include "SelfTest.h"
#include "SOS.h"

//��λ�ṹ��
code ModeStrDef ModeSettings[ModeTotalDepth]=
	{
		//�ػ�״̬
    {
		Mode_OFF,
		0,
		0,  //����0mA
		0,  //�ػ�״̬��ֵΪ0ǿ�ƽ������
		true,
		false,
		100
		}, 
		//������
		{
		Mode_Fault,
		0,
		0,  //����0mA
		0,
		false,
		false,
		100
		}, 
		//�¹�
		{
		Mode_Moon,
		15,  //Ĭ��15mA����
		0,   //��С����û�õ�������
		2800,  //2.8V�ض�
		false, //�¹⵵��ר����ڣ����������
		false,
		50
		}, 	
    //����
		{
		Mode_Low,
		1000,  //1A����
		0,   //��С����û�õ�������
		2900,  //2.8V�ض�
		true,
		false,
		69
		},
    //����
		{
		Mode_Mid,
		2000,  //2000mA����
		0,   //��С����û�õ�������
		3000,  //3V�ض�
		true,
		false,
		86
		}, 	
    //�и���
		{
		Mode_MHigh,
		4000,  //4000mA����
		0,   //��С����û�õ�������
		3050,  //3.05V�ض�
		true,
		true,
		96
		}, 	
    //����
		{
		Mode_High,
		8000,  //8000mA����
		0,   //��С����û�õ�������
		3100,  //3.1V�ض�
		true,
		true,
		102
		}, 	
    //����
		{
		Mode_Turbo,
		23000,  //23A����(������ٵ���ȡ����strap)
		0,   //��С����û�õ�������
		3200,  //3.2V�ض�
		false, //�������ܴ�����
		true,
		106
		}, 	
    //����		
		{
		Mode_Strobe,
		22000,  //22A����
		0,   //��С����û�õ�������
		2500,  //2.5V�ض�(ʵ����2.7�ͻ���բ���������2.5��Ϊ�˱���͵�ѹ�������������±��������쳣)
		false, //�������ܴ�����
		true,
		105
		}, 
	  //�޼�����		
		{
		Mode_Ramp,
		8000,  //8000mA�������
		1000,   //��С1000mA
		3200,  //3.2V�ض�
		false, //���ܴ�����  
		true,
		69
		}, 
	  //SOS
		{
		Mode_SOS,
		12000,  //12A����
		0,   //��С����û�õ�������
		2500,  //2.5V�ض�(ʵ����2.7�ͻ���բ��ʵ�����������2.5��Ϊ�˱���͵�ѹ��������������SOS״̬������SOS�����쳣)
		false,	//SOS���ܴ�����
		true,
		105
		}, 
	};

//ȫ�ֱ���(��λ)
ModeStrDef *CurrentMode; //��λ�ṹ��ָ��
xdata ModeIdxDef LastMode; //����Ϊ����
RampConfigDef RampCfg; //�޼���������	

//ȫ�ֱ���(״̬λ)
bit IsRampEnabled; //�Ƿ����޼�����
bit IsRampStart=0; //β�������޼�����
	
//�����ʱ����
xdata char HoldChangeGearTIM; //��λģʽ�³�������
xdata char DisplayLockedTIM=0; //������ս��ģʽ�����˳���ʾ	
xdata char TailSaveTIM=25; //β�����������ʱ��
	
//��ʼ��ģʽ״̬��
void ModeFSMInit(void)
{
	char i;
	//��ʼ���޼�����
	RampCfg.RampMaxDisplayTIM=0;
  for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)//��ȡ�޼����������
	  {
		RampCfg.BattThres=ModeSettings[i].LowVoltThres; //��ѹ������޻ָ�
		RampCfg.CurrentLimit=ModeSettings[i].Current; //�ҵ���λ�������޼�����ĵ�λ���������޻ָ�
		}
	ReadRampConfig(); //��EEPROM�ڶ�ȡ�޼���������
	//��λģʽ����
	ResetSOSModule(); //��λSOSģ��
	LastMode=Mode_Low;
	ErrCode=Fault_None; //û�й���
	CurrentMode=&ModeSettings[0]; //��������Ϊ��һ����
  SysMode=Operation_Normal; //ƽ��ģʽ
}	

//��λ״̬������������ʱ������
void ModeFSMTIMHandler(void)
{
	//�޼�������صĶ�ʱ��
  if(TailSaveTIM<24)TailSaveTIM++;
	if(RampCfg.CfgSavedTIM<32)RampCfg.CfgSavedTIM++;
	if(RampCfg.RampMaxDisplayTIM>0)RampCfg.RampMaxDisplayTIM--;
	//����������ʾ��ʱ��
  if(DisplayLockedTIM>0)DisplayLockedTIM--;
}

//��λ��ת
void SwitchToGear(ModeIdxDef TargetMode)
	{
	char i;
  int LastICC;
	//��¼����ǰ�Ľ��
	ModeIdxDef BeforeMode=CurrentMode->ModeIdx; //�洢��ǰģʽ				
	LastICC=CurrentMode->Current; //�洢֮ǰ�ĵ�λ
	//��ʼѰ��
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==TargetMode)
		{
    ResetSOSModule();		//��λ����SOSģ��
		CurrentMode=&ModeSettings[i]; //�ҵ�ƥ��index����ֵ�ṹ��
		if(BeforeMode==Mode_Turbo&&TargetMode!=Mode_Turbo)RecalcPILoop(LastICC); //�Ӽ����л���������λ����������PI��
		if(BeforeMode==Mode_OFF&&TargetMode!=Mode_OFF)TailMemory_Save(TargetMode); //�ػ��л��������������������
		else TailSaveTIM=0; //�����ʱ��׼����һ���ټ���
		}
	}
	
//�����ػ�����	
void ReturnToOFFState(void)
	{
	if(CurrentMode->ModeIdx==Mode_OFF)return; //�ػ�״̬��ִ��		
	if(CurrentMode->IsModeHasMemory)LastMode=CurrentMode->ModeIdx; //�洢�ػ�ǰ�ĵ�λ
	SwitchToGear(Mode_OFF); //ǿ�����ص��ػ���λ
	TailMemory_Save(Mode_OFF); //�ػ���ʱ�������������
	}	

//���������ļ����������
void HoldSwitchGearCmdHandler(void)
	{
	char buf;
	if(SysMode||IsRampEnabled)HoldChangeGearTIM=0; //ս��ģʽ���߽������������ߴ����޼�����ģʽ����ֹ����ϵͳ����
	else if(!getSideKeyHoldEvent()&&!getSideKey1HEvent())HoldChangeGearTIM=0; //�����ɿ�����ʱ����λ
	else //ִ�л�������
		{
		buf=HoldChangeGearTIM&0x1F; //ȡ��TIMֵ
		if(buf==0&&!(HoldChangeGearTIM&0x40))HoldChangeGearTIM|=getSideKey1HEvent()?0x20:0x80; //�������λ1ָʾ�������Լ���
		HoldChangeGearTIM&=0xE0; //ȥ����ԭʼ��TIMֵ
		if(buf<HoldSwitchDelay&&!(HoldChangeGearTIM&0x40))buf++;
		else buf=0;  //ʱ�䵽��������
		HoldChangeGearTIM|=buf; //����ֵд��ȥ
		}
	}	

//��ȡ��ǰ��λ�ĵ���ֵ
int QueryCurrentGearILED(void)	
	{
	//�����ǰ��λ����ֵ���ڼ��������򷵻ؼ�������ֵ
	return CurrentMode->Current<TurboCurrent?CurrentMode->Current:TurboCurrent;
	}

//�ఴ������������ִ��
static void SideKeySwitchGearHandler(ModeIdxDef TargetMode,char TKCount)	
	{
	if(!(HoldChangeGearTIM&0x80)&&TKCount!=1)return;
	HoldChangeGearTIM&=0x7F; //������λ��Ǳ��λ������
  SwitchToGear(TargetMode); //����Ŀ�굲λ
	}
	
//�ఴ����+�����������˲���ִ��
static void SideKey1HRevGearHandler(ModeIdxDef TargetMode)
	{
	if(!(HoldChangeGearTIM&0x20))return;
	HoldChangeGearTIM&=0xDF; //������λ��Ǳ��λ������
	SwitchToGear(TargetMode); //����Ŀ�굲λ
	}	
	
//�޼����⴦��
static void RampAdjHandler(char TKCount)
	{
	static bit IsKeyPressed=0;	
	static bit RampDIR=0;
  int Limit;
	bit IsPress;
  //������޼���������
	IsPress=(getSideKey1HEvent()||getSideKeyHoldEvent())?1:0;
	Limit=RampCfg.CurrentLimit<CurrentMode->Current?RampCfg.CurrentLimit:CurrentMode->Current;
	if(Limit<CurrentMode->Current&&IsPress&&RampCfg.Current>Limit)RampCfg.Current=Limit; //�ڵ��������Ƶ�������û����°������Ե��������������޷�
	//β��ģʽ��ѭ���ķ�ʽʵ���޼�����
	if(!IsRampStart)
		{
		//�ر���״̬�µ�����ʼ����ѭ��
		if(TKCount==1)IsRampStart=1;
		}
	else //��ʼ����ѭ��
		{
		if(RampDIR)RampCfg.Current++; 
		else RampCfg.Current--; //��������	
		if(RampCfg.Current<=CurrentMode->MinCurrent)
			{
			RampDIR=1;
		  RampCfg.Current=CurrentMode->MinCurrent; //�����ﵽ���޿�ʼ��ת
			}
	 if(RampCfg.Current>=Limit) //��ǰ������������
			{
			RampDIR=0;
			RampCfg.Current=Limit; //���Ƶ������ֵ	
			}
		//�û����°�������������
		if(TKCount||IsKeyEventOccurred())
			{
		  IsRampStart=0;
			RampCfg.CfgSavedTIM=30; //��λ��ʱ���������ȱ���
			}
		}	
	//�������ȵ���
	if(getSideKeyHoldEvent()&&!IsKeyPressed) //�������ӵ���
			{	
			if(RampCfg.Current<Limit)RampCfg.Current++;
			else
				{
				RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5��ָʾ�Ѿ�������
				RampCfg.Current=Limit; //���Ƶ������ֵ	
				IsKeyPressed=1;
				}
			}	
	else if(getSideKey1HEvent()&&!IsKeyPressed) //����+�������ٵ���
		 {
			if(RampCfg.Current>CurrentMode->MinCurrent)RampCfg.Current--; //���ٵ���	
	    else
				{
				RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5��ָʾ�Ѿ�������
				RampCfg.Current=CurrentMode->MinCurrent; //���Ƶ�����Сֵ
				IsKeyPressed=1;
				}
		 }
  else if(!IsPress&&IsKeyPressed)IsKeyPressed=0; //�û��ſ��������������		
	//�������ݱ�����ж�
	if(IsPress)RampCfg.CfgSavedTIM=0; //��������˵�����ڵ�������λ��ʱ��
	else if(RampCfg.CfgSavedTIM==32)
			{
			RampCfg.CfgSavedTIM++;
			SaveRampConfig(0);  //һ��ʱ����û����˵���Ѿ�������ϣ���������
			}
	}

//����Ƿ���Ҫ�ػ�
static void DetectIfNeedsOFF(int ClickCount)
	{
	if(getSideKeyNClickAndHoldEvent()==2)TriggerVshowDisplay();
	if(!SysMode&&ClickCount!=1)return;
	if(SysMode&&getSideKeyHoldEvent())return;
	ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
	}	

//��λ״̬��
void ModeSwitchFSM(void)
	{
	char ClickCount;
	char TKCount=0;
	//�ⲿ��������
	extern volatile bit StrobeFlag;
	//��ȡ����״̬
	if(!SysMode)TKCount=GetTailKeyCount(); //�ఴս��ģʽ��������β��
	ClickCount=getSideKeyShortPressCount(0);	//��ȡ�����������������Ĳ���
	//��λ�����������EEPROM����
	if(LastMode==Mode_OFF||LastMode>=ModeTotalDepth)LastMode=Mode_Low;
	//״̬��
	IsHalfBrightness=0; //������Ĭ��ȫ��
	switch(CurrentMode->ModeIdx)	
		{
		//���ִ���	
		case Mode_Fault:
      SysMode=Operation_Normal; //���Ϻ��Զ��ص���ͨģʽ			
			if(!getSideKeyLongPressEvent()||IsErrorFatal())break; //�û�û�а��°�ť�����������Ĵ���״̬����������
			ClearError(); //��������ǰ����
		  break;
		//�ػ�״̬
		case Mode_OFF:
		  //�������⹦��
		  SpecialModeOperation(ClickCount);
	    if(SysMode)break;
		  //������ģʽ�����������ػ�������
			if(ClickCount==1||TKCount)PowerToNormalMode(LastMode); //�ఴ������������ѭ��	
			//���뼫���ͱ���
			else EnterTurboStrobe(ClickCount,TKCount);		
      if(getSideKeyLongPressEvent())SwitchToGear(Mode_Moon); //��������ֱ�ӽ��¹�					
			if(ClickCount==4) //�Ļ��л���λģʽ���޼�����
					{	
					IsRampEnabled=~IsRampEnabled; //ת���޼�����״̬	
					LEDMode=IsRampEnabled?LED_GreenBlinkThird:LED_RedBlinkThird; //��ʾ�Ƿ���
					SaveRampConfig(0); //�������õ�ROM��
					}
		  //��ѯ��ѹ
			if(getSideKeyNClickAndHoldEvent())TriggerVshowDisplay();
  		break;
		//�¹�״̬
		 case Mode_Moon:
			 IsHalfBrightness=1; //�¹�ģʽ���������ȼ���
			 BatteryLowAlertProcess(true,Mode_Moon);
		   DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
		   EnterTurboStrobe(TKCount,0); //β��ģʽ�£���Ҫ����һ�����뼫�����߱������Լ��ϼ��			
			 //��ص�ѹ���㣬�������������λ
		   if(getSideKeyLongPressEvent()||TKCount==1)  
					{
					PowerToNormalMode(Mode_Low); //����������ģʽ
					if(CurrentMode->ModeIdx==Mode_Moon)break;//����֮���޷��ɹ��뿪�¹�ģʽ������������ĸ�λ����
					if(IsRampEnabled)RestoreToMinimumRampCurrent(); //������޼�������ָ�����͵���
					HoldChangeGearTIM|=0x40; //��ʱ���ڽ�ֹ����������ȷ��Ҫ�û��ɿ�����ܻ�
					if(!TKCount)RampCfg.RampMaxDisplayTIM=4; //�ఴģʽ��Ϊû������ϵ磬��ҪϨ��0.5������л�
					}		    
		    break;			
    //�޼�����״̬				
    case Mode_Ramp:
		    if(!IsRampStart) //�ǵ���ģʽ������ػ��ͽ�������ģʽ
					{
					if(TKCount==4)SwitchToGear(Mode_Moon); //β���Ļ������¹�
			    DetectIfNeedsOFF(ClickCount); //����Ƿ���Ҫ�ػ�
					EnterTurboStrobe(TKCount,ClickCount); //���뼫�����߱����ļ��
					}
		    //�޼����⴦��
		    RampLowVoltHandler(); //�͵�ѹ����
        RampAdjHandler(TKCount);			
		    break;
    //����״̬		
    case Mode_Low:
			  BatteryLowAlertProcess(true,Mode_Low);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
				EnterTurboStrobe(TKCount,ClickCount); //���뼫�����߱����ļ��
		    //������������
		    SideKeySwitchGearHandler(Mode_Mid,TKCount); //�����е�
		    break;	    		
    //����״̬		
    case Mode_Mid:
			  BatteryLowAlertProcess(false,Mode_Low);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
				EnterTurboStrobe(TKCount,ClickCount); //���뼫�����߱����ļ��
		    //������������
		    SideKeySwitchGearHandler(Mode_MHigh,TKCount); //�����иߵ�
		    SideKey1HRevGearHandler(Mode_Low); //����+�������˵�λ���͵�
		    break;	
	  //�и���״̬
    case Mode_MHigh:
			  BatteryLowAlertProcess(false,Mode_Mid);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
				EnterTurboStrobe(TKCount,ClickCount); //���뼫�����߱����ļ��
		    //������������
		    SideKeySwitchGearHandler(Mode_High,TKCount); //�����ߵ�
		    SideKey1HRevGearHandler(Mode_Mid); //����+�������˵�λ���е�
		    break;	
	  //����״̬
    case Mode_High:
			  BatteryLowAlertProcess(false,Mode_MHigh);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
				EnterTurboStrobe(TKCount,ClickCount); //���뼫�����߱����ļ��
		    //������������
		    SideKeySwitchGearHandler(Mode_Low,0); //�����͵�λ����ѭ��
		    if(TKCount==1)SwitchToGear(Mode_Moon); //�����¹�	    
		    SideKey1HRevGearHandler(Mode_MHigh); //����+�������˵�λ���иߵ�
		    break;
		//����״̬
    case Mode_Turbo:
			  BatteryLowAlertProcess(false,Mode_High);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
			  if(TKCount==1||ClickCount==2||IsForceLeaveTurbo)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //˫�������¶ȴﵽ����ֵ��ǿ�Ʒ��ص�����
				if(TKCount==3||ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    break;	
		//����״̬
    case Mode_Strobe:
			  BatteryLowAlertProcess(true,Mode_Strobe);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
		    LeaveSpecialMode(TKCount,ClickCount); //�˳�����ģʽ�ص������ط������
		    //������������
		    SideKeySwitchGearHandler(Mode_SOS,TKCount); //�����л���SOS
		    break;	
    //SOS��ȵ�λ		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
			  LeaveSpecialMode(TKCount,ClickCount); //�˳�����ģʽ�ص������ط������
		    //������������
		    SideKeySwitchGearHandler(Mode_Strobe,TKCount); //�����л�������
		    break;	
		}
  //Ӧ���������
	if(DisplayLockedTIM>0||IsDisplayLocked)Current=50; //�û���������˳���������50mA���ݵ�����ʾһ��
	else if(RampCfg.RampMaxDisplayTIM>0)Current=0; //�޼�����ģʽ�ڵִ������޺����Ϩ��
	else switch(CurrentMode->ModeIdx)	
		{
		case Mode_SOS: 
		case Mode_Strobe://����ģʽ��SOSģʽ	     
	     switch(BattState)//ȡ����λ����
				 {
				 case Battery_Plenty:Current=QueryCurrentGearILED();break;
			   case Battery_Mid:Current=10000;break;
         case Battery_Low:Current=5000;break;
				 case Battery_VeryLow:Current=110;break;
				 }
			 //����״̬���Ƶ���
			 if(CurrentMode->ModeIdx==Mode_Strobe&&!StrobeFlag)Current=0; 
			 if(CurrentMode->ModeIdx==Mode_SOS&&!SOSFSM())Current=0;
		   break; 
		//����ģʽ������ȡ����ֵ
		default:
		  if(LowPowerStrobe())Current=0; //������ѹ��������˸
			else if(CurrentMode->ModeIdx==Mode_Ramp)Current=RampCfg.CurrentLimit<RampCfg.Current?RampCfg.CurrentLimit:RampCfg.Current; //�޼�����ģʽȡ�ṹ��������
		  else Current=QueryCurrentGearILED();//������λʹ������ֵ��ΪĿ�����
		}	
	//�����������
	getSideKeyShortPressCount(1); 
	//ROM��λ�������
	if((IsPOSTKPressed&&TKCount)||TailSaveTIM==24) //�ڵ�λͣ����ʱ���㹻���߿���ʱ�����˵�λ���أ���������
		{
		IsPOSTKPressed=0; //��λ���Ϊ
		TailSaveTIM++;
		TailMemory_Save(CurrentMode->ModeIdx);
		}
	}