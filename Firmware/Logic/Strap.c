#include "ADCCfg.h"
#include "Strap.h"
#include "ModeControl.h"

//ȫ�ֱ���
xdata char VbattCellCount; //ϵͳ�ĵ�ؽ���
xdata int TurboCurrent; //����ģʽ�µĵ���
bit Is6VLED; //�Ƿ�Ϊ6V LED

//��ȡStrap
void Strap_Init(void)
	{
	int res=Data.CfgStrapRes;
	//�жϵ���ֵ�Ƿ�Ϸ�(Ӧ��13-120K)
  if(res>120||res<12)
		{
		ReportError(Fault_StrapResistorError); //�������
		return;
		}
	//���������ѹ
	Is6VLED=res<30?0:1; //����С��50K�ĵ��趼��3V LED
  //�������������͵�ؽ���
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
	else if(res>30)//47K��33K XHP70.3HI ����10000mA
	  {
		VbattCellCount=res>40?3:2;
		TurboCurrent=10000;
		}
	else //24K��15K XHP70.3HI ����10000mA SBT90.2 3V
		{
		VbattCellCount=res>20?3:2;
		TurboCurrent=20000;
		}
	}
