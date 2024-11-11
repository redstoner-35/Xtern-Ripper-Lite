#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "delay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "cms8s6990.h"
#include "PWMCfg.h"

//PI����������С��������
#define ProtFullScale 18000 //PI�������ϸ��ֵ
#define IntegrateFullScale 12000 //���ֵ�Full Scale
#define IntegralFactor 150 //����ϵ��(Խ��ʱ�䳣��Խ��)
#define MinumumILED 900 //����ϵͳ���ܴﵽ����͵���(mA)

//�¶�����
#define ForceOffTemp 75 //���ȹػ��¶�
#define ForceDisableTurboTemp 60 //�������¶��޷����뼫��
#define ConstantTemperature 52 //�¿�������ά�ֵ��¶�

//�¶ȿ�����ȫ�ֱ���
static int TempIntegral=0;
static int TempProtBuf=0;
bit IsTempLIMActive=0;  //�¿��Ƿ��Ѿ�����
bit IsDisableTurbo=0;  //��ֹ�ٶȽ��뵽������
bit IsForceLeaveTurbo=0; //�Ƿ�ǿ���뿪������

//�ϵ�ʱ���NTC״̬
void CheckNTCStatus(void)
	{
	char i=64;
	//����¶�����
  do
		{
		delay_ms(10);
		SystemTelemHandler();
		if(Data.IsNTCOK)break; //NTC�Ѿ������������˳����
		i--;
		}		
	while(i);
	if(!i) //����0.64��ĵȴ���Ȼ����꣬����
		{
		LEDMode=LED_Amber; 
		LEDControlHandler(); //NTC�Լ첻ͨ�����ƵƳ���
		while(1); //��ѭ��
		}
	}

//�����ͻ����������ٷֱ��޷�
static float PresentLIM(float IN)
	{
	if(IN>100)return 100;
  if(IN<0)return 0;  
	//�Ϸ���ֵԭ·����
	return IN;
	}	
	
//�������ֵ�İٷֱ�
int ThermalILIMCalc(int Input)
	{
	float buf,ILED,itgbuf;
	//�¿ر���ֹ���ߴ���ĵ���С�ڵ���0��������ٵ����ͷ��ض���	
	if(!IsTempLIMActive||Input<=0)return Input;
	//���ӱ�����
	buf=(float)TempProtBuf/(float)ProtFullScale; //���ɱ�����
	buf*=100;
  buf=PresentLIM(buf);
	//���ӻ�����
	itgbuf=(float)TempIntegral/(float)IntegrateFullScale; //���������
	buf+=itgbuf*10;//��������ϵĻ�������뵽�������У�������10%�Ĺ��ʲ�����
	buf=PresentLIM(buf); //�޷�
	//����������ʹ���ĵ���ֵ���м���	
	if(Input<=MinumumILED)return MinumumILED; //��������������С�������ϸ��ֵ
	ILED=(float)(Input-MinumumILED)/(float)100; //���ϸ��ֵ
	ILED*=(100-buf); //�������͵���ֵ����Ŀ�����ֵ֮���������
	return MinumumILED+(int)ILED; //����ʵ�ʵĵ���ֵ
	}
	
//�¿ؼ��㺯��
void ThermalCalcProcess(void)
	{
	int Err;
	//�¶ȴ���������
	if(!Data.IsNTCOK)
		{
		ErrCode=Fault_NTCFailed; //��д�������
    if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
		return;
		}
	//��Ͳͷ�¶ȹ���ʱ���رռ�����	
	if(Data.Systemp>(ForceOffTemp-10))IsForceLeaveTurbo=1;  //�¶Ⱦ���ػ������ļ�಻��10�ȣ������˳�����
	if(Data.Systemp>ForceDisableTurboTemp)IsDisableTurbo=1;
	else if(Data.Systemp<(ForceDisableTurboTemp-10))IsDisableTurbo=0;
	if(IsForceLeaveTurbo&&!IsDisableTurbo)IsForceLeaveTurbo=0;	 //���ǿ���˳�������־λ��λ���¶��Ѿ����䵽������������ֵ�㣬��λ
	//���ȹ���
	if(Data.Systemp>ForceOffTemp)
		{
		ErrCode=Fault_OverHeat; //��д�������
    if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //ָʾ���Ϸ���
		return;
		}
	else if(Data.Systemp<(ForceOffTemp-20)&&ErrCode==Fault_OverHeat)
		{
	  ErrCode=Fault_None;
	  SwitchToGear(Mode_OFF); //�¶Ȼ��䣬��������ָʾ
		}
	//PI��ʹ�ܿ���
	if(!CurrentMode->IsNeedStepDown)IsTempLIMActive=0; //��ǰ��λ����Ҫ����
	else if(Data.Systemp>ConstantTemperature)IsTempLIMActive=1;
	else if(Data.Systemp<(ConstantTemperature-10))IsTempLIMActive=0; //�ͻؿ���
	//PI���رգ���λ��ֵ
	if(!IsTempLIMActive)
		{
		TempIntegral=0;
		TempProtBuf=0;
		}
	//����PI����(�������������ʱ�����)
	else if(Current>0)
		{
		//�����
		Err=Data.Systemp-ConstantTemperature;
		//������(P)
		TempProtBuf+=(Err>1)?Err*(iabsf(Current/6000)+1):0; //��̬�������������
    if(TempProtBuf>ProtFullScale)TempProtBuf=ProtFullScale;
    if(TempProtBuf<0)TempProtBuf=0;  //���Ʒ���
    //������(I)
    TempIntegral+=Err; //�ۼ����
    if(TempIntegral>IntegrateFullScale)TempIntegral=IntegrateFullScale;
		if(TempIntegral<-IntegrateFullScale)TempIntegral=-IntegrateFullScale;  //�����޷�
		}
	}	
