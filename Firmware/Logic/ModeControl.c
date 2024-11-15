#include "ModeControl.h"
#include "LEDMgmt.h"
#include "SideKey.h"
#include "BattDisplay.h"
#include "OutputChannel.h"
#include "RampConfig.h"
#include "ADCCfg.h"
#include "cms8s6990.h"

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
		15000,  //15A����
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
		3100,  //3.1V�ض�
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
bit IsSideLEDCfgMode; //�ఴLED����ģʽ
static xdata SOSStateDef SOSState; //ȫ�ֱ���״̬λ
xdata FaultCodeDef ErrCode; //�������	
	
//�����ʱ����
xdata char BattAlertTimer=0; //��ص͵�ѹ�澯����
xdata char HoldChangeGearTIM=0; //��λģʽ�³�������
xdata char DisplayLockedTIM=0; //������ս��ģʽ�����˳���ʾ	
xdata char ClickHoldReverseGearTIM=0; //��λģʽ�µ���+�������򻻵�
xdata	char MoonCfgTIM=0; //�¹⵲λ���ü�ʱ
xdata char SOSTIM=0;  //SOS��ʱ
xdata char RampRiseCurrentTIM=0; //�޼�����ָ������ļ�ʱ��	
	
//��ʼ��ģʽ״̬��
void ModeFSMInit(void)
{
	char i;
	//��ʼ���޼�����
	RampCfg.RampMaxDisplayTIM=0;
  for(i=0;i<ModeTotalDepth;i++)if(ModeSettings[i].ModeIdx==Mode_Ramp)
	    {
			RampCfg.BattThres=ModeSettings[i].LowVoltThres; //��ѹ������޻ָ�
			RampCfg.CurrentLimit=ModeSettings[i].Current; //�ҵ���λ�������޼�����ĵ�λ���������޻ָ�
			}
	ReadRampConfig(); //��EEPROM�ڶ�ȡ�޼���������
	//��λģʽ����
	SOSState=SOSState_Prepare; //SOS״̬������Ϊ��ʼֵ
	LastMode=Mode_Low;
	ErrCode=Fault_None; //û�й���
	CurrentMode=&ModeSettings[0]; //��������Ϊ��һ����
	IsLocked=0; //�ر�����
	IsEnableMoonConfigMode=0;
	IsSideLEDCfgMode=0; //������ģʽ
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

//��λ״̬������������ʱ������
void ModeFSMTIMHandler(void)
{
	char buf;
	//SOS��ʱ��
	if(SOSTIM>0)SOSTIM--;
	//�޼�������صĶ�ʱ��
	if(RampRiseCurrentTIM>0&&RampRiseCurrentTIM<9)RampRiseCurrentTIM++;
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
	
//�޼�����ĵ͵�ѹ����
void RampLowVoltHandler(void)
	{
	char time;
	extern xdata BattStatusDef BattState;
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
//�ఴָʾ���������ú���
void SideKeyLEDBriAdjHandler(void)
	{
	static xdata bool SideLEDRampDir=false;
	static xdata char SpeedDIV=8;
	//��ǰռ�ձ����ڵ���
	if(LEDMgmt_WaitSubmitDuty())return;
	//�����ٶȵķ�Ƶ	
	SpeedDIV--;	
	if(SpeedDIV)return;
	SpeedDIV=8;
	//�ӵ�ramp����
	if(!SideLEDRampDir)
		{
		if(LEDBrightNess<2399)LEDBrightNess++;
		else SideLEDRampDir=true; //��ת״̬
		}
	//�Ӹ�Ramp����
	else
		{
		if(LEDBrightNess>50)LEDBrightNess--;
		else SideLEDRampDir=false;
		}
		LEDMgmt_SetBrightness(); //�����ĺ�����ȱ���
	}				 	
	
//�޼����⴦��
static void RampAdjHandler(void)
	{
	static bit IsKeyPressed=0;	
  int Limit;
	bit IsPress;
  //������޼���������
	IsPress=(getSideKeyClickAndHoldEvent()||getSideKeyHoldEvent())?1:0;
	Limit=RampCfg.CurrentLimit<CurrentMode->Current?RampCfg.CurrentLimit:CurrentMode->Current;
	if(Limit<CurrentMode->Current&&IsPress&&RampCfg.Current>Limit)RampCfg.Current=Limit; //�ڵ��������Ƶ�������û����°������Ե��������������޷�
	//�������ȵ���
	if(getSideKeyHoldEvent()&&!IsKeyPressed)RampCfg.Current+=3; //�������ӻ��߼��ٵ���
	else if(getSideKeyClickAndHoldEvent()&&!IsKeyPressed)RampCfg.Current-=3; //���ӻ��߼��ٵ���	
  else if(!getSideKeyClickAndHoldEvent()&&!getSideKeyHoldEvent()&&IsKeyPressed)IsKeyPressed=0; //�û��ſ��������������		
	//�����ﵽ����
	if(getSideKeyHoldEvent()&&!IsKeyPressed&&RampCfg.Current>=Limit)
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
//����Ƿ���Ҫ�ػ�
static void DetectIfNeedsOFF(int ClickCount)
	{
	if(ClickCount!=1)return;
	if(!IsTacMode&&getSideKeyHoldEvent())return;
	ReturnToOFFState();//�ఴ����������ս��ģʽ���ɿ���ťʱ�ػ�
	}	
	
//��ȡ�¹⵵����
static int ObtainMoonCurrent(void)	
	{
	switch(MoonCfg)
		{
		case MoonLight_10mA:return 10;  //10mA
		case MoonLight_25mA:return 25;  //25mA
		case MoonLight_50mA:return 50;  //50mA
		case MoonLight_100mA:return 100; //100mA
		case MoonLight_200mA:return 200; //200mA
    }
	//�����������Ĭ��ֵ
	return CurrentMode->Current;
	}
	
//PI��·���¿����ݴ�������
int ThermalILIMCalc(int Input);
	
//��λ״̬��
void ModeSwitchFSM(void)
	{
	bit IsHoldEvent;
	int ClickCount;
	xdata float TargetCurrent; //��ǰĿ�����	
	//�ⲿ��������
	extern volatile bit StrobeFlag;
	extern bit IsDisableTurbo;
	extern bit IsForceLeaveTurbo;
	//��ȡ����״̬
	IsHoldEvent=getSideKeyLongPressEvent();	
	ClickCount=getSideKeyShortPressCount(1);	//��ȡ�����������������Ĳ���
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
			if(ClickCount==1) //�ఴ������������ѭ��
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
			 BatteryLowAlertProcess(true,Mode_Moon);
			 if(ClickCount==1)//�ఴ�����ػ�
					{
					if(IsEnableMoonConfigMode||IsSideLEDCfgMode)SaveRampConfig(0); //�¹�Ͳఴ���ȷ����������������õ�ROM��
			    IsEnableMoonConfigMode=0;
			    IsSideLEDCfgMode=0;
					ReturnToOFFState(); //�ص��ػ�״̬
					}
			 if(ClickCount==5&&!IsSideLEDCfgMode)IsSideLEDCfgMode=1;	 //��������ఴLED����	
			 //���òఴLED��������
       if(IsSideLEDCfgMode)SideKeyLEDBriAdjHandler();
			 if(IsEnableMoonConfigMode||IsSideLEDCfgMode)break; //��������ģʽ����ֹ��Ӧ
			 //������ģʽ������Ĳ���
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
		    DetectIfNeedsOFF(ClickCount); //����Ƿ���Ҫ�ػ�
				if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //�޼����⴦��
		    RampLowVoltHandler(); //�͵�ѹ����
        RampAdjHandler();			
		    break;
    //����״̬		
    case Mode_Low:
			  BatteryLowAlertProcess(true,Mode_Low);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_Mid); //�����е�
		    break;	    		
    //����״̬		
    case Mode_Mid:
			  BatteryLowAlertProcess(false,Mode_Low);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_MHigh); //�����иߵ�
		    SideKey1HRevGearHandler(Mode_Low); //����+�������˵�λ���͵�
		    break;	
	  //�и���״̬
    case Mode_MHigh:
			  BatteryLowAlertProcess(false,Mode_Mid);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_High); //�����ߵ�
		    SideKey1HRevGearHandler(Mode_Mid); //����+�������˵�λ���е�
		    break;	
	  //����״̬
    case Mode_High:
			  BatteryLowAlertProcess(false,Mode_MHigh);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
		    if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    //������������
		    SideKeySwitchGearHandler(Mode_Low); //�����͵�λ����ѭ��
		    SideKey1HRevGearHandler(Mode_MHigh); //����+�������˵�λ���иߵ�
		    break;
		//����״̬
    case Mode_Turbo:
			  BatteryLowAlertProcess(false,Mode_High);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
			  if(ClickCount==2||IsForceLeaveTurbo)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //˫�������¶ȴﵽ����ֵ��ǿ�Ʒ��ص�����
				if(ClickCount==3)SwitchToGear(Mode_Strobe); //�ఴ3�����뱬��
		    break;	
		//����״̬
    case Mode_Strobe:
			  BatteryLowAlertProcess(true,Mode_Strobe);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
		    //������������
		    SideKeySwitchGearHandler(Mode_SOS); //�����л���SOS
		    break;	
    //SOS��ȵ�λ		
		case Mode_SOS:
			  BatteryLowAlertProcess(true,Mode_SOS);
		    DetectIfNeedsOFF(ClickCount); //ִ�йػ��������
			  if(ClickCount==2&&!IsDisableTurbo)SwitchToGear(Mode_Turbo); //˫������
				if(ClickCount==3)SwitchToGear(IsRampEnabled?Mode_Ramp:Mode_Low); //�����˻ص���ͨģʽ
		    //������������
		    SideKeySwitchGearHandler(Mode_Strobe); //�����л�������
		    break;	
		}
  //Ӧ���������
	if(DisplayLockedTIM>0)TargetCurrent=80; //�û���������˳���������80mA���ݵ�����ʾһ��
	else if(RampCfg.RampMaxDisplayTIM>0)TargetCurrent=-1; //�޼�����ģʽ�ڵִ������޺����Ϩ��(-1������ʾ���رշ�����FET)
	else switch(CurrentMode->ModeIdx)	
		{
		case Mode_Strobe:TargetCurrent=StrobeFlag?CurrentMode->Current:-1;break; //����ģʽ���ݱ���flag���ز���
		case Mode_Ramp://�޼�����ģʽȡ�޼���������ṹ���ڵĵ���
			TargetCurrent=RampCfg.CurrentLimit<RampCfg.Current?RampCfg.CurrentLimit:RampCfg.Current;
		  break;
		case Mode_SOS:TargetCurrent=SOSFSM();break; //SOSģʽ�����������SOS״̬������
		case Mode_Moon:TargetCurrent=ObtainMoonCurrent();break; //�¹�ģʽ���ض�Ӧ�ĵ���
		default:TargetCurrent=CurrentMode->Current; //Ŀ�����
		}	
	//�����¿ص���������������������޷�
	Current=ThermalILIMCalc(TargetCurrent);	
	}