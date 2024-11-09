#include "ModeControl.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"

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
		false
		}, 
		//������
		{
		Mode_Fault,
		0,
		0,  //����0mA
		0,
		false,
		false
		}, 
		//�¹�
		{
		Mode_Moon,
		350,  //Ĭ��350mA����
		0,   //��С����û�õ�������
		2800,  //2.8V�ض�
		false, //�¹⵵��ר����ڣ����������
		false
		}, 	
    //����
		{
		Mode_Low,
		800,  //800mA����
		0,   //��С����û�õ�������
		2900,  //2.8V�ض�
		true,
		false
		},
    //����
		{
		Mode_Mid,
		1500,  //1500mA����
		0,   //��С����û�õ�������
		3000,  //3V�ض�
		true,
		false
		}, 	
    //�и���
		{
		Mode_MHigh,
		3000,  //3000mA����
		0,   //��С����û�õ�������
		3050,  //3.05V�ض�
		true,
		true
		}, 	
    //����
		{
		Mode_High,
		7500,  //7500mA����
		0,   //��С����û�õ�������
		3100,  //3.1V�ض�
		true,
		true
		}, 	
    //����
		{
		Mode_Turbo,
		14000,  //14000mA����
		0,   //��С����û�õ�������
		3200,  //3.2V�ض�
		false, //�������ܴ�����
		true
		}, 	
    //����		
		{
		Mode_Strobe,
		10000,  //10000mA����
		0,   //��С����û�õ�������
		3000,  //3.0V�ض�
		false, //�������ܴ�����
		true
		}, 
	  //�޼�����		
		{
		Mode_Ramp,
		7500,  //7500mA�������
		500,   //��С500mA
		3000,  //3.0V�ض�
		false, //���ܴ�����  
		true
		}, 
	  //SOS
		{
		Mode_SOS,
		10000,  //10000mA����
		0,   //��С����û�õ�������
		3000,  //3.0V�ض�
		false,	//SOS���ܴ�����
		true
		}, 
	};

//ȫ�ֱ���(��λ)
ModeStrDef *CurrentMode; //��λ�ṹ��ָ��
xdata ModeIdxDef LastMode; //����Ϊ����
RampConfigDef RampCfg; //�޼���������		
xdata MoonLightBrightnessDef MoonCfg;	 //�¹�ģʽ����
	
//ȫ�ֱ���(״̬λ)
bit IsRampEnabled; //�Ƿ����޼�����
bit IsLocked; //����ָʾ
bit IsTacMode; //����ս��ģʽ
bit IsEnableMoonConfigMode; //���¹�����ģʽ
static xdata SOSStateDef SOSState; //ȫ�ֱ���״̬λ
xdata FaultCodeDef ErrCode; //�������	
xdata float TargetCurrent; //��ǰĿ�����	
	
//������ʱ����
xdata char BattAlertTimer=0; //��ص͵�ѹ�澯����
xdata char HoldChangeGearTIM=0; //��λģʽ�³�������
xdata char DisplayLockedTIM=0; //������ս��ģʽ�����˳���ʾ	
xdata char ClickHoldReverseGearTIM=0; //��λģʽ�µ���+�������򻻵�
xdata	char MoonCfgTIM=0; //�¹⵲λ���ü�ʱ
xdata char SOSTIM=0;  //SOS��ʱ
	
//��ʼ��ģʽ״̬��
void ModeFSMInit(void)
{
	RampCfg.RampMaxDisplayTIM=0;
	ReadRampConfig(); //��EEPROM�ڶ�ȡ�޼���������
	//��λģʽ����
	SOSState=SOSState_Prepare; //SOS״̬������Ϊ��ʼֵ
	LastMode=Mode_Low;
	ErrCode=Fault_None; //û�й���
	CurrentMode=&ModeSettings[0]; //��������Ϊ��һ����
	IsLocked=0; //�ر�����
	IsTacMode=0; //�˳�ս��ģʽ
}	

