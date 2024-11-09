/*******************************************************************************
* Copyright (C) 2019 China Micro Semiconductor Limited Company. All Rights Reserved.
*
* This software is owned and published by:
* CMS LLC, No 2609-10, Taurus Plaza, TaoyuanRoad, NanshanDistrict, Shenzhen, China.
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with CMS
* components. This software is licensed by CMS to be adapted only
* for use in systems utilizing CMS components. CMS shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein. CMS is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*/

/****************************************************************************/
/** \file gpio.c
**
** 
**
**	History:
**	
*****************************************************************************/
/****************************************************************************/
/*	include files
*****************************************************************************/
#include "gpio.h"

/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
****************************************************************************/

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
****************************************************************************/

/****************************************************************************/
/*	Local type definitions('typedef')
****************************************************************************/

/****************************************************************************/
/*	Local variable  definitions('static')
****************************************************************************/

/****************************************************************************/
/*	Local function prototypes('static')
****************************************************************************/

/****************************************************************************/
/*	Function implementation - global ('extern') and local('static')
****************************************************************************/
/****************************************************************************
 ** \brief	 GPIO_ConfigGPIOMode
 **			 ����GPIOģʽ
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			   PinMSK: GPIO_PIN_0_MSK ~ GPIO_PIN_7_MSK	
 **        Cfg : ���ýṹ��
 **         |-Mode:	GPIO_Input_Floating(���븡��)
 ** 				| 			GPIO_IPU(���������)
 **					|				GPIO_IPD(���������)
 **					|				GPIO_Out_PP(�������)
 **					| 			GPIO_Out_OD(��©���)
 **					|
 **					|-Slew: GPIO_Slow_Slew(��б��)  GPIO_Fast_Slew(��б��)
 **					|-DRVCurrent: GPIO_Low_Current(�͵���)  GPIO_High_Current(�ߵ���) 
 ** \return  none
 ** \note   
 ***************************************************************************/
