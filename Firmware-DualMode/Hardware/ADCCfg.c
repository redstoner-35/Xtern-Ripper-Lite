#include "cms8s6990.h"
#include "PinDefs.h"
#include "ADCCfg.h"
#include "ADCAsync.h"
#include "delay.h"

/**********************************************************************
���º���ΪADC�첽ת������ʵ�ֹ���������ڲ��������Լ�������ڲ�ȫ�ֱ�
�����벻Ҫ�����޸ĺ������ݣ������ڳ��˱��ļ��ڵ������κεط����ã�����
�ᵼ��ADC���湤���쳣��	
**********************************************************************/
static xdata ADCConvertTemp ADCTemp;
static ADCAsyncStateDef ADCState;	
static xdata char ADCConvertQueue[ADCConvertQueueDepth];	
xdata bool IsNotAllowAsync;	 //�Ƿ�����ADC�����������첽ģʽ
	
//��ADC�ύ����	
static void ADC_SubmitMisson(char Ch)	
	{
	//��鴫���ͨ�������Ƿ�Ϸ�
	if(ADCTemp.IsMissionProcessing)return;
	if(ADC_CheckIfChInvalid(Ch))return; 
	//���г�ʼ��
	ADCTemp.avgbuf=0;
	ADCTemp.Count=0;
	ADCTemp.Ch=Ch;
	ADCTemp.IsMissionProcessing=true;
	//����ADCͨ��		
	if(Ch&0x10)ADCON0|=0x80;
	else ADCON0&=0x7F; //����ADCHS[4]
	ADCON1&=0xF0;
	ADCON1|=(Ch&0x0F); //����ADCHS[3:0]					
	//����ת��
	delay_us(150);  
	ADC_StartConv();
	}	

//��ȡ����
static int ADC_ReadBackResult(int *Result,char *Queue)	
	{
	//ADCδ��ɱ���ת��
	if(ADC_GetIfStillConv())return 0; 
	//��ȡ���
	ADCTemp.Count++; //��ֵ+1
	ADCTemp.avgbuf+=(long)ADC_ReadConvResult(); //��AD�Ĵ�����ȡ���������ƽ���ۼ�
	if(ADCTemp.Count<ADCAverageCount) 
		{
		ADC_StartConv();
		return 0;//ƽ������δ������������ADC������һ�ֵĴ���
		}
	//���ת�������ؽ����׼�������ύ�µ�����
	ADCTemp.avgbuf/=(long)ADCAverageCount;
	*Result=(int)ADCTemp.avgbuf; //���ؽ��
	*Queue=ADCTemp.Ch; //����ת���Ķ���
	ADCTemp.IsMissionProcessing=false; //�����Ѵ������
  return 1;	
	}

//ADC���õ�ѹ�ο�	
static void ADC_SetVREF(bit IsUsingVDD)
	{
	ADC_DisableCmd(); //ת��ADC��׼��Ҫ��ʱ�ر�ADC	
	_nop_();
	ADC_SetVREFReg(IsUsingVDD); //����оƬ�ڲ���׼
	_nop_();
	ADC_EnableCmd(); //��׼�л���ϣ���������
	}
//ת����Ϻ�д�������
char CalcNTCTemp(bool *IsNTCOK,unsigned long NTCRes); //��������		

static void ADC_WriteOutputBuf(int ADCResult,char Ch)
	{
	float Buf,Rt,Vadc;
	unsigned long NTCRES;
	extern xdata char VbattCellCount;	
	//����ADC
	Rt=ADC_IsUsingIVREF()?ADCVREF:Data.MCUVDD; //���ݻ�׼���õõ�AD��ǰ�Ļ�׼��ѹ
	Vadc=(float)ADCResult*(Rt/(float)4096);//��ADֵת��Ϊԭʼ��ѹ
	//״̬��
  switch(Ch)
		{
		//����ο���ѹ
		case ADC_INTVREFCh:
			Buf=ADCBGVREF*(float)4096/(float)ADCResult;
			Data.MCUVDD=Buf; //�����MCUVDD(VREF)
		  break; 
		//�����ص�ѹ
		case VBATInputAIN:
			Buf=(float)VBattLowerResK/(float)(VBattLowerResK+VBattUpperResK);//�������ѹ�����ϵ��
			Data.RawBattVolt=Vadc/Buf; //���ݷ�ѹϵ�����Ƴ���ص�ѹ
			if(Data.RawBattVolt<3.0)Data.RawBattVolt-=0.1; //��������
		  Data.BatteryVoltage=Data.RawBattVolt/(float)VbattCellCount; //��3�ڵ�ص��ܵ�ѹת��Ϊ���ڵ�صĵ�ѹ
		  break;
	  //���������ѹ
		case VOUTFBAIN:		
			Buf=(float)VoutLowerResK/(float)(VoutLowerResK+VoutUpperResK);//�������ѹ�����ϵ��
			Data.OutputVoltage=Vadc/Buf; //���ݷ�ѹϵ�����Ƴ�DCDC�����ѹ
			if(Data.OutputVoltage<3.0)Data.OutputVoltage-=0.1; //��������			
		  break;
    //�����¶�
		case NTCInputAIN:
			Rt=((float)NTCUpperResValueK*Vadc)/(Data.MCUVDD-Vadc);//�õ�NTC+��Ƭ��IO��ͨ����Ĵ������¶�ֵ
			Rt*=1000; //����ֵ��KŷתΪ��
			NTCRES=(unsigned long)Rt; //ȡ��
			Data.Systemp=CalcNTCTemp(&Data.IsNTCOK,NTCRES); //�����¶�
			break;
		}
  }

