/************************************************************************************//**
* \file         cancomm.h
* \brief        Header file of the library for convenient access to CAN communication.
*
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*           Copyright (c) 2022 by PragmaticLinux     All rights reserved
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* This library is free software; you can redistribute it and/or modify it under the terms
* of the GNU Lesser General Public License as published by the Free Software Foundation;
* either version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
*
* You have received a copy of the GNU Lesser General Public License along with library.
* If not, see https://www.gnu.org/licenses/.
*
****************************************************************************************/
#ifndef CANCOMM_H
#define CANCOMM_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************
* Include files
****************************************************************************************/
#include <stdint.h>                         /* for standard integer types              */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Boolean true value. */
#define CANCOMM_TRUE                   (1)

/** \brief Boolean false value. */
#define CANCOMM_FALSE                  (0)


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Opaque pointer for the CAN communication context. */
typedef void * cancomm_t;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
/* API for obtaining a context, allowing multiple applications to use this library. */
cancomm_t   cancomm_new(void);
void        cancomm_free(cancomm_t ctx);
/* API for CAN communication using a specific CAN device. */
/* TODO Add flags parameter? One bit would be to enable CAN FD mode, instead of CAN
 *      classic. Might not be needed though, because FD mode should be enabled on
 *      netlink level with the MTU size. You can read this one out to determine if
 *      the CAN socket needs to be in CAN FD mode. Note that this needs to be added
 *      inside cancomm_connect(). Can look at cansend.c for an example.
 *      Wait..I'm not sure though if the MTU size is automatically set by hardware when
 *      it supports CAN FD. If so, then the user can not configure classic vs FD on a 
 *      netlink layer and the flags parameter is absolutely needed!
 *      Nope. The user needs to specify the extra data bitrate and then explicitly set
 *      the "fd on" option to switch from classic to FD. After the the MTU switches to
 *      the higher value (CANFD_MTU). So you don't need this extra parameter and you can
 *      truly rely on the MTU value from netlink to determine if FD mode needs to be
 *      enabled for the socket.
 *      Probably want to add an fd_enabled member to the context to keep track of this.
 */ 
/* TODO Maybe add fd_enabled member to the context. */
uint8_t     cancomm_connect(cancomm_t ctx, char const * device);
void        cancomm_disconnect(cancomm_t ctx);
/* TODO Add flags parameter. One bit would be for the CAN FD bit rate switch. This is
 *      an optional feature of CAN FD. For example, it is perfectly fine to send a 64-bit
 *      FD message as the arbitration speed. It will just occupy the bus longer so the
 *      real-time performance of the network gets worse. But it's up to the application
 *      to decide this.
 *      Nope. I think you can make an assumption here. When "fd on" is enabled in
 *      netlink, that can only be when the user specifically specified a bitrate of the
 *      data part. Hence, they will want to do the bit rate switch. So if fd_enabled is
 *      set in the context, then you can assume the application wants to send at the
 *      higher bit rate, so the BRS bit should be set in the can_fd_frame.flags part.
 */
/* TODO Do proper length check if len > 8. Could  use fd_enabled member to check if this
 *      is allowed at all. And if so, the length must be 12, 16, 20, 24, 32, 48 or 64.
 */
/* TODO Should switch can_frame variable to the fd version for FD compatibility. 
 */ 
uint8_t     cancomm_transmit(cancomm_t ctx, uint32_t id, uint8_t ext, uint8_t len, 
                             uint8_t const * data, uint64_t * timestamp);
/* TODO Should switch can_frame variable to the fd version for FD compatibility. 
 */ 
/* TODO Maybe add a flags parameter for future support of error frame detection. */
uint8_t     cancomm_receive(cancomm_t ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                             uint8_t * data, uint64_t * timestamp);
/* API for obtaining CAN device names on the system (can0, vcan0, etc.). */
uint8_t     cancomm_devices_buildlist(cancomm_t ctx);
char      * cancomm_devices_name(cancomm_t ctx, uint8_t idx);

#ifdef __cplusplus
}
#endif

#endif /* CANCOMM_H */
/*********************************** end of cancomm.h **********************************/