void GPIO_ConfigGPIOMode( uint8_t Port, uint8_t PinMSK,GPIOCfgDef *Cfg)
{
	
	int TRISMSK,ODMSK,UPMSK,RDMSK,DRMSK,SRMSK;
	//����ģʽ������Ӧ��mask
	switch(Cfg->Mode)
		{
		case GPIO_Input_Floating: //���븡��
			 TRISMSK=0xFF&(~PinMSK); //TRIS=0
		   ODMSK=0xFF&(~PinMSK); //OD=0
		   UPMSK=0xFF&(~PinMSK); //UR=0
		   RDMSK=0xFF&(~PinMSK); //RD=0
		   break;
		case GPIO_IPU: //���������
			 TRISMSK=0xFF&(~PinMSK); //TRIS=0
		   ODMSK=0xFF&(~PinMSK); //OD=0
		   UPMSK=0x100|PinMSK; //UR=1
		   RDMSK=0xFF&(~PinMSK); //RD=0
		   break;
		case GPIO_IPD: //���������
			 TRISMSK=0xFF&(~PinMSK); //TRIS=0
		   ODMSK=0xFF&(~PinMSK); //OD=0
		   UPMSK=0xFF&(~PinMSK); //UR=0
		   RDMSK=0x100|PinMSK; //RD=1
		   break;			
		case GPIO_Out_PP: //�������
		case GPIO_Out_OD: //��̬�������
		   UPMSK=0xFF&(~PinMSK); //UR=0
		   RDMSK=0xFF&(~PinMSK); //RD=0			
		   TRISMSK=0x100|PinMSK; //TRIS=1
		   ODMSK=(Cfg->Mode==GPIO_Out_OD)?0x100|PinMSK:0xFF&(~PinMSK);
		}
	SRMSK=(Cfg->Slew==GPIO_Slow_Slew)?0x100|PinMSK:0xFF&(~PinMSK);
	DRMSK=(Cfg->DRVCurrent==GPIO_Low_Current)?0x100|PinMSK:0xFF&(~PinMSK);
	//��maskд����Ӧ�Ĵ���
	switch(Port)
	{
		case GPIO0:
			P0TRIS=(TRISMSK&0x100)?P0TRIS|(TRISMSK&0xFF):P0TRIS&(TRISMSK&0xFF);
	    P0OD=(ODMSK&0x100)?P0OD|(ODMSK&0xFF):P0OD&(ODMSK&0xFF);
		  P0UP=(UPMSK&0x100)?P0UP|(UPMSK&0xFF):P0UP&(UPMSK&0xFF);
		  P0RD=(RDMSK&0x100)?P0RD|(RDMSK&0xFF):P0RD&(RDMSK&0xFF);
		  P0DR=(DRMSK&0x100)?P0DR|(DRMSK&0xFF):P0DR&(DRMSK&0xFF);
	    P0SR=(SRMSK&0x100)?P0SR|(SRMSK&0xFF):P0SR&(SRMSK&0xFF);
		  break;
	 case GPIO1:
			P1TRIS=(TRISMSK&0x100)?P1TRIS|(TRISMSK&0xFF):P1TRIS&(TRISMSK&0xFF);
	    P1OD=(ODMSK&0x100)?P1OD|(ODMSK&0xFF):P1OD&(ODMSK&0xFF);
		  P1UP=(UPMSK&0x100)?P1UP|(UPMSK&0xFF):P1UP&(UPMSK&0xFF);
		  P1RD=(RDMSK&0x100)?P1RD|(RDMSK&0xFF):P1RD&(RDMSK&0xFF);
		  P1DR=(DRMSK&0x100)?P1DR|(DRMSK&0xFF):P1DR&(DRMSK&0xFF);
	    P1SR=(SRMSK&0x100)?P1SR|(SRMSK&0xFF):P1SR&(SRMSK&0xFF);	
	    break;
	 case GPIO2:
			P2TRIS=(TRISMSK&0x100)?P2TRIS|(TRISMSK&0xFF):P2TRIS&(TRISMSK&0xFF);
	    P2OD=(ODMSK&0x100)?P2OD|(ODMSK&0xFF):P2OD&(ODMSK&0xFF);
		  P2UP=(UPMSK&0x100)?P2UP|(UPMSK&0xFF):P2UP&(UPMSK&0xFF);
		  P2RD=(RDMSK&0x100)?P2RD|(RDMSK&0xFF):P2RD&(RDMSK&0xFF);
		  P2DR=(DRMSK&0x100)?P2DR|(DRMSK&0xFF):P2DR&(DRMSK&0xFF);
	    P2SR=(SRMSK&0x100)?P2SR|(SRMSK&0xFF):P2SR&(SRMSK&0xFF);	
	    break;
		case GPIO3:
			P3TRIS=(TRISMSK&0x100)?P3TRIS|(TRISMSK&0xFF):P3TRIS&(TRISMSK&0xFF);
	    P3OD=(ODMSK&0x100)?P3OD|(ODMSK&0xFF):P3OD&(ODMSK&0xFF);
		  P3UP=(UPMSK&0x100)?P3UP|(UPMSK&0xFF):P3UP&(UPMSK&0xFF);
		  P3RD=(RDMSK&0x100)?P3RD|(RDMSK&0xFF):P3RD&(RDMSK&0xFF);
		  P3DR=(DRMSK&0x100)?P3DR|(DRMSK&0xFF):P3DR&(DRMSK&0xFF);
	    P3SR=(SRMSK&0x100)?P3SR|(SRMSK&0xFF):P3SR&(SRMSK&0xFF);	
	    break;
	}
}

 /********************************************************************************
 ** \brief	 GPIO_EnableInt
 **			 ʹ��IO�ڵ��жϹ���
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinMSK: GPIO_PIN_0_MSK ~ GPIO_PIN_7_MSK			 
 ** \return  none
 ** \note   
 **  (1)P0��PinMSK����ֵ��Χ��GPIO_PIN_0_MSK ~ GPIO_PIN_5_MSK
 **  (2)P1��PinMSK����ֵ��Χ��GPIO_PIN_3_MSK ~ GPIO_PIN_7_MSK
 **  (3)P2��PinMSK����ֵ��Χ��GPIO_PIN_1_MSK ~ GPIO_PIN_6_MSK
 **  (4)P3��PinMSK����ֵ��Χ��GPIO_PIN_0_MSK ~ GPIO_PIN_2_MSK��GPIO_PIN_5_MSK��GPIO_PIN_6_MSK
 ******************************************************************************/
