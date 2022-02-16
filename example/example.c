/************************************************************************************//**
* \file         example.c
* \brief        Source file of the example application.
*
*----------------------------------------------------------------------------------------
*                          C O P Y R I G H T
*----------------------------------------------------------------------------------------
*           Copyright (c) 2022 by PragmaticLinux     All rights reserved
*
*----------------------------------------------------------------------------------------
*                            L I C E N S E
*----------------------------------------------------------------------------------------
* Permission is hereby granted, free of charge, to any person obtaining a copy of this
* software and associated documentation files (the "Software"), to deal in the Software
* without restriction, including without limitation the rights to use, copy, modify, 
* merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to the following
* conditions:
*
* The above copyright notice and this permission notice shall be included in all copies
* or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <stdint.h>                         /* for standard integer types              */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdio.h>                          /* for standard I/O functions.             */
#include <stdlib.h>                         /* for standard library                    */
#include <signal.h>                         /* Signal handling                         */
#include <stdatomic.h>                      /* Atomic operations                       */
#include <cancomm.h>                        /* SocketCAN communication library         */


/****************************************************************************************
* Local constant declarations
****************************************************************************************/
/** \brief Name of the SocketCAN device. Adjust to the one you want to use. */
static const char * canDevice = "vcan0";


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Atomic boolean that is used to request a program exit. */
static atomic_bool appExitProgram;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static void AppInterruptSignalHandler(int signum);


/************************************************************************************//**
** \brief     This is the program entry point.
** \param     argc Number of program arguments.
** \param     argv Array with program arguments.
** \return    Program exit code. EXIT_SUCCESS for success, EXIT_FAILURE otherwise.
**
****************************************************************************************/
int main(int argc, char const * const argv[])
{
  cancomm_t canCommCtx;
  uint32_t  canId;
  uint8_t   canExt;
  uint8_t   canDlc;
  uint8_t   canData[CANCOMM_CAN_DATA_LEN_MAX];
  uint64_t  canTimestamp;

  /* Initialize locals. */
  atomic_init(&appExitProgram, false);

  /* Register interrupt signal handler for when CTRL+C was pressed. */
  signal(SIGINT, AppInterruptSignalHandler);

  /* Create a new CAN communication context. */
  if ((canCommCtx = cancomm_new()) == NULL)
  {
    printf("[ERROR] Could not create CAN communication context.\n");
    return EXIT_FAILURE;
  }
  printf("[INFO] Created CAN communication context.\n");


  /* Connect to the CAN device. */
  if (cancomm_connect(canCommCtx, canDevice) == CANCOMM_FALSE)
  {
    printf("[ERROR] Could not connect to CAN device %s.\n", canDevice);
    return EXIT_FAILURE;
  }
  printf("[INFO] Connected to CAN device %s.\n", canDevice);

  /* Enter the program loop until an exit (CTRL+C) is requested. */
  while (!atomic_load(&appExitProgram))
  {
    /* Check for the reception of a CAN message. */
    if (cancomm_receive(canCommCtx, &canId, &canExt, &canDlc, &canData[0], 
                        &canTimestamp) == CANCOMM_TRUE)
    {
      printf("[PING] Received CAN message with ID %Xh.\n", canId);
      /* Send the same message back but with an incremented identifier. */
      if (cancomm_transmit(canCommCtx, ++canId, canExt, canDlc, canData) == CANCOMM_TRUE)
      {
        printf("[PONG] Transmitted CAN message with ID %Xh.\n", canId);
      }
    }
  }

  /* Disconnect the CAN device. */
  cancomm_disconnect(canCommCtx);
  printf("[INFO] Disconnected from CAN device.\n");

  /* Release the CAN communication context. */
  cancomm_free(canCommCtx);
  printf("[INFO] Released CAN communication context.\n");

  /* Give the return code back to the caller. */
  return EXIT_SUCCESS;
} /*** end of main ***/


/************************************************************************************//**
** \brief     Application callback that gets called when CTRL+C was pressed to quit the
**            program.
** \param     signum Signal number (not used)
**
****************************************************************************************/
static void AppInterruptSignalHandler(int signum)
{
  /* Set request flag to exit the program when the CTRL+C key combo was pressed. */
  atomic_store(&appExitProgram, true);
} /*** end of AppInterruptSignalHandler ***/


/*********************************** end of example.c **********************************/
