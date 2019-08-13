#ifndef _LCD_12848_H_
#define _LCD_12848_H_

#define IO_LcdBL              P1_7
#define IO_LcdBL_BV           BV(7)
#define IO_LcdBL_SBIT         P1_7
#define IO_LcdBL_DDR          P1DIR
#define IO_LcdBL_POLARITY     !!
#define HAL_OFF_IO_LcdBL()    st( IO_LcdBL_SBIT = IO_LcdBL_POLARITY (1); )
#define HAL_ON_IO_LcdBL()     st( IO_LcdBL_SBIT = IO_LcdBL_POLARITY (0); )

#define IO_LcdRES              P1_5
#define IO_LcdRES_BV           BV(5)
#define IO_LcdRES_SBIT         P1_5
#define IO_LcdRES_DDR          P1DIR
#define IO_LcdRES_POLARITY     !!
#define HAL_OFF_IO_LcdRES()    st( IO_LcdRES_SBIT = IO_LcdRES_POLARITY (0); )
#define HAL_ON_IO_LcdRES()     st( IO_LcdRES_SBIT = IO_LcdRES_POLARITY (1); )

#define IO_LcdA0               P1_4
#define IO_LcdA0_BV            BV(4)
#define IO_LcdA0_SBIT          P1_4
#define IO_LcdA0_DDR           P1DIR
#define IO_LcdA0_POLARITY      !!
#define HAL_OFF_IO_LcdA0()     st( IO_LcdA0_SBIT = IO_LcdA0_POLARITY (0); )
#define HAL_ON_IO_LcdA0()      st( IO_LcdA0_SBIT = IO_LcdA0_POLARITY (1); )

#define IO_LcdSCL              P1_3
#define IO_LcdSCL_BV           BV(3)
#define IO_LcdSCL_SBIT         P1_3
#define IO_LcdSCL_DDR          P1DIR
#define IO_LcdSCL_POLARITY     !!
#define HAL_OFF_IO_LcdSCL()    st( IO_LcdSCL_SBIT = IO_LcdSCL_POLARITY (0); )
#define HAL_ON_IO_LcdSCL()     st( IO_LcdSCL_SBIT = IO_LcdSCL_POLARITY (1); )

#define IO_LcdSDA_H            I2CIO =  0x02
#define IO_LcdSDA_L            I2CIO =  0x00

extern void LCD_12848_Init(void);
extern void IO_LCD_Init(void);
extern void Write_Com(uint8 para);
extern void Write_Data(uint8 para);
extern void LCD_12848_End(void);
extern void ZL_UI(void);
extern void YB_UI(void);
extern void LCD_Clear(void);
extern void ZL_UI_Update();
extern void YB_UI_Update();
extern void HZ_12x12_Display(const char (*font)[24],uint8 line,uint8 col);
extern void HZ_16x16_Display(const char (*font)[32],uint8 col);
extern void ASCII_6x12_Display(const char (*font)[12],uint8 line,uint8 col);
#endif