void GPIO_EnableInt(uint8_t Port, uint8_t PinMSK)
{
	switch(Port)
	{
		case GPIO0:
			P0EXTIE |= PinMSK;
			break;
		case GPIO1:
			P1EXTIE |= PinMSK;
			break;		
		case GPIO2:
			P2EXTIE |= PinMSK;
			break;	
		case GPIO3:
			P3EXTIE |= PinMSK;
			break;
		default:
			break;	
	}
}
 /********************************************************************************
 ** \brief	 GPIO_CheckIfIntEnabled
 **			 ����ӦIO�ڵ��жϹ����Ƿ񼤻�
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinMSK: GPIO_PIN_0_MSK ~ GPIO_PIN_7_MSK			 
 ** \return  bit������жϿ������򷵻�1�����򷵻�0
 ** \note   
 **  (1)P0��PinMSK����ֵ��Χ��GPIO_PIN_0_MSK ~ GPIO_PIN_5_MSK
 **  (2)P1��PinMSK����ֵ��Χ��GPIO_PIN_3_MSK ~ GPIO_PIN_7_MSK
 **  (3)P2��PinMSK����ֵ��Χ��GPIO_PIN_1_MSK ~ GPIO_PIN_6_MSK
 **  (4)P3��PinMSK����ֵ��Χ��GPIO_PIN_0_MSK ~ GPIO_PIN_2_MSK��GPIO_PIN_5_MSK��GPIO_PIN_6_MSK
 ******************************************************************************/
bit GPIO_CheckIfIntEnabled(uint8_t Port, uint8_t PinMSK)
{
switch(Port)
	{
		case GPIO0:
			if(P0EXTIE&PinMSK)return 1;
			break;
		case GPIO1:
			if(P1EXTIE&PinMSK)return 1;
			break;		
		case GPIO2:
			if(P2EXTIE&PinMSK)return 1;
			break;	
		case GPIO3:
			if(P3EXTIE&PinMSK)return 1;
			break;
	}
//δ�ҵ��Ϸ�ֵ������0
return 0;
}
/********************************************************************************
 ** \brief	 GPIO_DisableInt
 **			 �ر�IO�ڵ��жϹ���
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinMSK: GPIO_PIN_0_MSK ~ GPIO_PIN_7_MSK			 
 ** \return  none
 ** \note   
 **  (1)P0��PinMSK����ֵ��Χ��GPIO_PIN_0_MSK ~ GPIO_PIN_5_MSK
 **  (2)P1��PinMSK����ֵ��Χ��GPIO_PIN_3_MSK ~ GPIO_PIN_7_MSK
 **  (3)P2��PinMSK����ֵ��Χ��GPIO_PIN_1_MSK ~ GPIO_PIN_6_MSK
 **  (4)P3��PinMSK����ֵ��Χ��GPIO_PIN_0_MSK ~ GPIO_PIN_2_MSK��GPIO_PIN_5_MSK��GPIO_PIN_6_MSK
 ******************************************************************************/
