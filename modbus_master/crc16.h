 /** \addtogroup Sistema
 *  @{
 */

/** \addtogroup modbus
 *  @{
 */
 
  /** \defgroup crc16 CRC16
 *  @{
 */
 
/******************************************************************************/
/* crc16.h                                                                    */
/******************************************************************************/


#ifndef __CRC16_H
#define __CRC16_H

#include "data_types.h"

/**************************** Func declarations *******************************/
uint16_t ModbusCrc16(const uint8_t * const _pBuff, uint32_t _len);



#endif

/** @} */
/** @} */
/** @} */
