#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "delay.h"
#include "ModeControl.h"
#include "TempControl.h"
#include "OutputChannel.h"
#include "cms8s6990.h"
#include "PWMCfg.h"
#include "Strap.h"
#include "SelfTest.h"

//�¶ȿ�����ȫ�ֱ���
static xdata int TempIntegral=0;
static xdata int TempProtBuf=0;
static bit IsTempLIMActive=0;  //�¿��Ƿ��Ѿ�����
static char Err=0; //ȫ�ֱ�����PI�������ֵ

//״̬λ
bit IsThermalStepDown=0; //���λ���Ƿ񽵵�
bit IsDisableTurbo=0;  //��ֹ�ٶȽ��뵽������
bit IsForceLeaveTurbo=0; //�Ƿ�ǿ���뿪������
bit IsSystemShutDown=0; //�Ƿ񴥷��¿�ǿ�ƹػ�

//������ʱ����ݵ�ǰ���µĵ�������PIֵ
void RecalcPILoop(int LastCurrent)	
	{
	int buf,ModeCur;
	//Ŀ�굲λ����Ҫ����
	if(!CurrentMode->IsNeedStepDown)return;
	//��ȡ��ǰ��λ����
	ModeCur=QueryCurrentGearILED();
	//����Pֵ����
	buf=TempProtBuf+(TempIntegral/IntegralFactor); //��������ۼ�ֵ
	if(buf<0)buf=0; //�����ۼ�ֵ����С��0
  buf=LastCurrent-buf; //�ɵ�λ������ȥ�ۼ�ֵ�õ�ʵ�ʵ���(mA)
	TempProtBuf=ModeCur-LastCurrent; //Pֵ��������µ�λ�ĵ���-�ɵ�λʵ�ʵ���(mA)
	if(TempProtBuf<0)TempProtBuf=0; //�������������С��0
	TempIntegral=0; //���ֻ���=0
	}
	
//�����ǰ�¿ص�����ֵ
int ThermalILIMCalc(void)
	{
	int result;
	//�ж��¿��Ƿ���Ҫ���м���
	if(!IsTempLIMActive)result=Current; //�¿ر��رգ��������ƽ������ٷ���ȥ����
	//��ʼ�¿ؼ���
	else
		{
	  result=TempProtBuf+(TempIntegral/IntegralFactor); //���ݻ��������
		if(result<0)result=0; //������ֵ����
		result=Current-result;
		if(result<MinumumILED)result=MinumumILED; //�������Ʋ�����С����͵���
		}
	//�ж��Ƿ񴥷����������ؽ��	
	IsThermalStepDown=result==Current?0:1; //����������������򽵵�û����
	return result; 
	}
//��ȡ�¿ػ�·�ĺ���ֵ
static char QueryConstantTemp(void)	
	{
	//������ʱ��ʹ�ø��ߵ��¿���������ʱ��
	return CurrentMode->ModeIdx==Mode_Turbo?TurboConstantTemperature:ConstantTemperature;
	}

//�¿�PI����I��(������)�ļ���
void ThermalItgCalc(void)	
	{
	if(!IsDCDCEnabled)return; //DCDC�رգ������вɼ�
	//������(I)
	if(Err>0&&TempIntegral<IntegrateFullScale)TempIntegral++;
  if(Err<0&&TempIntegral>(-IntegrateFullScale))TempIntegral--; //�ۼ����
	}

//�����¶�ʹ�ܿ��Ƶ�ʩ���ش�����
static bit TempSchmittTrigger(bit ValueIN,char HighThreshold,char LowThreshold)	
	{
	if(Data.Systemp>HighThreshold)return 1;
	if(Data.Systemp<LowThreshold)return 0;
	//��ֵ���֣�û�иı�
	return ValueIN;
	}

//�¿ؼ��㺯��
void ThermalCalcProcess(void)
	{
	int ProtFact,ProtRemain;
	bit ThermalStatus;
	//�¶ȴ���������
	if(!Data.IsNTCOK)
		{
		ReportError(Fault_NTCFailed);
		return;
		}
	//�ֵ��¶ȹ���ʱ�Լ�����������
	IsForceLeaveTurbo=TempSchmittTrigger(IsForceLeaveTurbo,ForceOffTemp-10,ForceDisableTurboTemp-10);	//�¶Ⱦ���ػ������ļ�಻��10�ȣ������˳�����
	IsDisableTurbo=TempSchmittTrigger(IsDisableTurbo,ForceDisableTurboTemp,ForceDisableTurboTemp-10); //�¶ȴﵽ�رռ���������ֵ���رռ���
	//���ȹػ�����
	IsSystemShutDown=TempSchmittTrigger(IsSystemShutDown,ForceOffTemp,ConstantTemperature-10);
  if(IsSystemShutDown)ReportError(Fault_OverHeat); //������
	else if(ErrCode==Fault_OverHeat)ClearError(); //��������ǰ����
	//PI��ʹ�ܿ���
	if(!CurrentMode->IsNeedStepDown)IsTempLIMActive=0; //��ǰ��λ����Ҫ����
	else //ʹ��ʩ���غ��������¿��Ƿ񼤻�
		{
		ThermalStatus=TempSchmittTrigger(IsTempLIMActive,QueryConstantTemp(),ReleaseTemperature); //��ȡʩ���ش������Ľ��
		if(ThermalStatus)IsTempLIMActive=1;//ʩ���غ���Ҫ�󼤻��¿أ���������
		else if(!ThermalStatus&&TempProtBuf==0&&TempIntegral<=0)IsTempLIMActive=0; //ʩ���غ���Ҫ��ر��¿أ��ȴ���������Ϊ0���������ر�
		}
	//PI���رգ���λ��ֵ
	if(!IsTempLIMActive)
		{
		TempIntegral=0;
		TempProtBuf=0;
		}
	//����PI����P����(�������������ʱ�����)
	else if(IsDCDCEnabled)
		{
		//�����
		Err=Data.Systemp-QueryConstantTemp();
		//������(P)
		if(Err>2) //�����
			{
			//����P����ֵ
			ProtFact=((CurrentBuf>>12)+1)*(int)Err; //���������(�������˱�ֵPI��Pֵ�����ŵ��������Ӷ�����,P=I/4096+1)
			//����������ۼ�
			if(Current>TurboCurrent)ProtRemain=TurboCurrent;
			else ProtRemain=Current; //���յ�ǰ��λ����ֵȡ��������
			ProtRemain=(ProtRemain-MinumumILED)-TempProtBuf; //�������ϼӵĿ���ʣ������ռ�
			if(ProtFact<ProtRemain)TempProtBuf+=ProtFact; //���ϵ����пռ䣬ֱ�Ӽ�
			else TempProtBuf+=ProtRemain; //���Ͽ��õ�ʣ��ֵ
			}
		else if(Err<0)//�����	
			{
			ProtFact=(int)Err/5; //������СΪ1/3���������ٶ�
			ProtRemain=-TempProtBuf; //���¼��Ŀ��ÿռ�Ϊ���ĵ�ǰֵ	
			if(ProtFact>ProtRemain)TempProtBuf+=ProtFact; //���ϵ����пռ䣬ֱ�Ӽ�
			else TempProtBuf+=ProtRemain; //���Ͽ��õ�ʣ��ֵ
			}
		}
	}	
