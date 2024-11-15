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

/*****************************************************************************/
/** \file gpio.h
**
** 
**
** History:
** 
*****************************************************************************/
#ifndef __GPIO_H__
#define __GPIO_H__
/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include "cms8s6990.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/
/*----------------------------------------------------------------------------
 **GPIO����
 ---------------------------------------------------------------------------*/
#define  GPIO_PIN_0_MSK				(0x01)		/* GPIO Pin 0 mask */
#define  GPIO_PIN_1_MSK				(0x02)		/* GPIO Pin 1 mask */
#define  GPIO_PIN_2_MSK				(0x04)		/* GPIO Pin 2 mask */
#define  GPIO_PIN_3_MSK				(0x08)		/* GPIO Pin 3 mask */
#define  GPIO_PIN_4_MSK				(0x10)		/* GPIO Pin 4 mask */
#define  GPIO_PIN_5_MSK				(0x20)		/* GPIO Pin 5 mask */
#define  GPIO_PIN_6_MSK				(0x40)		/* GPIO Pin 6 mask */
#define  GPIO_PIN_7_MSK				(0x80)		/* GPIO Pin 7 mask */

#define  GPIO_PIN_0					(0x00)		/* GPIO Pin 0 number*/
#define  GPIO_PIN_1					(0x01)		/* GPIO Pin 1 number */
#define  GPIO_PIN_2					(0x02)		/* GPIO Pin 2 number */
#define  GPIO_PIN_3					(0x03)		/* GPIO Pin 3 number */
#define  GPIO_PIN_4					(0x04)		/* GPIO Pin 4 number */
#define  GPIO_PIN_5					(0x05)		/* GPIO Pin 5 number */
#define  GPIO_PIN_6					(0x06)		/* GPIO Pin 6 number */
#define  GPIO_PIN_7					(0x07)		/* GPIO Pin 7 number */

#define  GPIO0						(0x00)
#define  GPIO1						(0x01)
#define  GPIO2						(0x02)
#define  GPIO3						(0x03)

#define  GPIO_P00					(0x00)
#define  GPIO_P01					(0x01)
#define  GPIO_P02					(0x02)
#define  GPIO_P03					(0x03)
#define  GPIO_P04					(0x04)
#define  GPIO_P05					(0x05)
#define  GPIO_P13					(0x13)
#define  GPIO_P14					(0x14)
#define  GPIO_P15					(0x15)
#define  GPIO_P16					(0x16)
#define  GPIO_P17					(0x17)
#define  GPIO_P21					(0x21)
#define  GPIO_P22					(0x22)
#define  GPIO_P23					(0x23)
#define  GPIO_P24					(0x24)
#define  GPIO_P25					(0x25)
#define  GPIO_P26					(0x26)
#define  GPIO_P30					(0x30)
#define  GPIO_P31					(0x31)
#define  GPIO_P32					(0x32)
#define  GPIO_P35					(0x35)
#define  GPIO_P36					(0x36)

/*----------------------------------------------------------------------------
 **GPIO ����ģʽ����
---------------------------------------------------------------------------*/
typedef enum
	{
	GPIO_Input_Floating, //���븡��
	GPIO_IPU, //���������
	GPIO_IPD, //���������
	GPIO_Out_PP, //�������
	GPIO_Out_OD //��̬�������
	}GPIOModeDef;

typedef enum
	{
	GPIO_Slow_Slew,
	GPIO_Fast_Slew //
	}GPIOSlewRateDef;

typedef enum
	{
	GPIO_Low_Current,
	GPIO_High_Current, //GPIO��������
	}GPIOCurrentDef;
	
typedef struct
	{
	GPIOModeDef Mode;
	GPIOSlewRateDef Slew;
	GPIOCurrentDef DRVCurrent;
	}GPIOCfgDef;
/*----------------------------------------------------------------------------
 **GPIO �����ж�ģʽ
---------------------------------------------------------------------------*/
#define GPIO_Int_Disable 0x00 //�ر��ж�
#define GPIO_Int_Rising 0x01 //�����ش���
#define GPIO_Int_Falling 0x02 //�½��ش���
#define GPIO_Int_DualEdge 0x03 //˫���ش���

