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
#include <linux/if_arp.h>                   /* ARP definitions                         */
#include <linux/can.h>                      /* CAN kernel definitions                  */
#include <linux/sockios.h>                  /* Socket I/O                              */
#include <sys/ioctl.h>                      /* I/O control operations                  */
#include <ifaddrs.h>                        /* Listing network interfaces.             */
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
  /** \brief Holds the number of CAN devices that were detected on the system. */
  uint32_t devices_cnt;
  /** \brief Pointer to an array of strings with the names of CAN devices that were
   *         detected on the system. Memory is allocated dynamically.
   */
  char * devices_list;
};


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static uint8_t cancomm_devices_is_can(char const * name);


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
    newCtx->devices_cnt = 0;
    newCtx->devices_list = NULL;
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
    /* Release memory allocated for the devices list. */
    if (currentCtx->devices_list != NULL)
    {
      free(currentCtx->devices_list);
      currentCtx->devices_list = NULL;
      currentCtx->devices_cnt = 0;
    }

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
  assert((ctx != NULL) && (len <= CAN_MAX_DLEN) && (data != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (len <= CAN_MAX_DLEN) && (data != NULL))
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Only transmit if actually connected. */
    if (currentCtx->socket != CANCOMM_INVALID_SOCKET)
    {
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

    /* Only receive if actually connected. */
    if (currentCtx->socket != CANCOMM_INVALID_SOCKET)
    {
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
uint8_t cancomm_devices_buildlist(cancomm_t ctx)
{
  uint8_t result = 0;
  struct cancomm_ctx * currentCtx;
  struct ifaddrs *ifaddr;
  char * deviceNameEntryPtr;

  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Reset the devices count in the context. */
    currentCtx->devices_cnt = 0;

    /* Attempt to obtain access to the linked list with network interfaces. */
    if (getifaddrs(&ifaddr) == 0) 
    {
      /* Loop through the linked list, while maintaining head pointer, needed to free the
       * list later on.
       */
      for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
      {
        /* We are interested in the ifa_name element, so only process the node, when this
         * one is valid.
        */
        if (ifa->ifa_name != NULL)
        {
          /* Check if this network interface is actually a CAN interface. */
          if (cancomm_devices_is_can(ifa->ifa_name) == CANCOMM_TRUE)
          {
            /* Increment the devices count in the context. */
            currentCtx->devices_cnt++;
            /* Allocate memory in the devices list to store the device name. */
            currentCtx->devices_list = realloc(currentCtx->devices_list, 
                                               currentCtx->devices_cnt * IFNAMSIZ);
            /* Check allocation results. */
            if (currentCtx->devices_list == NULL)
            {
              /* Could not build the devices list due to an allocation issue. Reset
               * the counter and break the loop. 
               */
              currentCtx->devices_cnt = 0;
              break;
            }
            /* Still here so the allocation was successful. Now determine the address
             * inside the device list, where to store the device name. It's basically
             * the start of the newly allocated memory.
             */
            deviceNameEntryPtr = &currentCtx->devices_list[(currentCtx->devices_cnt - 1)
                                                           * IFNAMSIZ]; 
            /* Store the device name in the list. */
            strncpy(deviceNameEntryPtr, ifa->ifa_name, IFNAMSIZ);
          }
        }
      }
      /* Free the list, now that we are done with it. */
      freeifaddrs(ifaddr);
      /* Update the result. */
      result = currentCtx->devices_cnt;
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_devices_buildlist ***/


/************************************************************************************//**
** \brief     Obtains the CAN device name at the specified index of the internal array
**            with CAN devices, created by function cancomm_devices_build_list(). You
**            could use this CAN device name when calling cancomm_connect().
** \attention Call cancomm_devices_build_list() prior to calling this function.
** \param     ctx CAN communication context.
** \param     idx Zero based index inthe to device list.
** \return    The CAN device name at the specified index, or NULL in case of an error.
**
****************************************************************************************/
char * cancomm_devices_name(cancomm_t ctx, uint8_t idx)
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

    /* Only continue if the specified index is valid. */
    if (idx < currentCtx->devices_cnt)
    {
      /* Point the result to the CAN device name as the specified index. */
      result = &currentCtx->devices_list[idx];
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_devices_name ***/


/************************************************************************************//**
** \brief     Determines if the specified network interface name is a CAN device.
** \param     name Network interface name. For example obtained by getifaddrs().
** \return    CANCOMM_TRUE is the specified network interface name is a CAN device,
**            CANCOMM_FALSE otherwise.
**
****************************************************************************************/
static uint8_t cancomm_devices_is_can(char const * name)
{
  uint8_t result = CANCOMM_FALSE;
  struct ifreq ifr;
  int canSocket;

  /* Verify parameter. */
  assert(name != NULL);

  /* Only continue with valid parameter and acceptable length of the interface name. */
  if ( (name != NULL) && (strlen(name) < IFNAMSIZ) )
  {
    /* Create an ifreq structure for passing data in and out of ioctl. */
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    /* Get open socket descriptor */
    if ((canSocket = socket(PF_CAN, (int)SOCK_RAW, CAN_RAW)) != -1)
    {
      /* Obtain the hardware address information. */
      if (ioctl(canSocket, SIOCGIFHWADDR, &ifr) != -1)
      {
        /* Is this a CAN device? */
        if (ifr.ifr_hwaddr.sa_family == ARPHRD_CAN)
        {
          /* Update the result accordingly. */
          result = CANCOMM_TRUE;
        }
      }
      /* Close the socket, now that we are done with it. */
      close(canSocket);
    }
  }
 
  /* Give the result back to the caller. */
  return result;
} /*** end of AppIsCanInterface ***/


/*********************************** end of cancomm.c **********************************/
