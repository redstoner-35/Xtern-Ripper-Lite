#include "ADCCfg.h"
#include "Strap.h"
#include "SelfTest.h"

//全局变量
xdata char VbattCellCount; //系统的电池节数
xdata int TurboCurrent; //极亮模式下的电流
bit Is6VLED; //是否为6V LED
bit IsPosTailKey; //是否为正向尾按

//配置电阻参数区
code unsigned char StrapResK[]={82,75,68,56,47,33,24,15}; //电阻值(K)
/*****************************************************
LED类型配置为bit field，每1位对应一种阻值的条件下的LED
类型。例如82K的阻值下对应的是6V LED，则对应的第0位为1
75K为0则第二位为0，68K为1则第三位为1。电池类型也以此类
推。3V电池时对应的位为1，6V电池时对应的位为0。

程序默认配置如下：
82K：3串3.7V三元锂输入，6V LED，极亮14.5A（适用于DFEx-SuperLED+ QV7007I Gen1）
75K：3串3.7V三元锂输入，6V LED，极亮18A（适用于DFEx-SuperLED+ FI1048D Gen1）
68K：3串3.7V三元锂输入，6V LED，极亮10A（适用于SFQ75.3-6V or CREE XHP70.2 or CREE XHP70.3HI/HD）
56K：3串3.7V三元锂输入，3V LED，极亮22A（适用于Luminus SBT90.2 or NBT160.2 or SFQ75.3）
47K：2串3.7V三元锂输入，3V LED，极亮22A（适用于Luminus SBT90.2 or NBT160.2 or SFQ75.3）
33K：2串3.7V三元锂输入，3V LED，极亮12A（适用于N5-235HP或者老批次SFT40 N5145冲刺极限光强）
24K：2串3.7V三元锂输入，3V LED，极亮18A（适用于Luminus SBT90.2标准使用不超功率）
15K：2串3.7V三元锂输入，3V LED，极亮10A（适用于N5-235HP标准使用不超功率）

*****************************************************/
code unsigned char ITurbo[]={145,180,100,220,220,120,180,100}; //极亮电流(0.1A)
code unsigned char LEDType=0x07;
code unsigned char BattType=0x0F; //电池类型

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
  //决定极亮电流和灯珠电压以及电池节数
	for(i=0;i<sizeof(StrapResK);i++)
		{
		if(res>(int)StrapResK[i]+4||res<(int)StrapResK[i]-4)continue;
		TurboCurrent=(int)ITurbo[i]*100;
		VbattCellCount=(BattType&(0x01<<i))?3:2; 
		Is6VLED=(LEDType&(0x01<<i))?1:0;
		return; //找到合法的配置
		}
	//合法的Strap列表里面都找过了没找到合适的，报错
  ReportError(Fault_StrapResistorError);
	}
