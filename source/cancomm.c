/************************************************************************************//**
* \file         cancomm.c
* \brief        Source file of the library for convenient access to CAN communication.
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

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stddef.h>                         /* for NULL declaration                    */
#include <stdlib.h>                         /* for standard library                    */
#include <string.h>                         /* for string library                      */
#include <fcntl.h>                          /* File control operations                 */
#include <unistd.h>                         /* UNIX standard functions                 */
#include <net/if.h>                         /* network interfaces                      */
#include <linux/can.h>                      /* CAN kernel definitions                  */
#include <linux/can/raw.h>                  /* CAN raw sockets                         */
#include <sys/ioctl.h>                      /* I/O control operations                  */
#include "cancomm.h"                        /* SocketCAN communication library         */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Maximum number of bytes in a CAN message. */
#define CANCOMM_CAN_DATA_LEN_MAX       (8U)

/** \brief Value of an invalid socket. */
#define CANCOMM_INVALID_SOCKET         (-1)


/************************************************************************************//**
** \brief     Creates a new context for the CAN communication instance. All subsequent
**            library functions need this context.
** \return    Pointer to the newly created context, if successful. NULL otherwise.
**
****************************************************************************************/
cancomm_t * cancomm_new(void)
{
  cancomm_t * result = NULL;
  cancomm_t * newCtx;

  /* Allocate memory for the new instance. */
  newCtx = malloc(sizeof(cancomm_t));

  /* Only continue if memory could be allocated. */
  if (newCtx != NULL)
  {
    /* Initialize the instance members. */
    newCtx->socket = CANCOMM_INVALID_SOCKET;
    /* Update the result. */
    result = newCtx;
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_new ***/


/************************************************************************************//**
** \brief     Releases the context. Should always be called for all CAN communication
**            instances, created with function cancomm_new(), once you no longer need it.
** \param     ctx Pointer to the CAN communication instance context.
**
****************************************************************************************/
void cancomm_free(cancomm_t * ctx)
{
  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Make sure to disconnect the CAN device. */
    cancomm_disconnect(ctx);
    /* Release the instance's allocated memory. */
    free(ctx);
    /* Reset the pointer to prevent a dangling pointer. */
    ctx = NULL;
  }
} /*** end of cancomm_free ***/


/************************************************************************************//**
** \brief     Connects to the specified SocketCAN device. Note that you can run command
**            "ip addr" in the terminal to determine the SocketCAN device name known to
**            your Linux system.
** \param     ctx Pointer to the CAN communication instance context.
** \param     device Null terminated string with the SocketCAN device name, e.g. "can0".
** \return    CANCOMM_TRUE if successfully connected to the SocketCAN device.
**            CANCOMM_FALSE otherwise.
**
****************************************************************************************/
uint8_t cancomm_connect(cancomm_t * ctx, char const * device)
{
  uint8_t result = CANCOMM_FALSE;
  struct sockaddr_can addr;
  struct ifreq ifr;
  int32_t flags;

  /* Verify parameters. */
  assert((ctx != NULL) && (device != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (device != NULL))
  {
    /* Set positive result at this point and negate upon error detection. */
    result = CANCOMM_TRUE;

    /* Make sure we are not already connected to a CAN device. */
    cancomm_disconnect(ctx);

    /* Create an ifreq structure for passing data in and out of ioctl. */
    strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    /* Get open socket descriptor */
    if ((ctx->socket = socket(PF_CAN, (int)SOCK_RAW, CAN_RAW)) < 0)
    {
      result = CANCOMM_FALSE;
    }

    if (result == CANCOMM_TRUE)
    {
      /* Obtain interface index. */
      if (ioctl(ctx->socket, SIOCGIFINDEX, &ifr) < 0)
      {
        close(ctx->socket);
        ctx->socket = CANCOMM_INVALID_SOCKET;
        result = CANCOMM_FALSE;
      }
    }

    if (result == CANCOMM_TRUE)
    {
      /* Configure socket to work in non-blocking mode. */
      flags = fcntl(ctx->socket, F_GETFL, 0);
      if (flags == -1)
      {
        flags = 0;
      }
      if (fcntl(ctx->socket, F_SETFL, flags | O_NONBLOCK) == -1)
      {
        close(ctx->socket);
        ctx->socket = CANCOMM_INVALID_SOCKET;
        result = CANCOMM_FALSE;
      }
    }

    if (result)
    {
      /* Set the address info. */
      addr.can_family = AF_CAN;
      addr.can_ifindex = ifr.ifr_ifindex;

      /* Bind the socket. */
      if (bind(ctx->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      {
        close(ctx->socket);
        ctx->socket = CANCOMM_INVALID_SOCKET;
        result = CANCOMM_FALSE;
      }
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_connect ***/


/************************************************************************************//**
** \brief     Disconnects from the SocketCAN device.
** \param     ctx Pointer to the CAN communication instance context.
**
****************************************************************************************/
void cancomm_disconnect(cancomm_t * ctx)
{
  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Only disconnect if actually connected. */
    if (ctx->socket != CANCOMM_INVALID_SOCKET)
    {
      close(ctx->socket);
      ctx->socket = CANCOMM_INVALID_SOCKET;
    }
  }
} /*** end of cancomm_disconnect ***/


/************************************************************************************//**
** \brief     Submits a CAN message for transmission.
** \param     ctx Pointer to the CAN communication instance context.
** \param     id CAN message identifier.
** \param     ext CANCOMM_FALSE for a 11-bit message identifier, CANCOMM_TRUE of 29-bit.
** \param     len Number of CAN message data bytes.
** \param     data Pointer to array with data bytes.
** \return    CANCOMM_TRUE if successfully submitted the message for transmission.
**            CANCOMM_FALSE otherwise.
**
****************************************************************************************/
uint8_t cancomm_transmit(cancomm_t * ctx, uint32_t id, uint8_t ext, uint8_t len, 
                         uint8_t const * data)
{
  uint8_t result = CANCOMM_FALSE;

  /* Verify parameters. */
  assert((ctx != NULL) && (len <= CANCOMM_CAN_DATA_LEN_MAX) && (data != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (len <= CANCOMM_CAN_DATA_LEN_MAX) && (data != NULL))
  {
    /* TODO Implement cancomm_transmit(). */
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_transmit ***/


/************************************************************************************//**
** \brief     Retrieves a possibly received CAN message.
** \param     ctx Pointer to the CAN communication instance context.
** \param     id Pointer to where the CAN message identifier is stored.
** \param     ext Pointer to where the CAN identifer type is stored. CANCOMM_FALSE for a
**            11-bit message identifier, CANCOMM_TRUE of 29-bit.
** \param     len Pointer to where the number of CAN message data bytes is stored.
** \param     data Pointer to array where the data bytes are stored.
** \param     timestamp Pointer to where the timestamp of the message is stored.
** \return    CANCOMM_TRUE if a new message was received and copied. CANCOMM_FALSE 
**            otherwise.
**
****************************************************************************************/
uint8_t cancomm_receive(cancomm_t * ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                        uint8_t * data, uint32_t * timestamp)
{
  uint8_t result = CANCOMM_FALSE;

  /* Verify parameters. */
  assert((ctx != NULL) && (id != NULL) && (ext != NULL) && (len != NULL) && 
        (data != NULL) && (timestamp != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (id != NULL) && (ext != NULL) && (len != NULL) && 
      (data != NULL) && (timestamp != NULL))
  {
    /* TODO Implement cancomm_receive(). */
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_receive ***/


/*********************************** end of cancomm.c **********************************/
