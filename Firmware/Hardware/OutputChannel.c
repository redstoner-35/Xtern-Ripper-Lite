#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "ModeControl.h"
#include "TempControl.h"
#include "OutputChannel.h"
#include "ADCCfg.h"
#include "TailKey.h"
#include "Strap.h"
#include "SelfTest.h"

//内部SFR
sbit RevPGate=RevProtIOP^RevProtIOx; //反接保护MOSFET
sbit DCDCEN=DCDCENIOP^DCDCENIOx; //DCDC使能功能
sbit LShuntSEL=LShuntSelIOP^LShuntSelIOx; //低亮月光档专用分流器选择位
sbit HShuntSEL=HShuntSelIOP^HShuntSelIOx; //主输出分流器选择位

//内部变量
bit IsSlowRamp; //开启慢速Ramp
volatile bit IsDCDCEnabled; //DCDC是否使能
xdata int CurrentBuf; //存储当前已经上传的电流值 
xdata volatile int Current; //目标电流(mA)

//初始化函数
void OutputChannel_Init(void)
	{
	GPIOCfgDef OCInitCfg;
	//设置结构体
	OCInitCfg.Mode=GPIO_Out_PP;
  OCInitCfg.Slew=GPIO_Fast_Slew;		
	OCInitCfg.DRVCurrent=GPIO_High_Current; //推MOSFET,需要高上升斜率
	//初始化bit
	RevPGate=0;
	DCDCEN=0;
	LShuntSEL=0;
	HShuntSEL=0;
	//开始配置IO	
	GPIO_ConfigGPIOMode(RevProtIOG,GPIOMask(RevProtIOx),&OCInitCfg);	
	GPIO_ConfigGPIOMode(DCDCENIOG,GPIOMask(DCDCENIOx),&OCInitCfg);		
	GPIO_ConfigGPIOMode(LShuntSelIOG,GPIOMask(LShuntSelIOx),&OCInitCfg);		
	GPIO_ConfigGPIOMode(HShuntSelIOG,GPIOMask(HShuntSelIOx),&OCInitCfg);			
	//系统上电时电流配置为0
	Current=0;
	CurrentBuf=0;
	IsDCDCEnabled=0;
	IsSlowRamp=0;
	}

//输出通道测试运行
void OutputChannel_TestRun(void)
	{
	int retry=64,i;
	xdata float LastOutput[5]={0};
  xdata float buf,Err;
	//准备启动输出
	if(Data.RawBattVolt<5.5||CurrentMode->ModeIdx!=Mode_OFF)return; //输入电压过低避免误报，或者上次关机前没有熄灯，为了尽快点亮跳过检测
	LShuntSEL=0;
	HShuntSEL=0;
	RevPGate=0; //关闭防反接检测PIN
	PWM_ForceSetDuty(1); //打开PWMDAC输出一个初值		
	DCDCEN=1; //令DCDCEN=1
	//等待DCDC启动	
	do
		{
		delay_ms(5);
		SystemTelemHandler();
	  if(Data.OutputVoltage>4.2)break; //输出电压正常
		retry--;
		}
	while(retry>0);
	//DCDC启动失败
	if(retry==0)
		{
		DCDCEN=0; //令DCDCEN=0
	  ReportError(Fault_DCDCFailedToStart); //报告错误
	  return;
		}
	//进行输出EN控制的检测
	retry=100; //复位延时等待
	DCDCEN=0; //令DCDCEN=0
	delay_ms(20); //延迟20ms
	PWM_ForceSetDuty(0); //关闭PWMDAC
	do
		{
		SystemTelemHandler();
		delay_ms(10);
		//更新数据
		for(i=4;i>0;i--)LastOutput[i]=LastOutput[i-1];	
		LastOutput[0]=Data.OutputVoltage;
		//求平均	
		buf=0;
    for(i=0;i<5;i++)buf+=LastOutput[i];
    buf/=(float)5;			
		//求数据里面每组数据的差距
		Err=0;	
		for(i=0;i<5;i++)Err+=fabs(buf-LastOutput[i]);
	  if(retry<93&&Err>0.5)break; //输出电压正常衰减中，掉电
		retry--;
		}
	while(retry>0);
	//DCDC停止失败，EN不受控，报错
	if(retry==0)ReportError(Fault_DCDCENOOC);
	}	

