#ifndef __OLED_H
#define __OLED_H 

#include "stdlib.h"	
#include "main.h"	


//-----------------OLED端口定义---------------- 

#define OLED_SCL_Clr() HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET)//SCL/D0
#define OLED_SCL_Set() HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET)

#define OLED_SDA_Clr() HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_RESET)//SDA/D1
#define OLED_SDA_Set() HAL_GPIO_WritePin(GPIOA,GPIO_PIN_7,GPIO_PIN_SET)

#define OLED_RES_Clr() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET)//RES
#define OLED_RES_Set() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET)

#define OLED_DC_Clr()  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_RESET)//DC
#define OLED_DC_Set()  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_SET)
 		     
#define OLED_CS_Clr()  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET)//CS
#define OLED_CS_Set()  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET)

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

void OLED_ClearPoint(unsigned char x,unsigned char y);
void OLED_ColorTurn(unsigned char i);
void OLED_DisplayTurn(unsigned char i);
void OLED_WR_Byte(unsigned char dat,unsigned char mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t);
void OLED_DrawLine(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char mode);
void OLED_DrawCircle(unsigned char x,unsigned char y,unsigned char r);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr,unsigned char size1,unsigned char mode);
void OLED_ShowChar6x8(unsigned char x,unsigned char y,unsigned char chr,unsigned char mode);
void OLED_ShowString(unsigned char x,unsigned char y,unsigned char *chr,unsigned char size1,unsigned char mode);
void OLED_ShowNum(unsigned char x,unsigned char y,unsigned long num,unsigned char len,unsigned char size1,unsigned char mode);
void OLED_ShowChinese(unsigned char x,unsigned char y,unsigned char num,unsigned char size1,unsigned char mode);
void OLED_ScrollDisplay(unsigned char num,unsigned char space,unsigned char mode);
void OLED_ShowPicture(unsigned char x,unsigned char y,unsigned char sizex,unsigned char sizey,unsigned char BMP[],unsigned char mode);
void OLED_Init(void);
void OLED_Windows(void);
#endif

