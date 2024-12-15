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

//温度控制用全局变量
static xdata int TempIntegral=0;
static xdata int TempProtBuf=0;
static bit IsTempLIMActive=0;  //温控是否已经启动
static unsigned char itgdelay=0xFF;
bit IsThermalStepDown=0; //标记位，是否降档
bit IsDisableTurbo=0;  //禁止再度进入到极亮档
bit IsForceLeaveTurbo=0; //是否强制离开极亮档

//换挡的时候根据当前恒温的电流重新PI值
void RecalcPILoop(int LastCurrent)	
	{
	int buf,ModeCur;
	//目标挡位不需要计算
	if(!CurrentMode->IsNeedStepDown)return;
	//获取当前挡位电流
	ModeCur=CurrentMode->Current;
	if(ModeCur>TurboCurrent)ModeCur=TurboCurrent; //取换挡之后的电流
	//计算P值缓存
	buf=TempProtBuf+(TempIntegral/IntegralFactor); //计算电流扣减值
	if(buf<0)buf=0; //电流扣减值不能小于0
  buf=LastCurrent-buf; //旧挡位电流减去扣减值得到实际电流(mA)
	TempProtBuf=ModeCur-LastCurrent; //P值缓存等于新挡位的电流-旧挡位实际电流(mA)
	if(TempProtBuf<0)TempProtBuf=0; //不允许比例缓存小于0
	TempIntegral=0; //积分缓存=0
	}
	
//输出当前温控的限流值
int ThermalILIMCalc(void)
	{
	int result;
	//判断温控是否需要进行计算
	if(!IsTempLIMActive)result=Current; //温控被关闭，电流限制进来多少返回去多少
	//开始温控计算
	else
		{
	  result=TempProtBuf+(TempIntegral/IntegralFactor); //根据缓存计算结果
		if(result<0)result=0; //不允许负值出现
		result=Current-result;
		if(result<MinumumILED)result=MinumumILED; //电流限制不允许小于最低电流
		}
	//判断是否触发降档并返回结果	
	IsThermalStepDown=result==Current?0:1; //如果输入等于输出，则降档没发生
	return result; 
	}
	
//温控计算函数
void ThermalCalcProcess(void)
	{
	int Err,ProtFact,ProtRemain;
	//温度传感器错误
	if(!Data.IsNTCOK)
		{
		ReportError(Fault_NTCFailed);
		return;
		}
	//当筒头温度过高时，关闭极亮档	
	if(Data.Systemp>(ForceOffTemp-10))IsForceLeaveTurbo=1;  //温度距离关机保护的间距不到10度，立即退出极亮
	if(Data.Systemp>ForceDisableTurboTemp)IsDisableTurbo=1;
	else if(Data.Systemp<(ForceDisableTurboTemp-10))IsDisableTurbo=0;
	if(IsForceLeaveTurbo&&!IsDisableTurbo)IsForceLeaveTurbo=0;	 //如果强制退出极亮标志位置位且温度已经回落到极亮解锁的阈值点，则复位
	//过热故障
	if(Data.Systemp>ForceOffTemp)
		{
		ReportError(Fault_OverHeat);
		return;
		}
	else if(Data.Systemp<(ConstantTemperature-10)&&ErrCode==Fault_OverHeat)
		{
	  ErrCode=Fault_None;
	  SwitchToGear(Mode_OFF); //温度回落，消除故障指示
		}
	//PI环使能控制
	if(!CurrentMode->IsNeedStepDown)IsTempLIMActive=0; //当前挡位不需要降档
	if(Data.Systemp>ConstantTemperature)IsTempLIMActive=1; //温度达到恒温阈值点，启动恒温
	else if(Data.Systemp<ReleaseTemperature&&TempProtBuf==0&&TempIntegral==(-IntegrateFullScale))IsTempLIMActive=0;  //温度低于释放点且积分器和微分器输出达到负饱和，温控关闭
	//PI环关闭，复位数值
	if(!IsTempLIMActive)
		{
		TempIntegral=0;
		TempProtBuf=0;
		}
	//进行PI计算(仅在输出开启的时候进行)
	else if(IsDCDCEnabled)
		{
		//求误差
		Err=(int)(Data.Systemp-ConstantTemperature);
		//比例项(P)
		if(Err>1) //正误差
			{
			//计算P项数值
			ProtFact=((CurrentBuf>>12)+1)*Err; //计算比例项(这里用了变值PI，P值会随着电流的增加而增加,P=I/4096+1)
			//进行正误差累加
			if(Current>TurboCurrent)ProtRemain=TurboCurrent;
			else ProtRemain=Current; //按照当前挡位电流值取积分上限
			ProtRemain=(ProtRemain-MinumumILED)-TempProtBuf; //计算往上加的可用剩余比例空间
			if(ProtFact<ProtRemain)TempProtBuf+=ProtFact; //向上递增有空间，直接加
			else TempProtBuf+=ProtRemain; //加上可用的剩余值
			}
		else if(Err<0)//负误差	
			{
			ProtFact=Err/3; //负误差，缩小为1/3减缓升档速度
			ProtRemain=-TempProtBuf; //往下减的可用空间为负的当前值	
			if(ProtFact>ProtRemain)TempProtBuf+=ProtFact; //向上递增有空间，直接加
			else TempProtBuf+=ProtRemain; //加上可用的剩余值
			}
    //积分项(I)
		itgdelay--;
		if(itgdelay)return;
		itgdelay=0xFF;  //制造额外的延时进行分频
		if(Err>0&&TempIntegral<IntegrateFullScale)TempIntegral++;
    else if(Err<0&&TempIntegral>(-IntegrateFullScale))TempIntegral--; //累加误差
		}
	}	
