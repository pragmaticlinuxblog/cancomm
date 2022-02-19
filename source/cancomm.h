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

/** \brief Bit flag to indicate that the message is a CAN FD message. */
#define CANCOMM_FLAG_CANFD_MSG         (0x01)

/** \brief Bit flag to indicate that the message is a CAN error frame. */
#define CANCOMM_FLAG_CANERR_MSG        (0x80)


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
uint8_t     cancomm_connect(cancomm_t ctx, char const * device);
void        cancomm_disconnect(cancomm_t ctx);
uint8_t     cancomm_transmit(cancomm_t ctx, uint32_t id, uint8_t ext, uint8_t len, 
                             uint8_t const * data, uint8_t flags, uint64_t * timestamp);
uint8_t     cancomm_receive(cancomm_t ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                             uint8_t * data, uint8_t * flags, uint64_t * timestamp);
/* API for obtaining CAN device names on the system (can0, vcan0, etc.). */
uint8_t     cancomm_devices_buildlist(cancomm_t ctx);
char      * cancomm_devices_name(cancomm_t ctx, uint8_t idx);

#ifdef __cplusplus
}
#endif

#endif /* CANCOMM_H */
/*********************************** end of cancomm.h **********************************/
