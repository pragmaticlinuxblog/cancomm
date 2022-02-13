/************************************************************************************//**
* \file         cancomm.c
* \brief        Source file of the library for convenient access to CAN communication.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stddef.h>                         /* for NULL declaration                    */
#include "cancomm.h"                        /* CAN communication header                */


/************************************************************************************//**
** \brief     Creates a new context for the CAN communication instance. All subsequent
**            library functions need this context.
** \return    Pointer to the newly created context, if successful. NULL otherwise.
**
****************************************************************************************/
cancomm_t * cancomm_new(void)
{
  cancomm_t * result = NULL;

  /* TODO Allocate memory for the context and initialize its members. */

  /* Give the result back to the caller. */
  return result;
} /*** end of cancomm_new ***/


/************************************************************************************//**
** \brief     Releases the context. Should always be called for all CAN communication
**            instances, with function cancomm_new(), once you no longer need it.
** \param     Pointer to the CAN communication instance context.
**
****************************************************************************************/
void cancomm_free(cancomm_t * ctx)
{
  /* Verify parameter. */
  assert(ctx != NULL);

  /* Only continue with a valid parameter. */
  if (ctx != NULL)
  {
    /* TODO Release allocated memory of the context. */
  }
} /*** end of cancomm_free ***/


/*********************************** end of cancomm.c **********************************/