//SOS����ģ��
static int SOSFSM(void)
{
  int buf;
	switch(SOSState)
		{
		//׼���׶�
		case SOSState_Prepare:
			 SOSTIM=(3*SOSDotTime*2)-1;
			 SOSState=SOSState_3Dot;
		   break;
		//��һ������
		case SOSState_3Dot:
		   buf=SOSTIM%(SOSDotTime*2); //���ݲ������û����ʱ������ʱ��
       if(buf>(SOSDotTime-1))return CurrentMode->Current; //��ǰ״̬��ҪLED����������Ŀ�����ֵ		
		   if(SOSTIM==0) //��ʾ����
				 {
				 SOSTIM=SOSGapTime; 
				 SOSState=SOSState_3DotWait;  //������ʱ�ȴ��׶�
				 }
		   break;
		//���������ĵȴ���ʱ�׶�
	  case SOSState_3DotWait:
			 if(SOSTIM>0)break;
		   SOSTIM=(3*SOSDashTime*2)-1;
		   SOSState=SOSState_3Dash;
		   break;
		//����
		case SOSState_3Dash:
			 buf=SOSTIM%(SOSDashTime*2); //���ݲ������û����ʱ������ʱ��
       if(buf>(SOSDashTime-1))return CurrentMode->Current; //��ǰ״̬��ҪLED����������Ŀ�����ֵ	
		   if(SOSTIM==0) //��ʾ����
				 {
				 SOSTIM=SOSGapTime; 
				 SOSState=SOSState_3DashWait;  //������ʱ�ȴ��׶�
				 }
		   break;			
		//����������ĵȴ���ʱ�׶�
	  case SOSState_3DashWait:
			 if(SOSTIM>0)break;
			 SOSTIM=(3*SOSDotTime*2)-1;
			 SOSState=SOSState_3DotAgain;
		   break;		
		//�ڶ�������
		case SOSState_3DotAgain:
			 buf=SOSTIM%(SOSDotTime*2); //���ݲ������û����ʱ������ʱ��
       if(buf>(SOSDotTime-1))return CurrentMode->Current; //��ǰ״̬��ҪLED����������Ŀ�����ֵ		
		   if(SOSTIM==0) //��ʾ����
				 {
				 SOSTIM=SOSFinishGapTime; 
				 SOSState=SOSState_Wait;  //������ʱ�ȴ��׶�
				 }
		   break;		
	  //�����źŷ�����ϣ��ȴ�
	  case SOSState_Wait:	
			 if(SOSTIM>0)break;
		   SOSState=SOSState_Prepare; //�ص�׼��״̬
		   break;
		}
	//�����������-1
	return -1;
}

//�¹⵲λѭ�����ù���
void MoonConfigHandler(void)
{
	int buf;
	//���¹�ģʽ��������δ�򿪣���ֹ����
	if(!IsEnableMoonConfigMode||CurrentMode->ModeIdx!=Mode_Moon)MoonCfgTIM=0; 
	//��������ģʽ��ѭ������
  else
		{
		MoonCfgTIM++;
		if(MoonCfgTIM<16)return;
		MoonCfgTIM=0;
		//��ʼ����������ѭ���¹⵲λ��index�Թ���ѭ��
		buf=(int)MoonCfg;
		if(buf<(int)MoonLight_UsingModeDef)buf++;
		else buf=0;
		MoonCfg=(MoonLightBrightnessDef)buf; //����ѭ��index
		}
}

//��λ״̬�������������ʱ������
void ModeFSMTIMHandler(void)
{
	char buf;
	//SOS��ʱ��
	if(SOSTIM>0)SOSTIM--;
	//�޼�������ʾ��ʱ��
	if(RampCfg.CfgSavedTIM<32)RampCfg.CfgSavedTIM++;
	if(RampCfg.RampMaxDisplayTIM>0)RampCfg.RampMaxDisplayTIM--;
	//����������ʾ��ʱ��
  if(DisplayLockedTIM>0)DisplayLockedTIM--;
	//��ⶨʱ��״̬
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
  int i;
	extern xdata float VBattBeforeTurbo;
	ModeIdxDef BeforeMode=CurrentMode->ModeIdx; //�洢��ǰģʽ	
	for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==TargetMode)
		{
		SOSState=SOSState_Prepare; //ÿ�λ�������SOS״̬������Ϊ��ʼֵ
		CurrentMode=&ModeSettings[i]; //�ҵ�ƥ��index����ֵ�ṹ��
		if(TargetMode==Mode_Turbo&&BeforeMode!=Mode_Turbo)VBattBeforeTurbo=Data.RawBattVolt; //�л���turboģʽʱ���в���
		return 0;
		}
	//ɶҲû�ҵ�������
	return 1;
	}