/*----------------------------------------------------------------------------
 ** IO���Ÿ���ģʽ
---------------------------------------------------------------------------*/
#define GPIO_AF_GPIO 0x00 //(����ΪGPIO)
#define GPIO_AF_Analog 0x01 //(����Ϊģ������)
#define GPIO_AF_CC0 0x04 //(��ʱ��2�Ƚ����ͨ��0)
#define GPIO_AF_CC1 0x05 //(��ʱ��2�Ƚ����ͨ��1)
#define GPIO_AF_CC2 0x06 //(��ʱ��2�Ƚ����ͨ��2)
#define GPIO_AF_CC3 0x07 //(��ʱ��2�Ƚ����ͨ��3)
#define GPIO_AF_U0TXD 0x08 //(����0����)
#define GPIO_AF_U0RXD 0x09 //(����0����)
#define GPIO_AF_U1TXD 0x0A //(����1����)
#define GPIO_AF_U1RXD 0x0B //(����1����)
#define GPIO_AF_SCL 0x0C //(�ڲ�IIC SCL)
#define GPIO_AF_SDA 0x0D //(�ڲ�IIC SDA)
#define GPIO_AF_SPINSS 0x0E //(SPI�ӻ�ģʽƬѡ�ź�)
#define GPIO_AF_SPICLK 0x0F //(SPIʱ���ź�)
#define GPIO_AF_SPIMOSI 0x10 //(SPI������������·)
#define GPIO_AF_SPIMISO 0x11 //(SPI�ӵ���������·)
#define GPIO_AF_PWMCH0 0x12 //(PWMͨ��0)
#define GPIO_AF_PWMCH1 0x13 //(PWMͨ��1)
#define GPIO_AF_PWMCH2 0x14 //(PWMͨ��2)
#define GPIO_AF_PWMCH3 0x15 //(PWMͨ��3)
#define GPIO_AF_PWMCH4 0x16 //(PWMͨ��4)
#define GPIO_AF_PWMCH5 0x17 //(PWMͨ��5)
#define GPIO_AF_BEEP 0x18 //(�������������)
#define GPIO_AF_ACMP0_O 0x1A //(ģ��Ƚ����Ƚ����0)
#define GPIO_AF_ACMP1_O 0x1B //(ģ��Ƚ����Ƚ����1)

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
void GPIO_SetMUXMode(uint8_t Port, uint8_t PinNum, uint8_t Mode);

/********************************************************************************
 ** \brief	 GPIO_SET_PS_MODE
 **			 ���ö˿����빦�ܷ���
 ** \param [in]  Ps_mode��PS_INT0��PS_INT1..... PS_FB
 **				 gpio:	GPIO_P00 .... GPIO_P36 
 ** \return  none
********************************************************************************/
#define  GPIO_SET_PS_MODE(Ps_mode,gpio)		(Ps_mode = gpio)	

/********************************************************************************
 ** \brief	 GPIO_ENABLE_OUTPUT
 **			 ʹ��GPIOΪ�������ģʽ
 ** \param [in] PortTRIS ������Ĵ��� P0TRIS ��P1TRIS ��P2TRIS ��P3TRIS 
 **            PinNum 
 ** \return  none
 ******************************************************************************/
#define	 GPIO_ENABLE_OUTPUT(PortTRIS, PinNum)	(PortTRIS |= (1<<PinNum))

/********************************************************************************
 ** \brief	 GPIO_ENABLE_OUTPUT
 **			 ʹ��GPIOΪ��̬����ģʽ
 ** \param [in] PorttRIS ������Ĵ��� P0TRIS ��P1TRIS ��P2TRIS ��P3TRIS 
 **            PinNum 
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_INPUT(PortTRIS, PinNum)	(PortTRIS &= ~(1<<PinNum))

/********************************************************************************
 ** \brief	 GPIO_ENABLE_OD
 **			 GPIO��©���ܿ���
 ** \param [in] PorttOD  :��©���ܼĴ�����P0OD��P1OD��P2OD��P3OD
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_OD(PortOD, PinNum)			(PortOD |= (1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_DISABLE_OD
 **			 GPIO��©���ܹر�
 ** \param [in] PorttOD  :��©���ܼĴ�����P0OD��P1OD��P2OD��P3OD
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_DISABLE_OD(PortOD, PinNum)		(PortOD &= ~(1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_ENABLE_UP
 **			 GPIO�������ܿ���
 ** \param [in] PorttUP  :�������ܼĴ�����P0UP��P1UP��P2UP��P3UP
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_UP(PortUP, PinNum)			(PortUP |= (1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_DISABLE_UP
 **			 GPIO�������ܹر�
 ** \param [in] PorttUP  :�������ܼĴ�����P0UP��P1UP��P2UP��P3UP
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_DISABLE_UP(PortUP, PinNum)		(PortUP &= ~(1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_ENABLE_RD
 **			 GPIO�������ܿ���
 ** \param [in] PorttRD  :�������ܼĴ�����P0RD��P1RD��P2RD��P3RD
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_RD(PortRD, PinNum)			(PortRD |= (1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_DISABLE_RD
 **			 GPIO�������ܹر�
 ** \param [in] PorttRD  :�������ܼĴ�����P0RD��P1RD��P2RD��P3RD
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_DISABLE_RD(PortRD, PinNum)		(PortRD &= ~(1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_ENABLE_LOW_CURRENT
 **			 GPIO�������������ܿ���
 ** \param [in] PorttDR  :�������ܼĴ�����P0DR��P1DR��P2DR��P3DR
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_LOW_CURRENT(PortDR, PinNum)	(PortDR |= (1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_ENABLE_LOW_CURRENT
 **			 GPIO�������������ܹر�
 ** \param [in] PorttDR  :�������ܼĴ�����P0DR��P1DR��P2DR��P3DR
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_DISABLE_LOW_CURRENT(PortDR, PinNum)	(PortDR &= ~(1<<PinNum))

/********************************************************************************
 ** \brief	 GPIO_ENABLE_LOW_CURRENT
 **			 GPIO��б�ʹ��ܿ���
 ** \param [in] PorttSR  :б�ʿ��ƹ��ܼĴ�����P0SR��P1SR��P2SR��P3SR
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_SLOW_SLOPE(PortSR, PinNum)      (PortSR |= (1<<PinNum))
/********************************************************************************
 ** \brief	 GPIO_ENABLE_LOW_CURRENT
 **			 GPIO��б�ʹ��ܹر�
 ** \param [in] PorttSR  :б�ʿ��ƹ��ܼĴ�����P0SR��P1SR��P2SR��P3SR
 **            PinNum  : 0~7
 ** \return  none
 ******************************************************************************/
