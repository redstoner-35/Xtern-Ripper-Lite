#ifndef _BattDs_
#define _BattDs_

//状态变量enum
typedef enum
	{
	Battery_Plenty, //电池电量充足
	Battery_Mid, //电池电量较为充足
	Battery_Low, //电池电量不足
	Battery_VeryLow //电池电量严重不足
	}BattStatusDef;

typedef enum
	{
  BattVdis_Waiting, //等待显示阶段
	BattVdis_PrepareDis, //准备显示
	BattVdis_DelayBeforeDisplay, //延迟一段时间
	BattVdis_Show10V, //显示十位
	BattVdis_Gap10to1V, //十位和个位之间的等待
	BattVdis_Show1V, //显示个位
	BattVdis_Gap1to0_1V, //个位和十分位之间的等待
	BattVdis_Show0_1V, //显示小数点后一位(0.1V)
	BattVdis_WaitShowChargeLvl, //等待一段时间后显示当前电量
	BattVdis_ShowChargeLvl, //显示电池电量的等待
	}BattVshowFSMDef; //电池电量显示处理


//外部参考
extern bit IsBatteryAlert; //电池低电警告发生
extern bit IsBatteryFault; //电池低电量故障发生
extern xdata float Battery; //滤波之后的电池电压
extern xdata BattVshowFSMDef VshowFSMState; //状态机状态	

//电池识别配置
#define EnableStrapConfig //开启基于P2.2的配置电阻配置节数的功能
#define ManualCellCount 2 //当禁用基于Strap的识别时手动指定电池节数（根据Strap识别时该宏参数无效） 

	
//函数
void BattDisplayTIM(void); //电池电量显示函数处理
void TriggerVshowDisplay(void); //启动电池电压显示
void DisplayVBattAtStart(void);
void BatteryTelemHandler(void);  //电池测量和指示灯控制

#endif