//�����ػ�����	
void ReturnToOFFState(void)
	{
	if(CurrentMode->ModeIdx==Mode_OFF)return; //�ػ�״̬��ִ��		
	if(CurrentMode->IsModeHasMemory)LastMode=CurrentMode->ModeIdx; //�洢�ػ�ǰ�ĵ�λ
	SwitchToGear(Mode_OFF); //ǿ�����ص��ػ���λ
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
	if(!getSideKeyHoldEvent()||CurrentMode->ModeIdx==Mode_OFF)HoldChangeGearTIM=0; //�����ɿ������ǹػ�״̬����ʱ����λ
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
  if(!getSideKeyClickAndHoldEvent()||CurrentMode->ModeIdx==Mode_OFF)ClickHoldReverseGearTIM=0;	//�����ɿ������ǹػ�״̬����ʱ����λ	
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
static void SideKeySwitchGearHandler(ModeIdxDef TargetMode)	
	{
	if(!(HoldChangeGearTIM&0x80))return;
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
static void RampAdjHandler(void)
	{
	static bit IsKeyPressed=0;		
	//�������ȵ���
	if(getSideKeyHoldEvent()&&!IsKeyPressed)RampCfg.Current+=3; //�������ӻ��߼��ٵ���
	else if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed)RampCfg.Current-=3; //���ӻ��߼��ٵ���	
  else if(!getSideKeyClickAndHoldEvent()&&!getSideKeyHoldEvent()&&IsKeyPressed)IsKeyPressed=0; //�û��ſ���������������		
	//�����ﵽ����
	if(getSideKeyHoldEvent()&&!IsKeyPressed&&RampCfg.Current>=CurrentMode->Current)
			{
			RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5��ָʾ�Ѿ�������
			RampCfg.Current=CurrentMode->Current; //���Ƶ������ֵ	
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
	if(getSideKeyHoldEvent()||getSideKeyClickAndHoldEvent())RampCfg.CfgSavedTIM=0; //��������˵�����ڵ�������λ��ʱ��
	else if(RampCfg.CfgSavedTIM==32)
			{
			RampCfg.CfgSavedTIM++;
			SaveRampConfig(0);  //һ��ʱ����û����˵���Ѿ�������ϣ���������
			}
	}
//PI��·���¿����ݴ�������
int ThermalILIMCalc(int Input);
	
//��λ״̬��
void ModeSwitchFSM(void)
	{
	bit IsHoldEvent;
	int ClickCount;
	//�ⲿ��������
	extern volatile bit StrobeFlag;
	extern bit IsDisableTurbo;
	extern bit IsForceLeaveTurbo;
	//��ȡ����״̬
	IsHoldEvent=getSideKeyLongPressEvent();	
	ClickCount=getSideKeyShortPressCount(1);	//��ȡ�������������������Ĳ���
	//��λ����������
	if(LastMode==Mode_OFF||LastMode>=ModeTotalDepth)LastMode=Mode_Low;
	//״̬��
	if(ErrCode==Fault_DCDCFailedToStart||ErrCode==Fault_DCDCENOOC)return; //������ʼ������
	else switch(CurrentMode->ModeIdx)	
		{
		//���ִ���	
		case Mode_Fault:
      IsTacMode=0; //���Ϻ��Զ�ȡ��ս��ģʽ			
			if(!IsHoldEvent||ErrCode==Fault_OverHeat)break; //�û�û�а��°�ť�����ǹ���״̬����������
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
			//������״̬��������������
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
			if(ClickCount==1) //�ఴ������������ѭ��
					{
				  if(Battery>2.9)SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode); //��������
					else if(Battery>2.7)SwitchToGear(Mode_Moon);	 //����2.7V��ʱ��ֻ�ܿ��¹�
					else LEDMode=LED_RedBlinkFifth; //��ص������ز��㣬��ɫ�����
					}			
			else if(ClickCount==2)  //˫��һ������		
					{
					if(IsDisableTurbo)break; //�ֵ��¶ȹ�������������������Ӧ
				  else if(Battery>3.1)SwitchToGear(Mode_Turbo); //��ص���������������
					else SwitchToGear(IsRampEnabled?Mode_Ramp:LastMode);  //��ص�ص�������ʱ˫��������ͨģʽ
					}
      else if(IsHoldEvent)SwitchToGear(Mode_Moon); //��������ֱ�ӽ��¹�					
			else if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ�������뱬��   
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
			 BatteryLowAlertProcess(true,Mode_Moon);
			 if(ClickCount==1)//�ఴ�����ػ�
					{
					if(IsEnableMoonConfigMode)SaveRampConfig(0); //�¹����ȷ����������������õ�ROM��
			    IsEnableMoonConfigMode=0;
			    ReturnToOFFState(); //�ص��ػ�״̬
					}
			 if(IsEnableMoonConfigMode)break; //��������ģʽ����ֹ��Ӧ
			 //������ģʽ�������Ĳ���
			 if(ClickCount==4)IsEnableMoonConfigMode=1; //�Ļ���������ģʽ
		   if(IsHoldEvent&&Battery>2.9)  //��ص�ѹ���㣬�������������λ
					{
					SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����ص�������λģʽ
					if(IsRampEnabled)RestoreToMinimumRampCurrent(); //������޼�������ָ�����͵���
					HoldChangeGearTIM|=0x40; //��ʱ���ڽ�ֹ����������ȷ��Ҫ�û��ɿ�����ܻ�
					RampCfg.RampMaxDisplayTIM=4; //Ϩ��0.5������л�
					}		    
		    break;			
    //�޼�����״̬				
    case Mode_Ramp:
			  BatteryLowAlertProcess(true,Mode_Ramp);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
				if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //�޼����⴦��
        RampAdjHandler();			
		    break;
    //����״̬		
    case Mode_Low:
			  BatteryLowAlertProcess(true,Mode_Low);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_Mid); //�����е�
		    break;	    		
    //����״̬		
    case Mode_Mid:
			  BatteryLowAlertProcess(false,Mode_Low);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_MHigh); //�����иߵ�
		    SideKey1HRevGearHandler(Mode_Low); //����+�������˵�λ���͵�
		    break;	
	  //�и���״̬
    case Mode_MHigh:
			  BatteryLowAlertProcess(false,Mode_Mid);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_High); //�����ߵ�
		    SideKey1HRevGearHandler(Mode_Mid); //����+�������˵�λ���е�
		    break;	
	  //����״̬
    case Mode_High:
			  BatteryLowAlertProcess(false,Mode_MHigh);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_Low); //�����͵�λ����ѭ��
		    SideKey1HRevGearHandler(Mode_MHigh); //����+�������˵�λ���иߵ�
		    break;
		//����״̬
    case Mode_Turbo:
			  BatteryLowAlertProcess(false,Mode_High);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
			  if(ClickCount==2||IsForceLeaveTurbo)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //˫�������¶ȴﵽ����ֵ��ǿ�Ʒ��ص�����
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    break;	
		//����״̬
    case Mode_Strobe:
			  BatteryLowAlertProcess(true,Mode_Strobe);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
		    //������������
		    SideKeySwitchGearHandler(Mode_SOS); //�����л���SOS
		    break;	
    //SOS��ȵ�λ		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    if(ClickCount==1||(IsTacMode&&!getSideKeyHoldEvent()))ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
		    //������������
		    SideKeySwitchGearHandler(Mode_Strobe); //�����л�������
		    break;	
		}
  //Ӧ���������
	if(DisplayLockedTIM>0)TargetCurrent=80; //�û���������˳���������80mA���ݵ�����ʾһ��
	else if(!StrobeFlag&&CurrentMode->ModeIdx==Mode_Strobe)TargetCurrent=-1;	 //����ģʽ���õ�������flag(-1������ʾ���رշ�����FET)
	else if(RampCfg.RampMaxDisplayTIM>0)TargetCurrent=-1; //�޼�����ģʽ�ڵִ������޺����Ϩ��(-1������ʾ���رշ�����FET)
  else if(CurrentMode->ModeIdx==Mode_Ramp)TargetCurrent=RampCfg.Current; //�޼�����ģʽȡ�޼���������ṹ���ڵĵ���
	else if(CurrentMode->ModeIdx==Mode_SOS)TargetCurrent=SOSFSM(); //SOSģʽ�����������SOS״̬������
	else if(CurrentMode->ModeIdx==Mode_Moon)switch(MoonCfg)
		{
		case MoonLight_10mA:TargetCurrent=10;break;  //10mA
		case MoonLight_25mA:TargetCurrent=25;break;  //25mA
		case MoonLight_50mA:TargetCurrent=50;break;  //50mA
		case MoonLight_100mA:TargetCurrent=100;break; //100mA
		case MoonLight_200mA:TargetCurrent=200;break; //200mA
		case MoonLight_UsingModeDef:TargetCurrent=CurrentMode->Current;break; //ʹ��ģʽ�ṹ���ڵĽ��
		}
  else TargetCurrent=CurrentMode->Current;		
	//�����¿ص���������������������޷�
	Current=ThermalILIMCalc(TargetCurrent);	
	}