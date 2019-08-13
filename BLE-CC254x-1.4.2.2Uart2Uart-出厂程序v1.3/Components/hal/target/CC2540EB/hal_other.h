#ifndef HAL_OTHER_H
#define HAL_OTHER_H

#ifdef __cplusplus
extern "C"
{
#endif
  
//#define IO_HEAT                 P0_0
//#define IO_HEAT_BV              BV(0)
//#define IO_HEAT_SBIT            P0_0
//#define IO_HEAT_DDR             P0DIR
//#define IO_HEAT_POLARITY        !!
//#define HAL_OFF_IO_HEAT()       st( IO_HEAT_SBIT = IO_HEAT_POLARITY (0); )
//#define HAL_ON_IO_HEAT()        st( IO_HEAT_SBIT = IO_HEAT_POLARITY (1); )


extern void IO_HEAT_Init(void);
extern void IO_HEAT_PutSet(uint8 mode);



#endif