void GPIO_DisableInt(uint8_t Port, uint8_t PinMSK)
{
	switch(Port)
	{
		case GPIO0:
			P0EXTIE &= ~PinMSK;
			break;
		case GPIO1:
			P1EXTIE &= ~PinMSK;
			break;		
		case GPIO2:
			P2EXTIE &= ~PinMSK;
			break;	
		case GPIO3:
			P3EXTIE &= ~PinMSK;
			break;
		default:
			break;	
	}
}
 /******************************************************************************
 ** \brief	 GPIO_WriteBit
 **			 ����GPIO����ĵ�ƽ
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinNum:  GPIO_PIN_0~GPIO_PIN_7 (0~7)		 
 **			    Val: 1(���Ӧ��GPIO�������1)
 **			         0(���Ӧ��GPIO�������0)
 ** \return  ��
 ** \note  
 **  (1)P0��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_5
 **  (2)P1��PinNum����ֵ��Χ��GPIO_PIN_3~GPIO_PIN_7
 **  (3)P2��PinNum����ֵ��Χ��GPIO_PIN_1~GPIO_PIN_6
 **  (4)P3��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_2��GPIO_PIN_5��GPIO_PIN_6
 ******************************************************************************/
void GPIO_WriteBit(uint8_t Port, uint8_t PinNum,bit Val)
{
 switch(Port)
		{
		case GPIO0:
			 if(PinNum>5)return;
			 if(Val)P0|=0x01<<PinNum;
		   else P0&=~(0x01<<PinNum);
		   break;
		case GPIO1:
			 if(PinNum>7||PinNum<3)return;
			 if(Val)P1|=0x01<<PinNum;
		   else P1&=~(0x01<<PinNum);
		   break;		
		case GPIO2:
			 if(PinNum>6||PinNum<1)return;
			 if(Val)P2|=0x01<<PinNum;
		   else P2&=~(0x01<<PinNum);
		   break;
		case GPIO3:
			 if(PinNum>6)return;
			 if(Val)P3|=0x01<<PinNum;
		   else P3&=~(0x01<<PinNum);
		   break;
		default:return;	
		}
}
 /******************************************************************************
 ** \brief	 GPIO_SetExtIntMode
 **			 �����ⲿ�жϵ���Ӧģʽ
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinNum:  GPIO_PIN_0~GPIO_PIN_7 (0~7)		 
 **			    Mode: GPIO_Int_Disable(�ر��ж�)
 **			          GPIO_Int_Rising(�������ش����ж�)
 **			          GPIO_Int_Falling(���½��ش����ж�)
 **			          GPIO_Int_DualEdge(�����غ��½��ض������ж�)
 ** \return  ��
 ** \note  
 **  (1)P0��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_5
 **  (2)P1��PinNum����ֵ��Χ��GPIO_PIN_3~GPIO_PIN_7
 **  (3)P2��PinNum����ֵ��Χ��GPIO_PIN_1~GPIO_PIN_6
 **  (4)P3��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_2��GPIO_PIN_5��GPIO_PIN_6
 ******************************************************************************/
void GPIO_SetExtIntMode(uint8_t Port, uint8_t PinNum, uint8_t Mode)
{
 //������
 volatile unsigned char xdata *PCFG;
 if(Mode>3)return;
 //��ʼ����
	switch(Port)
		{
		case GPIO0:
			 if(PinNum>5)return;
		   PCFG=&P00EICFG+PinNum;
		   break;
		case GPIO1:
			 if(PinNum>7||PinNum<3)return;
			 PCFG=&P13EICFG+(PinNum-3);
		   break;		
		case GPIO2:
			 if(PinNum>6||PinNum<1)return;
			 PCFG=&P21EICFG+(PinNum-1);
		   break;
		case GPIO3:
			 if(PinNum>6)return;
		   PCFG=&P30EICFG+PinNum;
		   break;
		default:return;	
		}
*PCFG=Mode&0x03; //д������ֵ
}

 /******************************************************************************
 ** \brief	 GPIO_GetExtIntMode
 **			 ��ȡָ��GPIO���ж�ģʽ
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinNum:  GPIO_PIN_0~GPIO_PIN_7 (0~7)		 
 **			   
 ** \return  int: GPIO_Int_Disable(�ر��ж�)
 **			          GPIO_Int_Rising(�������ش����ж�)
 **			          GPIO_Int_Falling(���½��ش����ж�)
 **			          GPIO_Int_DualEdge(�����غ��½��ض������ж�)
 ** \note  
 **  (1)P0��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_5
 **  (2)P1��PinNum����ֵ��Χ��GPIO_PIN_3~GPIO_PIN_7
 **  (3)P2��PinNum����ֵ��Χ��GPIO_PIN_1~GPIO_PIN_6
 **  (4)P3��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_2��GPIO_PIN_5��GPIO_PIN_6
 ******************************************************************************/
