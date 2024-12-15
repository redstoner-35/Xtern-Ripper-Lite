#include "ModeControl.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"
#include "cms8s6990.h"
#include "TempControl.h"
#include "TailKey.h"
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
		12000,  //12A����
		0,   //��С����û�õ�������
		3000,  //3.0V�ض�
		false, //�������ܴ�����
		true,
		105
		}, 
	  //�޼�����		
		{
		Mode_Ramp,
		8000,  //8000mA�������
		1000,   //��С1000mA
		3100,  //3.1V�ض�
		false, //���ܴ�����  
		true,
		69
		}, 
	  //SOS
		{
		Mode_SOS,
		12000,  //12A����
		0,   //��С����û�õ�������
		3000,  //3.0V�ض�
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
bit IsLocked; //����ָʾ
bit IsTacMode; //����ս��ģʽ
bit IsRampStart=0; //β�������޼�����
	
//�����ʱ����
xdata char BattAlertTimer=0; //��ص͵�ѹ�澯����
xdata char HoldChangeGearTIM=0; //��λģʽ�³�������
xdata char DisplayLockedTIM=0; //������ս��ģʽ�����˳���ʾ	
xdata char ClickHoldReverseGearTIM=0; //��λģʽ�µ���+�������򻻵�
xdata char TailSaveTIM=25; //β�����������ʱ��
xdata char RampRiseCurrentTIM=0; //�޼�����ָ������ļ�ʱ��	
	
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
	IsLocked=0; //�ر�����
	IsTacMode=0; //�˳�ս��ģʽ
}	

//��λ״̬������������ʱ������
void ModeFSMTIMHandler(void)
{
	char buf;
	//�޼�������صĶ�ʱ��
  if(TailSaveTIM<24)TailSaveTIM++;
	if(RampRiseCurrentTIM>0&&RampRiseCurrentTIM<9)RampRiseCurrentTIM++;
	if(RampCfg.CfgSavedTIM<32)RampCfg.CfgSavedTIM++;
	if(RampCfg.RampMaxDisplayTIM>0)RampCfg.RampMaxDisplayTIM--;
	//����������ʾ��ʱ��
  if(DisplayLockedTIM>0)DisplayLockedTIM--;
	//��ؾ�����ʱ��
	if(BattAlertTimer&0x80)
		{
		buf=BattAlertTimer&0x7F; //ȡ��TIMֵ
		BattAlertTimer&=0x80; //ȥ����ԭʼ��TIMֵ
		if(buf<(BatteryAlertDelay+1))buf++;
		BattAlertTimer|=buf; //����ֵд��ȥ
		}
	else BattAlertTimer=0; //���buf	
}

//��λ��ת
int SwitchToGear(ModeIdxDef TargetMode)
	{
  int i,LastICC;
	ModeIdxDef BeforeMode=CurrentMode->ModeIdx; //�洢��ǰģʽ				
	LastICC=CurrentMode->Current; //�洢֮ǰ�ĵ�λ
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==TargetMode)
		{
    ResetSOSModule();		//��λ����SOSģ��
		CurrentMode=&ModeSettings[i]; //�ҵ�ƥ��index����ֵ�ṹ��
		if(BeforeMode==Mode_Turbo&&TargetMode!=Mode_Turbo)RecalcPILoop(LastICC); //�Ӽ����л���������λ����������PI��
		if(BeforeMode==Mode_OFF&&TargetMode!=Mode_OFF)TailMemory_Save(TargetMode); //�ػ��л��������������������
		else TailSaveTIM=0; //�����ʱ��׼����һ���ټ���
		return 0;
		}
	//ɶҲû�ҵ�������
	return 1;
	}
	
