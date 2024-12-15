#ifndef _BattDs_
#define _BattDs_

//��ؼ������
#define VBattAvgCount 40 //��Ч���ڵ�ص�ѹ���ݵ�ƽ������(�����ڲ��߼��ĵ�ѹ����,������ʾ�͵�����������)
#define LowVoltStrobeGap 15 //�����͵�ѹ��ʾ֮��ÿ�������һ��

//��ص�ѹƽ������ṹ������
typedef struct
	{
	int Min;
  int Max;
	long AvgBuf;
	unsigned char Count;
	}AverageCalcDef;	

//״̬����enum
typedef enum
	{
	Battery_Plenty=0, //��ص�������
	Battery_Mid=1, //��ص�����Ϊ����
	Battery_Low=2, //��ص�������
	Battery_VeryLow=3 //��ص������ز���
	}BattStatusDef;

typedef enum
	{
  BattVdis_Waiting, //�ȴ���ʾ�׶�
	BattVdis_PrepareDis, //׼����ʾ
	BattVdis_DelayBeforeDisplay, //�ӳ�һ��ʱ��
	BattVdis_Show10V, //��ʾʮλ
	BattVdis_Gap10to1V, //ʮλ�͸�λ֮��ĵȴ�
	BattVdis_Show1V, //��ʾ��λ
	BattVdis_Gap1to0_1V, //��λ��ʮ��λ֮��ĵȴ�
	BattVdis_Show0_1V, //��ʾС�����һλ(0.1V)
	BattVdis_WaitShowChargeLvl, //�ȴ�һ��ʱ�����ʾ��ǰ����
	BattVdis_ShowChargeLvl, //��ʾ��ص����ĵȴ�
	}BattVshowFSMDef; //��ص�����ʾ����


//�ⲿ�ο�
extern bit IsBatteryAlert; //��ص͵羯�淢��
extern bit IsBatteryFault; //��ص͵������Ϸ���
extern xdata float Battery; //�˲�֮��ĵ�ص�ѹ
extern xdata BattVshowFSMDef VshowFSMState; //״̬��״̬	
	
//����
void BattDisplayTIM(void); //��ص�����ʾ��������
void TriggerVshowDisplay(void); //������ص�ѹ��ʾ
void DisplayVBattAtStart(void);
void BatteryTelemHandler(void);  //��ز�����ָʾ�ƿ���
bit LowPowerStrobe(void); //�͵�����ʾ��
	
#endif