//ADC�첽�������������
static void ADCEngineHandler(void)
	{
	int i,result;
	char Ch;
	//ת��ѭ��
  do
		{
		if(ADCState==ADC_ConvertComplete)ADCState=ADC_SubmitQueue; //���һ��ת����������¿�ʼ
		switch(ADCState)
			{		
			//��ʼ�ύת������
			case ADC_SubmitQueue: 
				ADC_SetVREF(1); //ÿ���ύ����֮ǰ�����û�׼ʹ��MCUVDD��ת���¶Ⱥ�MCUVDD��ѹ
				for(i=0;i<ADCConvertQueueDepth;i++)ADCConvertQueue[i]=ADCChQueue[i]; //��ת��������������ݸ��ƹ�ȥ
			  ADCState=ADC_SubmitChFromQueue;
			  break;
		  //��ADCת���߳��ύ����
			case ADC_SubmitChFromQueue: 
			  i=0;
			  while(i<ADCConvertQueueDepth)
					{
					if(!ADC_CheckIfChInvalid(ADCConvertQueue[i]))break; //�ҵ�������δ��ɵĺϷ�ת����Ŀ
					i++;
					}
				//��ת����Ŀδ���
				if(i<ADCConvertQueueDepth)	
					{
					Ch=ADCConvertQueue[i]; //���Ŀ���ͨ��ֵ
          if(Ch==VBATInputAIN||Ch==VOUTFBAIN)ADC_SetVREF(0); //��غ������ѹת��ʹ���ڲ�����ͨ��
					ADC_SubmitMisson(Ch); //�ύ��Ŀ
					ADCState=ADC_WaitMissionDone;
					}
				//����ת������ɣ���ת����ɽ׶�
  			else ADCState=ADC_ConvertComplete;		
			  break;	
			//�ύ�߳������ȴ������������
      case ADC_WaitMissionDone:
          if(!ADC_ReadBackResult(&result,&Ch))break; //���Զ�ȡ�����ת��δ��������
			    ADC_WriteOutputBuf(result,Ch);
			    for(i=0;i<4;i++)if(ADCConvertQueue[i]==Ch)ADCConvertQueue[i]=-2; //����ǰ�Ѿ����ת��������ͨ������Ϊ-2���ת�����
			    ADCState=ADC_SubmitChFromQueue; //���»ص��ύ����Ľ׶�
			    break;
			//�������������
			case ADC_ConvertComplete:break;
			//�����κηǷ�״̬��ת����ʼ�׶�
			default:ADCState=ADC_SubmitQueue;
			}
		}
	while(IsNotAllowAsync&&ADCState!=ADC_ConvertComplete);
	}	
	
/**********************************************************************
���º���ΪADC�첽ת�������Լ�ADC�ĳ�ʼ���ͳ��ܲ��������������ȡ�ⲿͨ
���ĵ�ѹ����������ⲿ�������á��������ڳ�ʼ���׶κ��������ڵ���������
��ĺ�����ADC���г�ʼ���ͳ��ܲ������Լ����������ADC�����첽������
**********************************************************************/	
ADCResultStrDef Data;	 //ADC������
	
//�������ݻ�ȡ	
void SystemTelemHandler(void)
	{
  //����ADC�첽����
	ADCEngineHandler();
	}	
	