//�޼�����ĵ͵�ѹ����
void RampLowVoltHandler(void)
	{
	char time;
	extern BattStatusDef BattState;
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
	if(BattAlertTimer==0)BattAlertTimer=0x80;//��ʱ������
	time=BattAlertTimer&0x7F; //��ȡ��ǰ�ļ�ʱֵ
	if(IsBatteryFault&&time>4)ReturnToOFFState(); //��ص�ѹ���ڹػ���ֵ����0.5�룬�����ر�
	else if(time>BatteryAlertDelay) //��ص�λ����
		{
		if(RampCfg.CurrentLimit>750)RampCfg.CurrentLimit-=250; //�����µ�250mA
		if(RampCfg.BattThres>2750)RampCfg.BattThres-=25; //����25mV
    BattAlertTimer=0x80;//���ö�ʱ��
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
	
//�͵�����������
static void BatteryLowAlertProcess(bool IsNeedToShutOff,ModeIdxDef ModeJump)
	{
	char time,Thr;
	if(!IsBatteryAlert&&!IsBatteryFault)//û�и澯
		{
		BattAlertTimer=0;
		return;
		}
	if(BattAlertTimer==0)BattAlertTimer=0x80;//��ʱ������
	time=BattAlertTimer&0x7F; //��ȡ��ǰ�ļ�ʱֵ
	if(!IsBatteryFault)Thr=BatteryAlertDelay;
	else Thr=2;
	//��ص������ع���
	if(IsNeedToShutOff&&IsBatteryFault&&time>=3)ReturnToOFFState(); //��ص�ѹ���ڹػ���ֵ����0.5�룬�����ر�
	//��������
	else if(time>Thr)
		 {
	   BattAlertTimer=0x80;//���ö�ʱ��
	   SwitchToGear(ModeJump); //��λ��ָ����λ
		 }
	}	

//���������ļ����������
void HoldSwitchGearCmdHandler(void)
	{
	char buf;
	if(CurrentMode->ModeIdx==Mode_OFF)return; //�ػ�״̬��ֹ��ʱ������
	if(!getSideKeyHoldEvent())HoldChangeGearTIM=0; //�����ɿ������ǹػ�״̬����ʱ����λ
	else
		{
		buf=HoldChangeGearTIM&0x3F; //ȡ��TIMֵ
		if(buf==0&&!(HoldChangeGearTIM&0x40))HoldChangeGearTIM|=0x80; //�����λΪ1ָʾ�������Լ���
		HoldChangeGearTIM&=0xC0; //ȥ����ԭʼ��TIMֵ
		if(buf<HoldSwitchDelay&&!(HoldChangeGearTIM&0x40))buf++;
		else buf=0;  //ʱ�䵽��������
		HoldChangeGearTIM|=buf; //����ֵд��ȥ
		}
	//����+��������
  if(!getSideKeyClickAndHoldEvent())ClickHoldReverseGearTIM=0;	//�����ɿ������ǹػ�״̬����ʱ����λ	
	else
		{
		buf=ClickHoldReverseGearTIM&0x3F; //ȡ��TIMֵ
		if(buf==0&&!(ClickHoldReverseGearTIM&0x40))ClickHoldReverseGearTIM|=0x80; //�����λΪ1ָʾ�������Լ���
		ClickHoldReverseGearTIM&=0xC0; //ȥ����ԭʼ��TIMֵ
		if(buf<HoldSwitchDelay&&!(ClickHoldReverseGearTIM&0x40))buf++;
		else buf=0;  //ʱ�䵽��������
		ClickHoldReverseGearTIM|=buf; //����ֵд��ȥ
		}
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
	if(!(ClickHoldReverseGearTIM&0x80))return;
	ClickHoldReverseGearTIM&=0x7F; //������λ��Ǳ��λ������
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
	IsPress=(getSideKeyClickAndHoldEvent()||getSideKeyHoldEvent())?1:0;
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
		if(TKCount==1||IsKeyEventOccurred())
			{
		  IsRampStart=0;
			RampCfg.CfgSavedTIM=30; //��λ��ʱ���������ȱ���
			}
		}	
	//�������ȵ���
	if(getSideKeyHoldEvent()&&!IsKeyPressed)RampCfg.Current++; //�������ӻ��߼��ٵ���
	else if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed)RampCfg.Current--; //���ӻ��߼��ٵ���	
  else if(!IsPress&&IsKeyPressed)IsKeyPressed=0; //�û��ſ��������������		
	//�����ﵽ����
	if(getSideKeyHoldEvent()&&!IsKeyPressed&&RampCfg.Current>=Limit)
			{
			RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5��ָʾ�Ѿ�������
			RampCfg.Current=Limit; //���Ƶ������ֵ	
			IsKeyPressed=1;
			}		
	//�����ﵽ����
	if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed&&RampCfg.Current<=CurrentMode->MinCurrent)
			{
			RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5��ָʾ�Ѿ�������
			RampCfg.Current=CurrentMode->MinCurrent; //���Ƶ�����Сֵ
			IsKeyPressed=1;
			}			
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
	if(ClickCount!=1)return;
	if(!IsTacMode&&getSideKeyHoldEvent())return;
	ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
	}	