#define  GPIO_DISABLE_SLOW_SLOPE(PortSR, PinNum)     (PortSR &= ~(1<<PinNum))


/*----------------------------------------------------------------------------
 **GPIO �����жϴ���ģʽ
/*---------------------------------------------------------------------------*/
#define  GPIO_INT_RISING	(0x01)			/*�����ش����ж�*/
#define  GPIO_INT_FALLING	(0x02)			/*�½��ش����ж�*/
#define  GPIO_INT_BOTH_EDGE	(0x03)			/*�������½��ؾ������ж�*/

/********************************************************************************
 ** \brief	 GPIO_SET_INT_MODE
 **			 ����IO�ڵ��ⲿ�жϹ���
 ** \param [in] PorttEICFG ��IO�ڵ��ⲿ�ж�ģʽ���üĴ���
 **            PinIntMode ��IO�ڵĸ��ù��� 
 ** \return  none
 ******************************************************************************/
#define  GPIO_SET_INT_MODE(PortEICFG, PinIntMode)	(PortEICFG = PinIntMode)

/********************************************************************************
 ** \brief	 GPIO_ENABLE_INT_MODE
 **			ʹ��IO�ڵ��ⲿ�жϹ���
 ** \param [in] PorttEXTIE ��IO�ڵ��ⲿ�жϼĴ���
 **            PinMSK��GPIO_PIN_0_MSK .. GPIO_PIN_7_MSK
 ** \return  none
 ******************************************************************************/
#define  GPIO_ENABLE_INT_MODE(PortEXTIE,PinMSK)		(PortEXTIE |= PinMSK)	

/********************************************************************************
 ** \brief	 GPIO_DISABLE_INT_MODE
 **			�ر�IO�ڵ��ⲿ�жϹ���
 ** \param [in] PorttEXTIE ��IO�ڵ��ⲿ�жϼĴ���
 **            PinMSK: GPIO_PIN_0_MSK .. GPIO_PIN_7_MSK
 ** \return  none
 ******************************************************************************/
#define  GPIO_DISABLE_INT_MODE(PortEXTIE,PinMSK)	(PortEXTIE &= ~PinMSK)	

/*****************************************************************************/
/* Global type definitions ('typedef') */
/*****************************************************************************/



/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source) */
/*****************************************************************************/


/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source) */
/*****************************************************************************/
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
void GPIO_ConfigGPIOMode( uint8_t Port, uint8_t PinMSK,GPIOCfgDef *Cfg);

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
void GPIO_EnableInt(uint8_t Port, uint8_t PinMSK);
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
void GPIO_DisableInt(uint8_t Port, uint8_t PinMSK);

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
uint8_t  GPIO_GetIntFlag(uint8_t Port, uint8_t PinNum);
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
void GPIO_ClearIntFlag(uint8_t Port, uint8_t PinNum);
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
void GPIO_SetExtIntMode(uint8_t Port, uint8_t PinNum, uint8_t Mode);

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
void GPIO_WriteBit(uint8_t Port, uint8_t PinNum,bit Val);

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
bit GPIO_CheckIfIntEnabled(uint8_t Port, uint8_t PinMSK);

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
int GPIO_GetExtIntMode(uint8_t Port, uint8_t PinNum);

#endif /* __GPIO_H__ */






