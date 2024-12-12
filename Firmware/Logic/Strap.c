#include "ADCCfg.h"
#include "Strap.h"
#include "SelfTest.h"

//ȫ�ֱ���
xdata char VbattCellCount; //ϵͳ�ĵ�ؽ���
xdata int TurboCurrent; //����ģʽ�µĵ���
bit Is6VLED; //�Ƿ�Ϊ6V LED
bit IsPosTailKey; //�Ƿ�Ϊ����β��

//���õ��������
code unsigned char StrapResK[]={82,75,68,56,47,33,24,15}; //����ֵ(K)
/*****************************************************
LED��������Ϊbit field��ÿ1λ��Ӧһ����ֵ�������µ�LED
���͡�����82K����ֵ�¶�Ӧ����6V LED�����Ӧ�ĵ�0λΪ1
75KΪ0��ڶ�λΪ0��68KΪ1�����λΪ1���������Ҳ�Դ���
�ơ�3V���ʱ��Ӧ��λΪ1��6V���ʱ��Ӧ��λΪ0��

����Ĭ���������£�
82K��3��3.7V��Ԫ����룬6V LED������14.5A��������DFEx-SuperLED+ QV7007I Gen1��
75K��3��3.7V��Ԫ����룬6V LED������18A��������DFEx-SuperLED+ FI1048D Gen1��
68K��3��3.7V��Ԫ����룬6V LED������10A��������SFQ75.3-6V or CREE XHP70.2 or CREE XHP70.3HI/HD��
56K��3��3.7V��Ԫ����룬3V LED������22A��������Luminus SBT90.2 or NBT160.2 or SFQ75.3��
47K��2��3.7V��Ԫ����룬3V LED������22A��������Luminus SBT90.2 or NBT160.2 or SFQ75.3��
33K��2��3.7V��Ԫ����룬3V LED������12A��������N5-235HP����������SFT40 N5145��̼��޹�ǿ��
24K��2��3.7V��Ԫ����룬3V LED������18A��������Luminus SBT90.2��׼ʹ�ò������ʣ�
15K��2��3.7V��Ԫ����룬3V LED������10A��������N5-235HP��׼ʹ�ò������ʣ�

*****************************************************/
code unsigned char ITurbo[]={145,180,100,220,220,120,180,100}; //��������(0.1A)
code unsigned char LEDType=0x07;
code unsigned char BattType=0x0F; //�������

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
  //�������������͵����ѹ�Լ���ؽ���
	for(i=0;i<sizeof(StrapResK);i++)
		{
		if(res>(int)StrapResK[i]+4||res<(int)StrapResK[i]-4)continue;
		TurboCurrent=(int)ITurbo[i]*100;
		VbattCellCount=(BattType&(0x01<<i))?3:2; 
		Is6VLED=(LEDType&(0x01<<i))?1:0;
		return; //�ҵ��Ϸ�������
		}
	//�Ϸ���Strap�б����涼�ҹ���û�ҵ����ʵģ�����
  ReportError(Fault_StrapResistorError);
	}
