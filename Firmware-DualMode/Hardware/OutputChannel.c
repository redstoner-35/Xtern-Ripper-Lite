#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"
#include "delay.h"
#include "ModeControl.h"
#include "OutputChannel.h"
#include "ADCCfg.h"
#include "Watchdog.h"
#include "TailKey.h"

//内部SFR
sbit RevPGate=RevProtIOP^RevProtIOx; //反接保护MOSFET
sbit DCDCEN=DCDCENIOP^DCDCENIOx; //DCDC使能功能
sbit LShuntSEL=LShuntSelIOP^LShuntSelIOx; //低亮月光档专用分流器选择位
sbit HShuntSEL=HShuntSelIOP^HShuntSelIOx; //主输出分流器选择位

//内部变量
xdata int Current; //目标电流(mA)
static xdata int CurrentBuf;
static xdata char IsEnableDCDCCounter=0; //延时启用DCDC的计时器

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
	}

//输出通道测试运行
void OutputChannel_TestRun(void)
	{
	int retry=64,i;
	xdata float LastOutput[5]={0};
  xdata float buf,Err;
	//检查是否由看门狗导致复位	
	if(GetIfWDogCauseRST())	
		{
		ReportError(Fault_MPUHang); //指示故障由单片机死机导致
		return;
		}
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
	
//输出通道进入休眠的操作
void OutputChannel_DeInit(void)
	{
	//复位电流缓冲器
	Current=0;
	CurrentBuf=0;
	//关闭所有输出
	RevPGate=0;
	DCDCEN=0;
	LShuntSEL=0;
	HShuntSEL=0;	
	}
	
//内部用于计算PWMDAC占空比的函数	
static float Duty_Calc(float ShuntmOhm,int Current,float Offset)
	{
	float buf;
	buf=(float)Current*ShuntmOhm; //输入传进来的电流(mA)并乘以检流电阻阻值(mR)得到运放端整定电压(uV)
	buf/=(float)1000; //uV转mV
	buf/=((float)VdivDownResK/(float)(VdivUpResK+VdivDownResK+PWMDACResK)); //将运放端整定电压除以电阻的分压比例得到DAC端的电压
	buf*=Offset; //乘以矫正系数修正电流
	buf/=Data.MCUVDD*(float)1000; //计算出目标DAC输出电压和PWMDAC缓冲器供电电压(MCUVDD)之间的比值
	buf*=100; //转换为百分比
	//进行限幅和结果输出	
	if(buf>100)buf=100;
	if(buf<0)buf=0;
	return buf;
	}
	
//输出通道计算
void OutputChannel_Calc(void)
	{
	//延时启用DCDC	
	if(IsEnableDCDCCounter&&!IsNeedToUploadPWM)	
		{
		IsEnableDCDCCounter--;
		if(!IsEnableDCDCCounter)DCDCEN=1; //时间到，打开DCDC
		}
	//避免无效的重复计算
	if(TailKeyTIM<(TailKeyRelTime+1))Current=0; //当前进入掉电模式，立即关闭输出
	if(CurrentBuf==Current)return;
	CurrentBuf=Current;
	//电流小于等于0，关闭所有输出
	if(CurrentBuf<=0)
		{
		if(CurrentMode->ModeIdx!=Mode_Strobe) //非爆闪模式下清零PWMDAC基准输出
			{
	    PWMDuty=0;
		  IsNeedToUploadPWM=1;
			}
		RevPGate=CurrentBuf==-1?1:0;
		DCDCEN=0;
		LShuntSEL=0;
		HShuntSEL=0;
		}
	//使用辅助通道
	else if(CurrentBuf<AUXChannelImax)
		{
		PWMDuty=Duty_Calc(AUXChannelShuntmOhm,CurrentBuf,LowShuntIOffset);
		RevPGate=0;   //输入功率不大时下关闭防反接FET节省能量
		if(!DCDCEN)IsEnableDCDCCounter=PWMDACSettleDelay; //如果当前DCDC是关闭状态则延时一段时间再打开
		LShuntSEL=1;  
		HShuntSEL=0;  //启动DCDC，选择低量程通道
		IsNeedToUploadPWM=1; //需要更新PWM输出
		}
	//电流大于辅助通道上限，使用主通道
	else
		{
		PWMDuty=Duty_Calc(MainChannelShuntmOhm,CurrentBuf,HighShuntIOffset);
		RevPGate=1;   //主输出启用，打开防反接FET提高能效
		DCDCEN=1; 
		LShuntSEL=0;  
		HShuntSEL=1;  //启动DCDC，选择高量程通道
		IsNeedToUploadPWM=1; //需要更新PWM输出
		}
	}
