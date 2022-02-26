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
#include <linux/can/raw.h>                  /* CAN raw definitions                     */
#include <linux/sockios.h>                  /* Socket I/O                              */
#include <sys/ioctl.h>                      /* I/O control operations                  */
#include <sys/time.h>                       /* System time utilities                   */
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
  /** \brief Boolean flag to determine if the CAN device is CAN classic or CAN FD. */
  uint8_t  fd_enabled;
  /** \brief System time at which this module connected to the CAN network. Used to
   *         calculated zero based CAN message timestamps.
   */
  uint64_t connectTime;
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
static uint8_t cancomm_sanitize_frame_len(uint8_t len);


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
  newCtx = malloc(sizeof(struct cancomm_ctx));

  /* Only continue if memory could be allocated. */
  if (newCtx != NULL)
  {
    /* Initialize the context members. */
    newCtx->socket = CANCOMM_INVALID_SOCKET;
    newCtx->fd_enabled = CANCOMM_FALSE;
    newCtx->connectTime = 0;
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
** \brief     Connects to the specified SocketCAN device. Note that you can use the
**            functions cancomm_devices_buildlist() and cancomm_devices_name() to
**            determine the names of the SocketCAN devices known to the system. 
**            Alternatively, you can run command "ip addr" in the terminal to find out
**            about the SocketCAN devices know to the system.
**            This function automatically figures out if the SocketCAN device supports
**            CAN FD, in addition to CAN classic.
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
  int32_t deviceMtu;
  struct timeval tv = { 0 };

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

    /* Get current system time. */
    if (gettimeofday(&tv, NULL) == 0)
    {
      /* Convert the current time to microseconds and store it as the connection start
       * time. Needed for zero based timestamp. 
       */
      currentCtx->connectTime = ((int64_t)tv.tv_sec * 1000 * 1000ULL) +
                               ((int64_t)tv.tv_usec);
    }
    else
    {
      result = CANCOMM_FALSE;
    }

    if (result == CANCOMM_TRUE)
    {
      /* Get open socket descriptor. */
      if ((currentCtx->socket = socket(PF_CAN, (int)SOCK_RAW, CAN_RAW)) < 0)
      {
        result = CANCOMM_FALSE;
      }
    }

    if (result == CANCOMM_TRUE)
    {
      /* Determine if the CAN device is configured for CAN classic or CAN FD mode. Do so
       * by reading the MTU size of the CAN device. For CAN classic it will be CAN_MTU.
       * For CAN FD it will be CANFD_MTU.
       */
      deviceMtu = CAN_MTU;
      /* Attempt to read the MTU value from the CAN device. */
      if (ioctl(currentCtx->socket, SIOCGIFMTU, &ifr) >= 0)
      {
        /* Only update the MTU value if it is a supported value. */
        if ( (ifr.ifr_mtu == CAN_MTU) || (ifr.ifr_mtu == CANFD_MTU) )
        {
          deviceMtu = ifr.ifr_mtu;
        }
      }
      /* Use the MTU value to determine if the CAN device is operating in CAN classic or
       * CAN FD mode. Note that the MTU value of the CAN device changes automatically to
       * the value of CANFD_MTU, after the data bitrate was configured and the fd mode
       * was turned on. Example:
       *   ip link set can0 type can bitrate 500000 dbitrate 4000000 fd on
       */
      currentCtx->fd_enabled = (deviceMtu == CANFD_MTU) ? CANCOMM_TRUE : CANCOMM_FALSE;
      
      /* Attempt to switch socket into CAN FD mode, if the CAN device is configured for
       * CAN FD.
       */
      if (currentCtx->fd_enabled == CANCOMM_TRUE)
      {
        int enable_canfd = 1;
        if (setsockopt(currentCtx->socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, 
            &enable_canfd, sizeof(enable_canfd)) != 0)
        {
          /* Could not switch the socket into CAN FD mode. Fall back to CAN classic
           * operation.
           */
          currentCtx->fd_enabled = CANCOMM_FALSE;
        }
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
** \param     ext CANCOMM_FALSE for an 11-bit message identifier, CANCOMM_TRUE for
**            29-bit.
** \param     len Number of CAN message data bytes.
** \param     data Pointer to array with data bytes.
** \param     flags Bit flags for providing additional information about how to transmit
**            the message:
**              CANCOMM_FLAG_CANFD_MSG - The message is CAN FD and not CAN classic. 
**                                       Ignored for non CAN FD SocketCAN devices.
** \param     timestamp Pointer to where the timestamp (microseconds) of the message is
**            stored.
** \return    CANCOMM_TRUE if successfully submitted the message for transmission.
**            CANCOMM_FALSE otherwise.
**
****************************************************************************************/
uint8_t cancomm_transmit(cancomm_t ctx, uint32_t id, uint8_t ext, uint8_t len, 
                         uint8_t const * data, uint8_t flags, uint64_t * timestamp)
{
  uint8_t result = CANCOMM_FALSE;
  struct cancomm_ctx * currentCtx;
  struct canfd_frame canTxFrame = { 0 };
  struct timeval tv = { 0 };
  uint8_t frameLenMax;
  size_t frameSizeMax;

  /* Verify parameters. */
  assert((ctx != NULL) && (len <= CANFD_MAX_DLEN) && (data != NULL));

  /* Only continue with a valid parameters. */
  if ((ctx != NULL) && (len <= CANFD_MAX_DLEN) && (data != NULL))
  {
    /* Cast the opaque pointer to its non-opaque counter part. */
    currentCtx = (struct cancomm_ctx *)ctx;

    /* Only transmit if actually connected. */
    if (currentCtx->socket != CANCOMM_INVALID_SOCKET)
    {
      /* Initialize the settings as if the message will be CAN classic. */
      frameLenMax = CAN_MAX_DLEN;
      frameSizeMax = CAN_MTU;
      /* Should the message be transmitted as CAN FD? */
      if ((currentCtx->fd_enabled) && (flags & CANCOMM_FLAG_CANFD_MSG))
      {
        /* Update the settings for the mesasge to be CAN FD. */
        frameLenMax = CANFD_MAX_DLEN;
        frameSizeMax = CANFD_MTU;
        /* Configure the bit rate switch when transmitting messages in CAN FD mode. */
        canTxFrame.flags |= CANFD_BRS;        
      }

      /* Only transmit if all the data actually fits. */
      if (len <= frameLenMax)
      {
        /* Construct the transmit frame. */
        canTxFrame.can_id = id;
        if (ext == CANCOMM_TRUE)
        {
          canTxFrame.can_id |= CAN_EFF_FLAG;
        }
        /* Sanitize the frame length before storing it. */
        canTxFrame.len = cancomm_sanitize_frame_len(len);
        for (uint8_t idx = 0; idx < len; idx++)
        {
          canTxFrame.data[idx] = data[idx];
        }

        /* Request transmission of the frame. */
        if (write(currentCtx->socket, &canTxFrame, frameSizeMax) == frameSizeMax)
        {
          /* Get the timestamp of the transmit event. */
          *timestamp = 0;
          if (gettimeofday(&tv, NULL) == 0)
          {
            /* Convert the timestamp to microseconds. */
            *timestamp = ((int64_t)tv.tv_sec * 1000 * 1000ULL) + ((int64_t)tv.tv_usec);
            /* Make the timestamp relative to the connection time. */
            *timestamp -= currentCtx->connectTime;
          }
          /* Successfully submitted for transmission. Update the result accordingly. */
          result = CANCOMM_TRUE;
        }
      }
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_transmit ***/


/************************************************************************************//**
** \brief     Reads a possibly received CAN message or CAN eror frame in a non-blocking
**            manner.
** \param     ctx CAN communication context.
** \param     id Pointer to where the CAN message identifier is stored.
** \param     ext Pointer to where the CAN identifier type is stored. CANCOMM_FALSE for
**            an 11-bit message identifier, CANCOMM_TRUE for 29-bit.
** \param     len Pointer to where the number of CAN message data bytes is stored.
** \param     data Pointer to array where the data bytes are stored.
** \param     flags Pointer to where the bit flags are stored for providing additional
**            information about the received message:
**              CANCOMM_FLAG_CANFD_MSG - The message is CAN FD and not CAN classic.
**              CANCOMM_FLAG_CANERR_MSG - The message is a CAN error frame.
** \param     timestamp Pointer to where the timestamp (microseconds) of the message is
**            stored.
** \return    CANCOMM_TRUE if a new message was received and copied. CANCOMM_FALSE 
**            otherwise.
**
****************************************************************************************/
uint8_t cancomm_receive(cancomm_t ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                        uint8_t * data, uint8_t * flags, uint64_t * timestamp)
{
  uint8_t result = CANCOMM_FALSE;
  struct cancomm_ctx * currentCtx;
  struct canfd_frame canRxFrame = { 0 };
  struct timeval tv = { 0 };
  size_t frameSize;

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
      /* Attempt to read the next frame from the queue. */
      frameSize = read(currentCtx->socket, &canRxFrame, CANFD_MTU);
      /* CAN FD or CAN classic frames are the only valid ones. */
      if ( (frameSize == CANFD_MTU) || (frameSize == CAN_MTU) )
      {
        /* Ignore remote frames. Pretty much no one actually uses these. */
        if (!(canRxFrame.can_id & CAN_RTR_FLAG))
        {
          /* Reset the bit flags. */
          *flags = 0;
          /* Obtain the timestamp of the reception event. */
          *timestamp = 0;
          if (ioctl(currentCtx->socket, SIOCGSTAMP, &tv) == 0)
          {
            /* Convert the timestamp to microseconds. */
            *timestamp = ((int64_t)tv.tv_sec * 1000 * 1000ULL) + ((int64_t)tv.tv_usec);
            /* Make the timestamp relative to the connection time. */
            *timestamp -= currentCtx->connectTime;
          }

          /* Was it an error frame? */
          if (canRxFrame.can_id & CAN_ERR_FLAG)
          {
            /* Store error frame info. */
            *flags |= CANCOMM_FLAG_CANERR_MSG;
            *id = 0;
            *ext = CANCOMM_FALSE;
            *len = 0;
          }
          /* It was a regular data frame. Either CAN FD or CAN classic. */
          else
          {
            /* Was it a CAN FD frame? */
            if (frameSize == CANFD_MTU)
            {
              /* Flag the frame as a CAN FD frame for the caller. */
              *flags |= CANCOMM_FLAG_CANFD_MSG;
            }

            /* Copy the CAN data frame. */
            if (canRxFrame.can_id & CAN_EFF_FLAG)
            {
              *ext = CANCOMM_TRUE;
            }
            else
            {
              *ext = CANCOMM_FALSE;
            }
            *id = canRxFrame.can_id & ~CAN_EFF_FLAG;
            *len = canRxFrame.len;
            for (uint8_t idx = 0; idx < canRxFrame.len; idx++)
            {
              data[idx] = canRxFrame.data[idx];
            }
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
**            Afterwards, you can call cancomm_devices_name() to retrieve the name of a
**            specific SocketCAN device, using its array index.
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
** \brief     Obtains the CAN device name at the specified index of the internal list
**            with CAN devices, created by function cancomm_devices_buildlist(). You
**            could use this CAN device name when calling cancomm_connect().
** \attention Call cancomm_devices_buildlist() prior to calling this function.
** \param     ctx CAN communication context.
** \param     idx Zero based index into the device list.
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
      result = &currentCtx->devices_list[idx * IFNAMSIZ];
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
} /*** end of cancomm_devices_is_can ***/


/************************************************************************************//**
** \brief     Helper function to sanitize the CAN frame length, specifically for CAN FD.
**            On CAN FD, the frame lengths can be: 0..8, 12, 16, 20, 24, 32, 48, 64.
**            This means that if a frame length of 14 is specified, it should be rounded
**            up to the next supported frame length value, 16 in this case.
** \param     len Unsanitized frame length. 0..64.
** \return    Sanitized frame length in the range 0..8, 12, 16, 20, 24, 32, 48, 64.
**
****************************************************************************************/
static uint8_t cancomm_sanitize_frame_len(uint8_t len)
{
  uint8_t result;
  uint8_t frame_len;
  uint8_t frame_dlc;
  static const uint8_t len2dlc[] = 
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,    /*  0 -  8 */
     9,  9,  9,  9,                        /*  9 - 12 */
    10, 10, 10, 10,                        /* 13 - 16 */
    11, 11, 11, 11,                        /* 17 - 20 */
    12, 12, 12, 12,                        /* 21 - 24 */
    13, 13, 13, 13, 13, 13, 13, 13,        /* 25 - 32 */
    14, 14, 14, 14, 14, 14, 14, 14,        /* 33 - 40 */
    14, 14, 14, 14, 14, 14, 14, 14,        /* 41 - 48 */
    15, 15, 15, 15, 15, 15, 15, 15,        /* 49 - 56 */
    15, 15, 15, 15, 15, 15, 15, 15         /* 57 - 64 */
  };
  static const uint8_t dlc2len[] = 
  {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64
  };                                    

  /* Make sure the specified len parameter is valid. If not, correct it. */
  frame_len = (len > CANFD_MAX_DLEN) ? CANFD_MAX_DLEN : len;
  /* Convert the lenght value to the CAN FD dlc value (0..15). */
  frame_dlc = len2dlc[frame_len];
  /* Convert the CAN FD dlc value to its representive frame length value. */
  result = dlc2len[frame_dlc];

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_sanitize_frame_len ***/


/*********************************** end of cancomm.c **********************************/
