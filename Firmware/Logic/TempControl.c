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
static char Err=0; //全局变量，PI环的误差值

//状态位
bit IsThermalStepDown=0; //标记位，是否降档
bit IsDisableTurbo=0;  //禁止再度进入到极亮档
bit IsForceLeaveTurbo=0; //是否强制离开极亮档
bit IsSystemShutDown=0; //是否触发温控强制关机

//换挡的时候根据当前恒温的电流重新PI值
void RecalcPILoop(int LastCurrent)	
	{
	int buf,ModeCur;
	//目标挡位不需要计算
	if(!CurrentMode->IsNeedStepDown)return;
	//获取当前挡位电流
	ModeCur=QueryCurrentGearILED();
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
//获取温控环路的恒温值
static char QueryConstantTemp(void)	
	{
	//极亮的时候使用更高的温控拉长降档时间
	return CurrentMode->ModeIdx==Mode_Turbo?TurboConstantTemperature:ConstantTemperature;
	}

//温控PI环中I项(积分器)的计算
void ThermalItgCalc(void)	
	{
	if(!IsDCDCEnabled)return; //DCDC关闭，不进行采集
	//积分项(I)
	if(Err>0&&TempIntegral<IntegrateFullScale)TempIntegral++;
  if(Err<0&&TempIntegral>(-IntegrateFullScale))TempIntegral--; //累加误差
	}

//负责温度使能控制的施密特触发器
static bit TempSchmittTrigger(bit ValueIN,char HighThreshold,char LowThreshold)	
	{
	if(Data.Systemp>HighThreshold)return 1;
	if(Data.Systemp<LowThreshold)return 0;
	//数值保持，没有改变
	return ValueIN;
	}

//温控计算函数
void ThermalCalcProcess(void)
	{
	int ProtFact,ProtRemain;
	bit ThermalStatus;
	//温度传感器错误
	if(!Data.IsNTCOK)
		{
		ReportError(Fault_NTCFailed);
		return;
		}
	//手电温度过高时对极亮进行限制
	IsForceLeaveTurbo=TempSchmittTrigger(IsForceLeaveTurbo,ForceOffTemp-10,ForceDisableTurboTemp-10);	//温度距离关机保护的间距不到10度，立即退出极亮
	IsDisableTurbo=TempSchmittTrigger(IsDisableTurbo,ForceDisableTurboTemp,ForceDisableTurboTemp-10); //温度达到关闭极亮档的阈值，关闭极亮
	//过热关机保护
	IsSystemShutDown=TempSchmittTrigger(IsSystemShutDown,ForceOffTemp,ConstantTemperature-10);
  if(IsSystemShutDown)ReportError(Fault_OverHeat); //报故障
	else if(ErrCode==Fault_OverHeat)ClearError(); //消除掉当前错误
	//PI环使能控制
	if(!CurrentMode->IsNeedStepDown)IsTempLIMActive=0; //当前挡位不需要降档
	else //使用施密特函数决定温控是否激活
		{
		ThermalStatus=TempSchmittTrigger(IsTempLIMActive,QueryConstantTemp(),ReleaseTemperature); //获取施密特触发器的结果
		if(ThermalStatus)IsTempLIMActive=1;//施密特函数要求激活温控，立即激活
		else if(!ThermalStatus&&TempProtBuf==0&&TempIntegral<=0)IsTempLIMActive=0; //施密特函数要求关闭温控，等待比例缓存为0解除限流后关闭
		}
	//PI环关闭，复位数值
	if(!IsTempLIMActive)
		{
		TempIntegral=0;
		TempProtBuf=0;
		}
	//进行PI环的P计算(仅在输出开启的时候进行)
	else if(IsDCDCEnabled)
		{
		//求误差
		Err=Data.Systemp-QueryConstantTemp();
		//比例项(P)
		if(Err>2) //正误差
			{
			//计算P项数值
			ProtFact=((CurrentBuf>>12)+1)*(int)Err; //计算比例项(这里用了变值PI，P值会随着电流的增加而增加,P=I/4096+1)
			//进行正误差累加
			if(Current>TurboCurrent)ProtRemain=TurboCurrent;
			else ProtRemain=Current; //按照当前挡位电流值取积分上限
			ProtRemain=(ProtRemain-MinumumILED)-TempProtBuf; //计算往上加的可用剩余比例空间
			if(ProtFact<ProtRemain)TempProtBuf+=ProtFact; //向上递增有空间，直接加
			else TempProtBuf+=ProtRemain; //加上可用的剩余值
			}
		else if(Err<0)//负误差	
			{
			ProtFact=(int)Err/5; //负误差，缩小为1/3减缓升档速度
			ProtRemain=-TempProtBuf; //往下减的可用空间为负的当前值	
			if(ProtFact>ProtRemain)TempProtBuf+=ProtFact; //向上递增有空间，直接加
			else TempProtBuf+=ProtRemain; //加上可用的剩余值
			}
		}
	}	