//���뼫���ͱ������ж�
static void EnterTurboStrobe(int TKCount,int ClickCount)	
	{
	extern bit IsDisableTurbo;
	int Count=TKCount>ClickCount?TKCount:ClickCount;
	if(Count==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
	if(Count==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
	}

//��λ״̬��
void ModeSwitchFSM(void)
	{
	bit IsHoldEvent;
	int ClickCount;
	char TKCount;
	//�ⲿ��������
	extern volatile bit StrobeFlag;
	extern bit IsForceLeaveTurbo;
	//��ȡ����״̬
	TKCount=GetTailKeyCount();
	IsHoldEvent=getSideKeyLongPressEvent();	
	ClickCount=getSideKeyShortPressCount(0);	//��ȡ�����������������Ĳ���
	//��λ�����������EEPROM����
	if(LastMode==Mode_OFF||LastMode>=ModeTotalDepth)LastMode=Mode_Low;
	//״̬��
	IsHalfBrightness=0; //������Ĭ��ȫ��
	switch(CurrentMode->ModeIdx)	
		{
		//���ִ���	
		case Mode_Fault:
      IsTacMode=0; //���Ϻ��Զ�ȡ��ս��ģʽ			
			if(!IsHoldEvent||IsErrorFatal())break; //�û�û�а��°�ť�����������Ĵ���״̬����������
		  ErrCode=Fault_None; //�޹���
			SwitchToGear(Mode_OFF);  //�������ô���
		  break;
		//�ػ�״̬
		case Mode_OFF:
			if(ClickCount==5)
					{
					IsTacMode=0; //��������ʱ�Զ��˳�ս��ģʽ
					IsLocked=IsLocked?0:1; //����״̬�л�
					DisplayLockedTIM=8; //ָʾ����״̬�л�
					}
			else if(IsLocked&&(ClickCount>0||IsKeyEventOccurred()))LEDMode=LED_RedBlinkFifth; //ָʾ�ֵ��ѱ�����
			//������״̬�������������
	    if(IsLocked)break;
			//ս��ģʽ
      if(ClickCount==6)  //6������
					{
					IsTacMode=IsTacMode?0:1; //�л�ս��ģʽ����
					DisplayLockedTIM=2; //ָʾս���л�
					}
			if(IsTacMode) //ս��ģʽ����ʱ�����ж�
					{
					if(!getSideKeyHoldEvent())break;
					if(Battery>3.1&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //��ص���������û�й�������������������
					else if(Battery>2.7)SwitchToGear(Mode_High);  //��ص�ص�������ʱ�������
					else LEDMode=LED_RedBlinkFifth; //��ص������ز��㣬��ɫ�����
					break;
					}
		  //�����������������ػ�������
			if(ClickCount==1||TKCount) //�ఴ������������ѭ��
					{
				  if(Battery>2.9)SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode); //��������
					else if(Battery>2.7)SwitchToGear(Mode_Moon);	 //����2.7V��ʱ��ֻ�ܿ��¹�
					else LEDMode=LED_RedBlinkFifth; //��ص������ز��㣬��ɫ�����
					}			
			else if(ClickCount==2)  //˫��һ������		
					{
					if(IsDisableTurbo)LEDMode=LED_RedBlinkFifth; //�ֵ��¶ȹ���������������ʾ�޷�����
				  else if(Battery>3.1)SwitchToGear(Mode_Turbo); //��ص���������������
					else if(Battery>2.7)SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode);  //��ص�ص�������ʱ˫��������ͨģʽ
					else LEDMode=LED_RedBlinkFifth; //�������������˸��ʾ
					}
      else if(IsHoldEvent)SwitchToGear(Mode_Moon); //��������ֱ�ӽ��¹�					
			else if(ClickCount==3)//�ఴ�������뱬�� 
					{
			    if(Battery>2.7)SwitchToGear(Mode_Strobe);   //���뱬��
					else LEDMode=LED_RedBlinkFifth; //�������������˸��ʾ
					}
			else if(ClickCount==4) //�Ļ��л���λģʽ���޼�����
					{	
					IsRampEnabled=IsRampEnabled?0:1; //ת���޼�����״̬	
					LEDMode=!IsRampEnabled?LED_RedBlinkThird:LED_GreenBlinkThird; //��ʾ�Ƿ���
					SaveRampConfig(0); //�������õ�ROM��
					}
			else if(getSideKeyClickAndHoldEvent())TriggerVshowDisplay(); //���������鿴��ص�ǰ��ѹ�͵���
  		break;
		//�¹�״̬
		 case Mode_Moon:
			 IsHalfBrightness=1; //�¹�ģʽ���������ȼ���
			 BatteryLowAlertProcess(true,Mode_Moon);
		   EnterTurboStrobe(TKCount,0); //β��ģʽ�£���Ҫ����һ�����뼫�����߱������Լ��ϼ��
			 if(ClickCount==1)ReturnToOFFState(); //�ص��ػ�״̬				
			 //��ص�ѹ���㣬�������������λ
		   if((IsHoldEvent||TKCount==1)&&Battery>2.9)  
					{
					SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����ص�������λģʽ
					if(TKCount==1)break; //β������ֱ������ת��
					if(IsRampEnabled)RestoreToMinimumRampCurrent(); //������޼�������ָ�����͵���
					HoldChangeGearTIM|=0x40; //��ʱ���ڽ�ֹ����������ȷ��Ҫ�û��ɿ�����ܻ�
					RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5������л�
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
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
		    //������������
		    SideKeySwitchGearHandler(Mode_SOS,TKCount); //�����л���SOS
		    break;	
    //SOS��ȵ�λ		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
		    //������������
		    SideKeySwitchGearHandler(Mode_Strobe,TKCount); //�����л�������
		    break;	
		}
  //Ӧ���������
	if(DisplayLockedTIM>0)Current=80; //�û���������˳���������80mA���ݵ�����ʾһ��
	else if(RampCfg.RampMaxDisplayTIM>0||LowPowerStrobe())Current=-1; //�޼�����ģʽ�ڵִ������޺����Ϩ��(-1������ʾ���رշ�����FET)
	else switch(CurrentMode->ModeIdx)	
		{
		case Mode_Strobe:Current=StrobeFlag?CurrentMode->Current:-1;break; //����ģʽ���ݱ���flag���ز���
		case Mode_Ramp://�޼�����ģʽȡ�޼���������ṹ���ڵĵ���
			Current=RampCfg.CurrentLimit<RampCfg.Current?RampCfg.CurrentLimit:RampCfg.Current;
		  break;
		case Mode_SOS:Current=SOSFSM();break; //SOSģʽ�����������SOS״̬������
		default:Current=CurrentMode->Current;break; //������λʹ������ֵ��ΪĿ�����
		}	
	//�����������
	getSideKeyShortPressCount(1); 
	//ROM��λ�������
	if((IsPOSTKPressed&&TKCount>0)||TailSaveTIM==24) //�ڵ�λͣ����ʱ���㹻���߿���ʱ�����˵�λ���أ���������
		{
		IsPOSTKPressed=0; //��λ���Ϊ
		TailSaveTIM++;
		TailMemory_Save(CurrentMode->ModeIdx);
		}
	}