//内部用于计算PWMDAC占空比的函数	
static float Duty_Calc(float ShuntmOhm,int CurrentInput)
	{
	float buf;
	char Offset;
	//计算补偿值
	Offset=CurrentMode->Offset;
	if(CurrentMode->ModeIdx==Mode_Ramp)Offset+=(char)(CurrentInput/205); //无极调光模式下自动增加offset
	//计算实际占空比
	buf=(float)CurrentInput*ShuntmOhm; //输入传进来的电流(mA)并乘以检流电阻阻值(mR)得到运放端整定电压(uV)
	buf/=(float)1000; //uV转mV
	buf/=((float)VdivDownResK/(float)(VdivUpResK+VdivDownResK+PWMDACResK)); //将运放端整定电压除以电阻的分压比例得到DAC端的电压
	buf*=(float)Offset/(float)100; //乘以矫正系数修正电流
	buf/=Data.MCUVDD*(float)1000; //计算出目标DAC输出电压和PWMDAC缓冲器供电电压(MCUVDD)之间的比值
	buf*=100; //转换为百分比
	//结果输出	
	return buf>100?100:buf;
	}
	
//输出通道计算
void OutputChannel_Calc(void)
	{
	int TargetCurrent;
	bit IsAux;
	//根据当前传入电流和其余状态得出实际要怼入温控计算函数的电流
	if(TailKeyTIM<(TailKeyRelTime+1))TargetCurrent=0; //当前进入掉电模式，立即关闭输出
	else if(Current>TurboCurrent)TargetCurrent=TurboCurrent; //如果目标电流超过了限制值，则等于目标设置
	else TargetCurrent=Current; //按照挡位状态机运算出来的结果填写  
	//温控计算
  if(TargetCurrent>ThermalILIMCalc())TargetCurrent=ThermalILIMCalc(); //温控反馈的电流限制超过允许值
	//避免无效的重复计算
	if(CurrentBuf==TargetCurrent)return;
	//保护LED的电流斜率限制器
	if(TargetCurrent-CurrentBuf>6000)IsSlowRamp=1; //监测到非常大的电流瞬态，避免冲爆灯珠采用软起
  if(IsSlowRamp&&TargetCurrent>0)
		{
		if(CurrentBuf==0)CurrentBuf=1500; //电流为0从1500开始输出
		else switch(CurrentMode->ModeIdx)
			{
			case Mode_Strobe:CurrentBuf+=3000;break;
			case Mode_SOS:CurrentBuf+=500;break;
			default:CurrentBuf+=20;
			}
		if(CurrentBuf>=TargetCurrent)
			{
			IsSlowRamp=0;
			CurrentBuf=TargetCurrent; //限幅，不允许目标电流大于允许值
			}
		}
	else CurrentBuf=TargetCurrent; //直接同步
	//电流小于等于0，关闭所有输出
	if(CurrentBuf<=0)
		{
		DCDCEN=0;
	  PWMDuty=0;			//DCDC被关闭，禁用输出
		IsDCDCEnabled=0;  //标记DCDC已被关闭
		RevPGate=CurrentBuf==-1?1:0; //复位防反接MOS
		LShuntSEL=0;
		HShuntSEL=0;  //复位检流选择MOS
		}
	//输出开启，选择对应通道
	else
		{
		//判断是否使用辅助通道
		IsAux=CurrentBuf<AUXChannelImax?1:0;
		//EN处于关闭状态，启用DCDC后令PWMDAC=0等待一段时间解决输出电流过冲导致闪烁的问题
		if(!IsDCDCEnabled)
				{
				RevPGate=~IsAux;   //输入功率不大时下关闭防反接FET节省能量
				DCDCEN=1;  
				LShuntSEL=IsAux;  
				HShuntSEL=~IsAux;  //启动DCDC，选择对应通道，延迟5mS后再送给定
				delay_ms(5); 
				IsDCDCEnabled=1; //标记DCDC已经开始运行
				}
		//设置输出占空比
		PWMDuty=Duty_Calc(IsAux?AUXChannelShuntmOhm:MainChannelShuntmOhm,CurrentBuf);
		}
	//更新完毕上传PWM数值
	IsNeedToUploadPWM=1;
	}
