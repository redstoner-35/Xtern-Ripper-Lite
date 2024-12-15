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
static unsigned char itgdelay=0xFF;
bit IsThermalStepDown=0; //���λ���Ƿ񽵵�
bit IsDisableTurbo=0;  //��ֹ�ٶȽ��뵽������
bit IsForceLeaveTurbo=0; //�Ƿ�ǿ���뿪������

//������ʱ����ݵ�ǰ���µĵ�������PIֵ
void RecalcPILoop(int LastCurrent)	
	{
	int buf,ModeCur;
	//Ŀ�굲λ����Ҫ����
	if(!CurrentMode->IsNeedStepDown)return;
	//��ȡ��ǰ��λ����
	ModeCur=CurrentMode->Current;
	if(ModeCur>TurboCurrent)ModeCur=TurboCurrent; //ȡ����֮��ĵ���
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
	
//�¿ؼ��㺯��
void ThermalCalcProcess(void)
	{
	int Err,ProtFact,ProtRemain;
	//�¶ȴ���������
	if(!Data.IsNTCOK)
		{
		ReportError(Fault_NTCFailed);
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
		ReportError(Fault_OverHeat);
		return;
		}
	else if(Data.Systemp<(ConstantTemperature-10)&&ErrCode==Fault_OverHeat)
		{
	  ErrCode=Fault_None;
	  SwitchToGear(Mode_OFF); //�¶Ȼ��䣬��������ָʾ
		}
	//PI��ʹ�ܿ���
	if(!CurrentMode->IsNeedStepDown)IsTempLIMActive=0; //��ǰ��λ����Ҫ����
	if(Data.Systemp>ConstantTemperature)IsTempLIMActive=1; //�¶ȴﵽ������ֵ�㣬��������
	else if(Data.Systemp<ReleaseTemperature&&TempProtBuf==0&&TempIntegral==(-IntegrateFullScale))IsTempLIMActive=0;  //�¶ȵ����ͷŵ��һ�������΢��������ﵽ�����ͣ��¿عر�
	//PI���رգ���λ��ֵ
	if(!IsTempLIMActive)
		{
		TempIntegral=0;
		TempProtBuf=0;
		}
	//����PI����(�������������ʱ�����)
	else if(IsDCDCEnabled)
		{
		//�����
		Err=(int)(Data.Systemp-ConstantTemperature);
		//������(P)
		if(Err>1) //�����
			{
			//����P����ֵ
			ProtFact=((CurrentBuf>>12)+1)*Err; //���������(�������˱�ֵPI��Pֵ�����ŵ��������Ӷ�����,P=I/4096+1)
			//����������ۼ�
			if(Current>TurboCurrent)ProtRemain=TurboCurrent;
			else ProtRemain=Current; //���յ�ǰ��λ����ֵȡ��������
			ProtRemain=(ProtRemain-MinumumILED)-TempProtBuf; //�������ϼӵĿ���ʣ������ռ�
			if(ProtFact<ProtRemain)TempProtBuf+=ProtFact; //���ϵ����пռ䣬ֱ�Ӽ�
			else TempProtBuf+=ProtRemain; //���Ͽ��õ�ʣ��ֵ
			}
		else if(Err<0)//�����	
			{
			ProtFact=Err/3; //������СΪ1/3���������ٶ�
			ProtRemain=-TempProtBuf; //���¼��Ŀ��ÿռ�Ϊ���ĵ�ǰֵ	
			if(ProtFact>ProtRemain)TempProtBuf+=ProtFact; //���ϵ����пռ䣬ֱ�Ӽ�
			else TempProtBuf+=ProtRemain; //���Ͽ��õ�ʣ��ֵ
			}
    //������(I)
		itgdelay--;
		if(itgdelay)return;
		itgdelay=0xFF;  //����������ʱ���з�Ƶ
		if(Err>0&&TempIntegral<IntegrateFullScale)TempIntegral++;
    else if(Err<0&&TempIntegral>(-IntegrateFullScale))TempIntegral--; //�ۼ����
		}
	}	
