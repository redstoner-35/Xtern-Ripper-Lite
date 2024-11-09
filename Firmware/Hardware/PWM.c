#include "cms8s6990.h"
#include "PinDefs.h"
#include "GPIO.h"
#include "PWMCfg.h"

//ȫ�ֱ���
xdata float PWMDuty;
static bit IsPWMLoading; //PWM���ڼ�����
static bit IsNeedToEnableOutput; //�Ƿ���Ҫ�������
bit IsNeedToUploadPWM; //�Ƿ���Ҫ����PWM

//�ر�PWM��ʱ��
void PWM_DeInit(void)
	{
	//����Ϊ��ͨGPIO
  GPIO_SetMUXMode(PWMDACIOG,PWMDACIOx,GPIO_AF_GPIO);	
	//����Ϊ���0
	GPIO_WriteBit(PWMDACIOG,PWMDACIOx,0);	
	//�ر�PWMģ��
	PWMOE=0x00;
	PWMCNTE=0x00;
	}

//PWM��ʱ����ʼ��
void PWM_Init(void)
	{
	GPIOCfgDef PWMInitCfg;
	//���ýṹ��
	PWMInitCfg.Mode=GPIO_Out_PP;
  PWMInitCfg.Slew=GPIO_Slow_Slew;		
	PWMInitCfg.DRVCurrent=GPIO_High_Current; //��PWMDAC������Ҫ�ܸߵ�����б��
	//����GPIO
	GPIO_WriteBit(PWMDACIOG,PWMDACIOx,0);
	GPIO_ConfigGPIOMode(PWMDACIOG,GPIOMask(PWMDACIOx),&PWMInitCfg); 
	//���ø��ù���
  GPIO_SetMUXMode(PWMDACIOG,PWMDACIOx,GPIO_AF_PWMCH0);
	//����PWM������
	PWMCON=0x00; //PWMͨ��Ϊ��ͨ������ģʽ�����¼������رշǶԳƼ�������	
	PWMOE=0x01; //��PWM���ͨ��0
	PWM01PSC=0x01;  //��Ԥ��Ƶ���ͼ�����ʱ�� 
  PWM0DIV=0xff;   //��Fpwmcnt=Fsys=48MHz(����Ƶ)
  PWMPINV=0x00; //����ͨ��������Ϊ�������ģʽ
	PWMCNTM=0x01; //ͨ��0����Ϊ�Զ�����ģʽ
	PWMCNTCLR=0x01; //��ʼ��PWM��ʱ��λ��ʱ��
	PWMDTE=0x00; //�ر�����ʱ��
	PWMMASKD=0x00; 
	PWMMASKE=0x01; //PWM���빦�����ã�Ĭ��״̬�½�ֹͨ��0���
	//������������
	PWMP0H=(PWMStepConstant>>8)&0xFF;
	PWMP0L=PWMStepConstant&0xFF;	
	//����ռ�ձ�����
  PWMD0H=0;
	PWMD0L=0;	
	//��ʼ������
	PWMDuty=0;
	IsPWMLoading=0; 
	IsNeedToUploadPWM=0;
	//����PWM
	PWMCNTE=0x01; //ʹ��ͨ��0�ļ�������PWM��ʼ����
	PWMLOADEN=0x01; //����ͨ��0��PWMֵ
	while(PWMLOADEN!=0); //�ȴ����ؽ���
	}

//PWMǿ������ռ�ձ�
void PWM_ForceSetDuty(bit IsEnable)
	{
	PWMD0H=0x01;
	PWMD0L=IsEnable?0xFF:0;			
	PWMLOADEN=0x01; //��ʼ����
	while(PWMLOADEN!=0); //�ȴ����ؽ���
	PWMMASKE=IsEnable?0x00:0x01;  //���üĴ��������
	}	
	
//����PWM�ṹ���ڵ����ý������
void PWM_OutputCtrlHandler(void)	
	{
	int value;
	float buf;
	//�ж��Ƿ���Ҫ���ص��߼�����
	if(!IsNeedToUploadPWM)return; //����Ҫ����
	else if(IsPWMLoading) //���μ���������
		{
	  if(!PWMLOADEN)//���ؼĴ�����λΪ0����ʾ���سɹ�
			 {
			 PWMMASKE=IsNeedToEnableOutput?0x00:0x01; //����PWMMASKE�Ĵ����������״̬���ö�Ӧ��ͨ��
			 IsNeedToUploadPWM=0;
		   IsPWMLoading=0;  //���ڼ���״̬Ϊ���
			 }
	  return;
		}
	//PWMռ�ձ�����
	if(PWMDuty>100)PWMDuty=100;
	if(PWMDuty<0)PWMDuty=0;
	//����װ����ֵ
	IsNeedToEnableOutput=buf?1:0; //�Ƿ���Ҫ�������
	buf=PWMDuty*(float)PWMStepConstant;
	buf/=(float)100;
	value=(int)buf;
	PWMD0H=(value>>8)&0xFF;
	PWMD0L=value&0xFF;			
	//PWM�Ĵ�����ֵ��װ�룬Ӧ����ֵ		
	IsPWMLoading=1; //��Ǽ��ع��̽�����
	PWMLOADEN=0x01; //��ʼ����
	}
