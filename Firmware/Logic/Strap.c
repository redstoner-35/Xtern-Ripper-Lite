#include "ADCCfg.h"
#include "Strap.h"
#include "ModeControl.h"

//全局变量
xdata char VbattCellCount; //系统的电池节数
xdata int TurboCurrent; //极亮模式下的电流
bit Is6VLED; //是否为6V LED
bit IsPosTailKey; //是否为正向尾按

code unsigned char ITurbo[]={100,145,180,200}; //极亮电流(0.1A)
code unsigned char StrapResK[]={70,50,30,10}; //电阻值

//读取Strap
void Strap_Init(void)
	{
	int res,i;	
	//读取Strap电压并决定是否开启正向尾按
	SystemTelemHandler();
	res=Data.CfgStrapRes;
	//决定尾按类型
	if(res>100)
		 { 
	   res-=100; //电阻值大于100K时减100K
	   IsPosTailKey=0; //标记为反向开关
		 }
	else IsPosTailKey=1; //正向开关
	//决定灯珠电压
	Is6VLED=res<30?0:1; //所有小于50K的电阻都是3V LED
  //决定极亮电流和电池节数
	for(i=0;i<sizeof(StrapResK);i++)
		{
		if(res>(int)StrapResK[i]+30||res<(int)StrapResK[i])continue;
		TurboCurrent=(int)ITurbo[i]*100;
		VbattCellCount=res>(int)StrapResK[i]+10?3:2; 
		return; //找到合法的配置
		}
	//合法的Strap列表里面都找过了没找到合适的，报错
  ReportError(Fault_StrapResistorError);
	}
