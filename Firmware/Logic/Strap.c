#include "ADCCfg.h"
#include "Strap.h"
#include "ModeControl.h"

//ȫ�ֱ���
xdata char VbattCellCount; //ϵͳ�ĵ�ؽ���
xdata int TurboCurrent; //����ģʽ�µĵ���
bit Is6VLED; //�Ƿ�Ϊ6V LED
bit IsPosTailKey; //�Ƿ�Ϊ����β��

code unsigned char ITurbo[]={100,145,180,200}; //��������(0.1A)
code unsigned char StrapResK[]={70,50,30,10}; //����ֵ

//��ȡStrap
void Strap_Init(void)
	{
	int res,i;	
	//��ȡStrap��ѹ�������Ƿ�������β��
	SystemTelemHandler();
	res=Data.CfgStrapRes;
	//����β������
	if(res>100)
		 { 
	   res-=100; //����ֵ����100Kʱ��100K
	   IsPosTailKey=0; //���Ϊ���򿪹�
		 }
	else IsPosTailKey=1; //���򿪹�
	//���������ѹ
	Is6VLED=res<30?0:1; //����С��50K�ĵ��趼��3V LED
  //�������������͵�ؽ���
	for(i=0;i<sizeof(StrapResK);i++)
		{
		if(res>(int)StrapResK[i]+30||res<(int)StrapResK[i])continue;
		TurboCurrent=(int)ITurbo[i]*100;
		VbattCellCount=res>(int)StrapResK[i]+10?3:2; 
		return; //�ҵ��Ϸ�������
		}
	//�Ϸ���Strap�б����涼�ҹ���û�ҵ����ʵģ�����
  ReportError(Fault_StrapResistorError);
	}