int GPIO_GetExtIntMode(uint8_t Port, uint8_t PinNum)
{
 uint8_t flag;
 volatile unsigned char xdata *PCFG;
 //��ȡ
	switch(Port)
		{
		case GPIO0:
			 if(PinNum>5)return GPIO_Int_Disable;
		   PCFG=&P00EICFG+PinNum;
		   flag=P0EXTIE&(1<<PinNum);
		   break;
		case GPIO1:
			 if(PinNum>7||PinNum<3)return GPIO_Int_Disable;
			 PCFG=&P13EICFG+(PinNum-3);
		   flag=P1EXTIE&(1<<PinNum);
		   break;		
		case GPIO2:
			 if(PinNum>6||PinNum<1)return GPIO_Int_Disable;
			 PCFG=&P21EICFG+(PinNum-1);
		   flag=P2EXTIE&(1<<PinNum);
		   break;
		case GPIO3:
			 if(PinNum>6)return GPIO_Int_Disable;
		   PCFG=&P30EICFG+PinNum;
		  flag=P3EXTIE&(1<<PinNum);
		   break;
		default:return GPIO_Int_Disable;	
		}
//��������ֵ
if(!flag)return GPIO_Int_Disable; //��Ӧ�Ĵ��������ж�
return *PCFG&0x03; //���ؼĴ���ֵ
}

/******************************************************************************
 ** \brief	 GPIO_SetMUXMode
 **			 ����GPIO���ù��ܵ���Ӧģʽ
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinNum:  GPIO_PIN_0~GPIO_PIN_7 (0~7)		 
 **			    Mode: GPIO_AF_GPIO(����ΪGPIO)
 **			          GPIO_AF_Analog(����Ϊģ������)
 **			          GPIO_AF_CC0(��ʱ��2�Ƚ����ͨ��0)
 **			          GPIO_AF_CC1(��ʱ��2�Ƚ����ͨ��1)
 **			          GPIO_AF_CC2(��ʱ��2�Ƚ����ͨ��2)
 **			          GPIO_AF_CC3(��ʱ��2�Ƚ����ͨ��3)
 **			          GPIO_AF_U0TXD(����0����)
 **			          GPIO_AF_U0RXD(����0����)
 **			          GPIO_AF_U1TXD(����1����)
 **			          GPIO_AF_U1RXD(����1����)
 **			          GPIO_AF_SCL(�ڲ�IIC SCL)
 **			          GPIO_AF_SDA(�ڲ�IIC SDA)
 **			          GPIO_AF_SPINSS(SPI�ӻ�ģʽƬѡ�ź�)
 **			          GPIO_AF_SPICLK(SPIʱ���ź�)
 **			          GPIO_AF_SPIMOSI(SPI������������·)
 **			          GPIO_AF_SPIMISO(SPI�ӵ���������·)
 **			          GPIO_AF_PWMCH0(PWMͨ��0)
 **			          GPIO_AF_PWMCH1(PWMͨ��1)
 **			          GPIO_AF_PWMCH2(PWMͨ��2)
 **			          GPIO_AF_PWMCH3(PWMͨ��3)
 **			          GPIO_AF_PWMCH4(PWMͨ��4)
 **			          GPIO_AF_PWMCH5(PWMͨ��5)
 **			          GPIO_AF_BEEP(�������������)
 **			          GPIO_AF_ACMP0_O(ģ��Ƚ����Ƚ����0)
 **			          GPIO_AF_ACMP1_O(ģ��Ƚ����Ƚ����1)
 ** \return  ��
 ** \note  
 **  (1)P0��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_5
 **  (2)P1��PinNum����ֵ��Χ��GPIO_PIN_3~GPIO_PIN_7
 **  (3)P2��PinNum����ֵ��Χ��GPIO_PIN_1~GPIO_PIN_6
 **  (4)P3��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_2��GPIO_PIN_5��GPIO_PIN_6
 ******************************************************************************/
