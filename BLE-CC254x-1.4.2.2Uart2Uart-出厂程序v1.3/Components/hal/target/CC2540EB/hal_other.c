#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "osal.h"
#include "hal_board.h"
#include "hal_other.h"

#define Mode_Out                1
#define Mode_In                 0


/*************加热控制引脚************/
void IO_HEAT_Init(void)
{
//  IO_HEAT_PutSet(Mode_Out);
//  HAL_OFF_IO_HEAT();
//}
//void IO_HEAT_PutSet(uint8 mode)
//{
//  if(mode == Mode_In)
//  {
//    IO_HEAT_DDR &= ~IO_HEAT_BV;
//  }
//  if(mode == Mode_Out)
//  {
//    IO_HEAT_DDR |=  IO_HEAT_BV;
//  }
}
