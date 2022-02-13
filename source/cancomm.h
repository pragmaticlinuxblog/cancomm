/************************************************************************************//**
* \file         cancomm.h
* \brief        Header file of the library for convenient access to CAN communication.
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
#define CANCOMM_TRUE         (1U)

/** \brief Boolean false value. */
#define CANCOMM_FALSE        (0U)


/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Structure for grouping all CAN communication context related data. */
typedef struct
{
  char    * device;
  uint8_t   connected;
} cancomm_t;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
cancomm_t * cancomm_new(void);
void        cancomm_free(cancomm_t * ctx);


#ifdef __cplusplus
}
#endif

#endif /* CANCOMM_H */
/*********************************** end of cancomm.h **********************************/