void GPIO_SetMUXMode(uint8_t Port, uint8_t PinNum, uint8_t Mode)
{
//������
 volatile unsigned char xdata *PCFG;
 if(Mode==2||Mode==3)return;
 if(Mode==0x19||Mode>0x1C)return;
 //��ʼ����
	switch(Port)
		{
		case GPIO0:
			 if(PinNum>5)return;
		   PCFG=&P00CFG+PinNum;
		   break;
		case GPIO1:
			 if(PinNum>7||PinNum<3)return;
			 PCFG=&P13CFG+(PinNum-3);
		   break;		
		case GPIO2:
			 if(PinNum>6||PinNum<1)return;
			 PCFG=&P21CFG+(PinNum-1);
		   break;
		case GPIO3:
			 if(PinNum>6)return;
		   PCFG=&P30CFG+PinNum;
		   break;
		default:return;	
		}
*PCFG=Mode&0x1F; //д������ֵ
}

 /******************************************************************************
 ** \brief	 GPIO_GetIntFlag
 **			 ��ȡ�жϱ�־
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinNum:  GPIO_PIN_0~GPIO_PIN_7 (0~7)		 
 ** \return  0�����жϲ���
 **			 1�����жϲ���
 ** \note  
 **  (1)P0��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_5
 **  (2)P1��PinNum����ֵ��Χ��GPIO_PIN_3~GPIO_PIN_7
 **  (3)P2��PinNum����ֵ��Χ��GPIO_PIN_1~GPIO_PIN_6
 **  (4)P3��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_2��GPIO_PIN_5��GPIO_PIN_6
 ******************************************************************************/
uint8_t  GPIO_GetIntFlag(uint8_t Port, uint8_t PinNum)
{
	uint8_t PinIntFlag = 0;
	switch(Port)
	{
		case GPIO0:
			PinIntFlag = P0EXTIF & (1<<PinNum);
			break;
		case GPIO1:
			PinIntFlag = P1EXTIF & (1<<PinNum);
			break;		
		case GPIO2:
			PinIntFlag = P2EXTIF & (1<<PinNum);
			break;	
		case GPIO3:
			PinIntFlag = P3EXTIF & (1<<PinNum);
			break;
		default:
			break;	
	}
	return( (PinIntFlag)? 1:0);
}
 /********************************************************************************
 ** \brief	 GPIO_ClearIntFlag
 **			����жϱ�־λ
 ** \param [in] Port  �� GPIO0��GPIO1��GPIO2��GPIO3
 **			    PinNum:  GPIO_PIN_0~GPIO_PIN_7 (0~7)		 
 ** \return  none
 ** \note  
 **  (1)P0��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_5
 **  (2)P1��PinNum����ֵ��Χ��GPIO_PIN_3~GPIO_PIN_7
 **  (3)P2��PinNum����ֵ��Χ��GPIO_PIN_1~GPIO_PIN_6
 **  (4)P3��PinNum����ֵ��Χ��GPIO_PIN_0~GPIO_PIN_2��GPIO_PIN_5��GPIO_PIN_6   
 ******************************************************************************/
void GPIO_ClearIntFlag(uint8_t Port, uint8_t PinNum)
{
	switch(Port)
	{
		case GPIO0:
			P0EXTIF = 0xff &(~(1<<PinNum));
			break;
		case GPIO1:
			P1EXTIF = 0xff &(~(1<<PinNum));			
			break;		
		case GPIO2:
			P2EXTIF = 0xff &(~(1<<PinNum));
			break;	
		case GPIO3:
			P3EXTIF = 0xff &(~(1<<PinNum));
			break;
		default:
			break;	
	}
}