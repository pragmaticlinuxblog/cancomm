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
#include <linux/sockios.h>                  /* Socket I/O                              */
#include <sys/ioctl.h>                      /* I/O control operations                  */
#include "cancomm.h"                        /* SocketCAN communication library         */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Value of an invalid socket. */
#define CANCOMM_INVALID_SOCKET         (-1)


/****************************************************************************************
* Structure definitions
****************************************************************************************/
/** \brief Structure for grouping all CAN communication context related data. Basically
 *         the non-opaque counter part of cancomm_t.
 */
struct cancomm_ctx
{
  /** \brief CAN raw socket handle. Also used to determine the connection state
   *         internally. CANCOMM_INVALID_SOCKET if not connected, any other value if
   *         connected.
   */
  uint32_t socket;
};


/************************************************************************************//**
** \brief     Creates a new CAN communication context. All subsequent library functions
**            need this context.
** \return    Newly created context, if successful. NULL otherwise.
**
****************************************************************************************/
cancomm_t cancomm_new(void)
{
  cancomm_t result = NULL;
  struct cancomm_ctx * newCtx;

  /* Allocate memory for the new context. */
  newCtx = malloc(sizeof(cancomm_t));

  /* Only continue if memory could be allocated. */
  if (newCtx != NULL)
  {
    /* Initialize the context members. */
    newCtx->socket = CANCOMM_INVALID_SOCKET;
    /* Update the result. */
    result = (cancomm_t)newCtx;
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_new ***/


/************************************************************************************//**
** \brief     Releases the context. Should be called for each CAN communication
**            context, created with function cancomm_new(), once you no longer need it.
** \param     ctx CAN communication context.
**
****************************************************************************************/
void cancomm_free(cancomm_t ctx)
{
  struct cancomm_ctx * currentCtx;

  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Make sure to disconnect the CAN device. */
    cancomm_disconnect(currentCtx);
    /* Release the context's allocated memory. */
    free(currentCtx);
    /* Reset the pointer to prevent a dangling pointer. */
    currentCtx = NULL;
  }
} /*** end of cancomm_free ***/


/************************************************************************************//**
** \brief     Connects to the specified SocketCAN device. Note that you can run command
**            "ip addr" in the terminal to determine the SocketCAN device name known to
**            your Linux system.
** \param     ctx CAN communication context.
** \param     device Null terminated string with the SocketCAN device name, e.g. "can0".
** \return    CANCOMM_TRUE if successfully connected to the SocketCAN device.
**            CANCOMM_FALSE otherwise.
**
****************************************************************************************/
uint8_t cancomm_connect(cancomm_t ctx, char const * device)
{
  uint8_t result = CANCOMM_FALSE;
  struct cancomm_ctx * currentCtx;
  struct sockaddr_can addr;
  struct ifreq ifr;
  int32_t flags;

  /* Verify parameters. */
  assert((ctx != NULL) && (device != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (device != NULL))
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Set positive result at this point and negate upon error detection. */
    result = CANCOMM_TRUE;

    /* Make sure we are not already connected to a CAN device. */
    cancomm_disconnect(currentCtx);

    /* Create an ifreq structure for passing data in and out of ioctl. */
    strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    /* Get open socket descriptor */
    if ((currentCtx->socket = socket(PF_CAN, (int)SOCK_RAW, CAN_RAW)) < 0)
    {
      result = CANCOMM_FALSE;
    }

    if (result == CANCOMM_TRUE)
    {
      /* Obtain interface index. */
      if (ioctl(currentCtx->socket, SIOCGIFINDEX, &ifr) < 0)
      {
        close(currentCtx->socket);
        currentCtx->socket = CANCOMM_INVALID_SOCKET;
        result = CANCOMM_FALSE;
      }
    }

    if (result == CANCOMM_TRUE)
    {
      /* Configure socket to work in non-blocking mode. */
      flags = fcntl(currentCtx->socket, F_GETFL, 0);
      if (flags == -1)
      {
        flags = 0;
      }
      if (fcntl(currentCtx->socket, F_SETFL, flags | O_NONBLOCK) == -1)
      {
        close(currentCtx->socket);
        currentCtx->socket = CANCOMM_INVALID_SOCKET;
        result = CANCOMM_FALSE;
      }
    }

    if (result)
    {
      /* Set the address info. */
      addr.can_family = AF_CAN;
      addr.can_ifindex = ifr.ifr_ifindex;

      /* Bind the socket. */
      if (bind(currentCtx->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      {
        close(currentCtx->socket);
        currentCtx->socket = CANCOMM_INVALID_SOCKET;
        result = CANCOMM_FALSE;
      }
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_connect ***/


/************************************************************************************//**
** \brief     Disconnects from the SocketCAN device.
** \param     ctx CAN communication context.
**
****************************************************************************************/
void cancomm_disconnect(cancomm_t ctx)
{
  struct cancomm_ctx * currentCtx;

  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Only disconnect if actually connected. */
    if (currentCtx->socket != CANCOMM_INVALID_SOCKET)
    {
      close(currentCtx->socket);
      currentCtx->socket = CANCOMM_INVALID_SOCKET;
    }
  }
} /*** end of cancomm_disconnect ***/


/************************************************************************************//**
** \brief     Submits a CAN message for transmission.
** \param     ctx CAN communication context.
** \param     id CAN message identifier.
** \param     ext CANCOMM_FALSE for a 11-bit message identifier, CANCOMM_TRUE of 29-bit.
** \param     len Number of CAN message data bytes.
** \param     data Pointer to array with data bytes.
** \return    CANCOMM_TRUE if successfully submitted the message for transmission.
**            CANCOMM_FALSE otherwise.
**
****************************************************************************************/
uint8_t cancomm_transmit(cancomm_t ctx, uint32_t id, uint8_t ext, uint8_t len, 
                         uint8_t const * data)
{
  uint8_t result = CANCOMM_FALSE;
  struct cancomm_ctx * currentCtx;
  struct can_frame canTxFrame = { 0 };

  /* Verify parameters. */
  assert((ctx != NULL) && (len <= CANCOMM_CAN_DATA_LEN_MAX) && (data != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (len <= CANCOMM_CAN_DATA_LEN_MAX) && (data != NULL))
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Construct the transmit frame. */
    canTxFrame.can_id = id;
    if (ext == CANCOMM_TRUE)
    {
      canTxFrame.can_id |= CAN_EFF_FLAG;
    }
    canTxFrame.can_dlc = len;
    for (uint8_t idx = 0; idx < len; idx++)
    {
      canTxFrame.data[idx] = data[idx];
    }

    /* Request transmission of the frame. */
    if (write(currentCtx->socket, &canTxFrame, sizeof(struct can_frame)) == 
        (ssize_t)sizeof(struct can_frame))
    {
      /* Successfully submitted for transmission. Update the result accordingly. */
      result = CANCOMM_TRUE;
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_transmit ***/


/************************************************************************************//**
** \brief     Retrieves a possibly received CAN message.
** \param     ctx CAN communication context.
** \param     id Pointer to where the CAN message identifier is stored.
** \param     ext Pointer to where the CAN identifer type is stored. CANCOMM_FALSE for a
**            11-bit message identifier, CANCOMM_TRUE of 29-bit.
** \param     len Pointer to where the number of CAN message data bytes is stored.
** \param     data Pointer to array where the data bytes are stored.
** \param     timestamp Pointer to where the timestamp (microseconds) of the message is
**            stored.
** \return    CANCOMM_TRUE if a new message was received and copied. CANCOMM_FALSE 
**            otherwise.
**
****************************************************************************************/
uint8_t cancomm_receive(cancomm_t ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                        uint8_t * data, uint64_t * timestamp)
{
  uint8_t result = CANCOMM_FALSE;
  struct cancomm_ctx * currentCtx;
  struct can_frame canRxFrame = { 0 };
  struct timeval tv = { 0 };

  /* Verify parameters. */
  assert((ctx != NULL) && (id != NULL) && (ext != NULL) && (len != NULL) && 
        (data != NULL) && (timestamp != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (id != NULL) && (ext != NULL) && (len != NULL) && 
      (data != NULL) && (timestamp != NULL))
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Attempt to get the next CAN event from the queue. */
    if (read(currentCtx->socket, &canRxFrame, sizeof(struct can_frame)) == 
        (ssize_t)sizeof(struct can_frame))
    {
      /* Ignore remote frames and error information. */
      if (!(canRxFrame.can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG)))
      {
        /* Obtain the timestamp of the reception event. */
        *timestamp = 0;
        if (ioctl(currentCtx->socket, SIOCGSTAMP, &tv) == 0)
        {
          /* Convert the timestamp to microseconds. */
          *timestamp = ((int64_t)tv.tv_sec * 1000 * 1000ULL) + ((int64_t)tv.tv_usec);
        }

        /* Copy the CAN frame. */
        if (canRxFrame.can_id & CAN_EFF_FLAG)
        {
          *ext = CANCOMM_TRUE;
        }
        else
        {
          *ext = CANCOMM_FALSE;
        }
        *id = canRxFrame.can_id & ~CAN_EFF_FLAG;
        *len = canRxFrame.can_dlc;
        for (uint8_t idx = 0; idx < canRxFrame.can_dlc; idx++)
        {
          data[idx] = canRxFrame.data[idx];
        }
        /* Frame successfully read. Update the result accordingly. */
        result = CANCOMM_TRUE;
      }        
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_receive ***/


/************************************************************************************//**
** \brief     Builds a list with all the CAN device names currently present on the
**            system. Basically an internal array with strings such as can0, vcan0, etc.
**            Afterwards, you can call the cancomm_devices_get_xxx functions to retrieve
**            information about a specific device, using its array index.
** \param     ctx CAN communication context.
** \return    The total number of CAN devices currently present on the system, or 0 if
**            none were found or in case of an error.
**
****************************************************************************************/
uint8_t cancomm_devices_build_list(cancomm_t ctx)
{
  uint8_t result;
  struct cancomm_ctx * currentCtx;

  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* TODO Implement cancomm_devices_build_list(). */
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_devices_build_list ***/


/************************************************************************************//**
** \brief     Obtains the CAN device name at the specified index of the internal array
**            with CAN devices, created by function cancomm_devices_build_list().
** \attention Call cancomm_devices_build_list() prior to calling this function.
** \param     ctx CAN communication context.
** \param     idx Zero based index inthe to device list.
** \return    The CAN device name at the specified index, or NULL in case of an error.
**
****************************************************************************************/
char * cancomm_devices_get_name(cancomm_t ctx, uint8_t idx)
{
  char * result = NULL;
  struct cancomm_ctx * currentCtx;

  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* TODO Implement cancomm_devices_get_name(). */
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_devices_get_name ***/


/************************************************************************************//**
** \brief     Obtains the CAN device baudrate at the specified index of the internal
**            array with CAN devices, created by function cancomm_devices_build_list().
** \attention Call cancomm_devices_build_list() prior to calling this function.
** \param     ctx CAN communication context.
** \param     idx Zero based index inthe to device list.
** \return    The CAN device name at the specified index, or NULL in case of an error.
**
****************************************************************************************/
uint32_t cancomm_devices_get_baudrate(cancomm_t ctx, uint8_t idx)
{
  uint32_t result = 0;
  struct cancomm_ctx * currentCtx;

  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* TODO Implement cancomm_devices_get_baudrate(). */
  }

  /* Give the result back to the caller. */
  return result;
} /*** end ofcancomm_devices_get_baudrate ***/


/*********************************** end of cancomm.c **********************************/
