#include "ADCCfg.h"
#include "LEDMgmt.h"
#include "delay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "cms8s6990.h"
#include "PWMCfg.h"

//PI环参数和最小电流限制
#define ProtFullScale 18000 //PI环输出的细分值
#define IntegrateFullScale 12000 //积分的Full Scale
#define IntegralFactor 150 //积分系数(越大时间常数越高)
#define MinumumILED 900 //降档系统所能达到的最低电流(mA)

//温度配置
#define ForceOffTemp 75 //过热关机温度
#define ForceDisableTurboTemp 60 //超过此温度无法进入极亮
#define ConstantTemperature 52 //温控启动后维持的温度

//温度控制用全局变量
static int TempIntegral=0;
static int TempProtBuf=0;
bit IsTempLIMActive=0;  //温控是否已经启动
bit IsDisableTurbo=0;  //禁止再度进入到极亮档
bit IsForceLeaveTurbo=0; //是否强制离开极亮档

//上电时检测NTC状态
void CheckNTCStatus(void)
	{
	char i=64;
	//检查温度数据
  do
		{
		delay_ms(10);
		SystemTelemHandler();
		if(Data.IsNTCOK)break; //NTC已经正常工作，退出检测
		i--;
		}		
	while(i);
	if(!i) //经过0.64秒的等待仍然不达标，报错
		{
		LEDMode=LED_Amber; 
		LEDControlHandler(); //NTC自检不通过，黄灯常亮
		while(1); //死循环
		}
	}

//比例和积分运算结果百分比限幅
static float PresentLIM(float IN)
	{
	if(IN>100)return 100;
  if(IN<0)return 0;  
	//合法数值原路返回
	return IN;
	}	
	
//输出限流值的百分比
int ThermalILIMCalc(int Input)
	{
	float buf,ILED,itgbuf;
	//温控被禁止或者传入的电流小于等于0，传入多少电流就返回多少	
	if(!IsTempLIMActive||Input<=0)return Input;
	//附加比例项
	buf=(float)TempProtBuf/(float)ProtFullScale; //换成比例项
	buf*=100;
  buf=PresentLIM(buf);
	//附加积分项
	itgbuf=(float)TempIntegral/(float)IntegrateFullScale; //换算积分项
	buf+=itgbuf*10;//将换算完毕的积分项加入到比例项中（最多造成10%的功率波动）
	buf=PresentLIM(buf); //限幅
	//将输入电流和传入的电流值进行计算	
	if(Input<=MinumumILED)return MinumumILED; //输入最大电流参数小于允许的细分值
	ILED=(float)(Input-MinumumILED)/(float)100; //算出细分值
	ILED*=(100-buf); //算出在最低电流值到达目标电流值之间的增量Δ
	return MinumumILED+(int)ILED; //返回实际的电流值
	}
	
//温控计算函数
void ThermalCalcProcess(void)
	{
	int Err;
	//温度传感器错误
	if(!Data.IsNTCOK)
		{
		ErrCode=Fault_NTCFailed; //填写错误代码
    if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //指示故障发生
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
		ErrCode=Fault_OverHeat; //填写错误代码
    if(CurrentMode->ModeIdx!=Mode_Fault)SwitchToGear(Mode_Fault);  //指示故障发生
		return;
		}
	else if(Data.Systemp<(ForceOffTemp-20)&&ErrCode==Fault_OverHeat)
		{
	  ErrCode=Fault_None;
	  SwitchToGear(Mode_OFF); //温度回落，消除故障指示
		}
	//PI环使能控制
	if(!CurrentMode->IsNeedStepDown)IsTempLIMActive=0; //当前挡位不需要降档
	else if(Data.Systemp>ConstantTemperature)IsTempLIMActive=1;
	else if(Data.Systemp<(ConstantTemperature-10))IsTempLIMActive=0; //滞回控制
	//PI环关闭，复位数值
	if(!IsTempLIMActive)
		{
		TempIntegral=0;
		TempProtBuf=0;
		}
	//进行PI计算(仅在输出开启的时候进行)
	else if(Current>0)
		{
		//求误差
		Err=Data.Systemp-ConstantTemperature;
		//比例项(P)
		TempProtBuf+=(Err>1)?Err*(iabsf(Current/6000)+1):0; //动态比例项调整功能
    if(TempProtBuf>ProtFullScale)TempProtBuf=ProtFullScale;
    if(TempProtBuf<0)TempProtBuf=0;  //限制幅度
    //积分项(I)
    TempIntegral+=Err; //累加误差
    if(TempIntegral>IntegrateFullScale)TempIntegral=IntegrateFullScale;
		if(TempIntegral<-IntegrateFullScale)TempIntegral=-IntegrateFullScale;  //积分限幅
		}
	}	
