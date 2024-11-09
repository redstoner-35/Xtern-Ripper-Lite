#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"

//全局变量
xdata float PWMDuty;
static bit IsPWMLoading; //PWM正在加载中
static bit IsNeedToEnableOutput; //是否需要启用输出
bit IsNeedToUploadPWM; //是否需要更新PWM

//关闭PWM定时器
void PWM_DeInit(void)
	{
	//配置为普通GPIO
  GPIO_SetMUXMode(PWMDACIOG,PWMDACIOx,GPIO_AF_GPIO);	
	//设置为输出0
	GPIO_WriteBit(PWMDACIOG,PWMDACIOx,0);	
	//关闭PWM模块
	PWMOE=0x00;
	PWMCNTE=0x00;
	}

//PWM定时器初始化
void PWM_Init(void)
	{
	GPIOCfgDef PWMInitCfg;
	//设置结构体
	PWMInitCfg.Mode=GPIO_Out_PP;
  PWMInitCfg.Slew=GPIO_Slow_Slew;		
	PWMInitCfg.DRVCurrent=GPIO_High_Current; //推PWMDAC，不需要很高的上升斜率
	//配置GPIO
	GPIO_WriteBit(PWMDACIOG,PWMDACIOx,0);
	GPIO_ConfigGPIOMode(PWMDACIOG,GPIOMask(PWMDACIOx),&PWMInitCfg); 
	//启用复用功能
  GPIO_SetMUXMode(PWMDACIOG,PWMDACIOx,GPIO_AF_PWMCH0);
	//配置PWM发生器
	PWMCON=0x00; //PWM通道为六通道独立模式，向下计数，关闭非对称计数功能	
	PWMOE=0x01; //打开PWM输出通道0
	PWM01PSC=0x01;  //打开预分频器和计数器时钟 
  PWM0DIV=0xff;   //令Fpwmcnt=Fsys=48MHz(不分频)
  PWMPINV=0x00; //所有通道均设置为正常输出模式
	PWMCNTM=0x01; //通道0配置为自动加载模式
	PWMCNTCLR=0x01; //初始化PWM的时候复位定时器
	PWMDTE=0x00; //关闭死区时间
	PWMMASKD=0x00; 
	PWMMASKE=0x01; //PWM掩码功能启用，默认状态下禁止通道0输出
	//配置周期数据
	PWMP0H=(PWMStepConstant>>8)&0xFF;
	PWMP0L=PWMStepConstant&0xFF;	
	//配置占空比数据
  PWMD0H=0;
	PWMD0L=0;	
	//初始化变量
	PWMDuty=0;
	IsPWMLoading=0; 
	IsNeedToUploadPWM=0;
	//启用PWM
	PWMCNTE=0x01; //使能通道0的计数器，PWM开始运行
	PWMLOADEN=0x01; //加载通道0的PWM值
	while(PWMLOADEN!=0); //等待加载结束
	}

//PWM强制设置占空比
void PWM_ForceSetDuty(bit IsEnable)
	{
	PWMD0H=0x01;
	PWMD0L=IsEnable?0xFF:0;			
	PWMLOADEN=0x01; //开始加载
	while(PWMLOADEN!=0); //等待加载结束
	PWMMASKE=IsEnable?0x00:0x01;  //设置寄存器打开输出
	}	
	
//根据PWM结构体内的配置进行输出
void PWM_OutputCtrlHandler(void)	
	{
	int value;
	float buf;
	//判断是否需要加载的逻辑运算
	if(!IsNeedToUploadPWM)return; //不需要加载
	else if(IsPWMLoading) //当次加载运行中
		{
	  if(!PWMLOADEN)//加载寄存器复位为0，表示加载成功
			 {
			 PWMMASKE=IsNeedToEnableOutput?0x00:0x01; //更新PWMMASKE寄存器根据输出状态启用对应的通道
			 IsNeedToUploadPWM=0;
		   IsPWMLoading=0;  //正在加载状态为清除
			 }
	  return;
		}
	//PWM占空比限制
	if(PWMDuty>100)PWMDuty=100;
	if(PWMDuty<0)PWMDuty=0;
	//配置装载数值
	IsNeedToEnableOutput=buf?1:0; //是否需要启用输出
	buf=PWMDuty*(float)PWMStepConstant;
	buf/=(float)100;
	value=(int)buf;
	PWMD0H=(value>>8)&0xFF;
	PWMD0L=value&0xFF;			
	//PWM寄存器数值已装入，应用数值		
	IsPWMLoading=1; //标记加载过程进行中
	PWMLOADEN=0x01; //开始加载
	}
