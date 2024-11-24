#include "ADCCfg.h"
#include "Strap.h"
#include "ModeControl.h"

//全局变量
xdata char VbattCellCount; //系统的电池节数
xdata int TurboCurrent; //极亮模式下的电流
bit Is6VLED; //是否为6V LED

//读取Strap
void Strap_Init(void)
	{
	int res=Data.CfgStrapRes;
	//判断电阻值是否合法(应在13-120K)
  if(res>120||res<12)
		{
		ReportError(Fault_StrapResistorError); //报告错误
		return;
		}
	//决定灯珠电压
	Is6VLED=res<30?0:1; //所有小于50K的电阻都是3V LED
  //决定极亮电流和电池节数
	if(res>70)
		{
		//75K 100K 7007I 2-3S
		VbattCellCount=res>80?3:2;
		TurboCurrent=15000;
		}
	else if(res>50) 
		{
		//68K 56K 1048D 2-3S
		VbattCellCount=res>60?3:2;
		TurboCurrent=18000;
		}		
	else if(res>30)//47K和33K XHP70.3HI 极亮10000mA
	  {
		VbattCellCount=res>40?3:2;
		TurboCurrent=10000;
		}
	else //24K和15K XHP70.3HI 极亮10000mA SBT90.2 3V
		{
		VbattCellCount=res>20?3:2;
		TurboCurrent=20000;
		}
	}