//�ر�ADC
void ADC_DeInit(void)
	{
	GPIOCfgDef ADCDeInitCfg;
  char i;		
	//���üĴ����ر�ADC
	ADCON1=0x00; //�ر�ADC
	ADCLDO=0x00; //�ر�Ƭ�ڻ�׼
	//��ն��в���λ�첽����
  IsNotAllowAsync=1;		
	ADCState=ADC_SubmitQueue;
	for(i=0;i<ADCConvertQueueDepth;i++)ADCConvertQueue[i]=-2;	
	ADCTemp.avgbuf=0;
	ADCTemp.Count=0;
	ADCTemp.Ch=0;
	ADCTemp.IsMissionProcessing=false;	
	//��GPIO����Ϊ��ͨģʽ
  GPIO_SetMUXMode(NTCInputIOG,NTCInputIOx,GPIO_AF_GPIO);
	GPIO_SetMUXMode(VOUTFBIOG,VOUTFBIOx,GPIO_AF_GPIO);
	//����Ϊ�������
	ADCDeInitCfg.Mode=GPIO_Out_PP;
  ADCDeInitCfg.Slew=GPIO_Slow_Slew;		
	ADCDeInitCfg.DRVCurrent=GPIO_Low_Current; //����Ϊ�͵����������
	GPIO_ConfigGPIOMode(NTCInputIOG,GPIOMask(NTCInputIOx),&ADCDeInitCfg); 
  GPIO_ConfigGPIOMode(VOUTFBIOG,GPIOMask(VOUTFBIOx),&ADCDeInitCfg); 
	//ȫ�����0
  GPIO_WriteBit(NTCInputIOG,NTCInputIOx,0);
	GPIO_WriteBit(VOUTFBIOG,VOUTFBIOx,0);
	GPIO_WriteBit(NTCENIOG,NTCENIOx,0); //������=0�ر�NTC��Դ
	}

//ADC��ʼ��
void ADC_Init(void)
	{
	GPIOCfgDef ADCInitCfg;
	//��ʼ��GPIO
	ADCInitCfg.Mode=GPIO_Input_Floating;
  ADCInitCfg.Slew=GPIO_Slow_Slew;		
	ADCInitCfg.DRVCurrent=GPIO_Low_Current; //����Ϊ��������	
	
  GPIO_ConfigGPIOMode(VOUTFBIOG,GPIOMask(VOUTFBIOG),&ADCInitCfg); //����Ӧ��IO����Ϊ��������
	GPIO_ConfigGPIOMode(VBATInputIOG,GPIOMask(VBATInputIOx),&ADCInitCfg); 
	GPIO_ConfigGPIOMode(NTCInputIOG,GPIOMask(NTCInputIOx),&ADCInitCfg); 	
		
  GPIO_SetMUXMode(NTCInputIOG,NTCInputIOx,GPIO_AF_Analog);
	GPIO_SetMUXMode(VOUTFBIOG,VOUTFBIOx,GPIO_AF_Analog);
	GPIO_SetMUXMode(VBATInputIOG,VBATInputIOx,GPIO_AF_Analog); //��GPIO��������Ϊģ������
	//���ò���NTC��ѹ����Ĺ���
	ADCInitCfg.Mode=GPIO_Out_PP;
  ADCInitCfg.Slew=GPIO_Slow_Slew;		
	ADCInitCfg.DRVCurrent=GPIO_High_Current; 	 //������������
		
	GPIO_SetMUXMode(NTCENIOG,NTCENIOx,GPIO_AF_GPIO);
  GPIO_ConfigGPIOMode(NTCENIOG,GPIOMask(NTCENIOx),&ADCInitCfg);		
  GPIO_WriteBit(NTCENIOG,NTCENIOx,1); //������=1��NTC��Դ
	//����ADC
	ADCON0=0x40; //AN31=�ڲ�1.2V��׼������Ҷ���
	ADCON1=0x60; //Fadc=Fsys/128=375KHz
	ADCON2=0x00; //�ر�ADCӲ���������ܣ�ʹ�������������ADC
	ADCMPC=0x00; //�ر�ADC�Ƚ�������ɲ������
	ADDLYL=0x00; //��ADCӲ������������ʱ����Ϊ0
	ADCMPH=0x0F;
	ADCMPL=0xFF; //ADC�Ƚ���Ĭ��ֵ����Ϊ0x0FFF
  ADCLDO=0xA0; //ʹ��оƬ����ADC��׼�����2.0V
	//��ʼ���첽ADC����
	ADCState=ADC_SubmitQueue;
	ADCTemp.avgbuf=0;
	ADCTemp.Count=0;
	ADCTemp.Ch=0;
	ADCTemp.IsMissionProcessing=false;
	IsNotAllowAsync=true; //��ʼ��ʱ��ֹ�첽����	
	//��ȡһ���ʼ��ϵͳ����
	ADC_EnableCmd(); //ʹ��ADCģ��
  SystemTelemHandler();
	}	
