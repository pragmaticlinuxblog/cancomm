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
#define CANCOMM_TRUE                   (1U)

/** \brief Boolean false value. */
#define CANCOMM_FALSE                  (0U)

/** \brief Maximum number of bytes in a CAN message. */
#define CANCOMM_CAN_DATA_LEN_MAX       (8U)


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
/* TODO Perhaps add API to enumerate list of CAN devices. */
cancomm_t * cancomm_new(void);
void        cancomm_free(cancomm_t * ctx);
uint8_t     cancomm_connect(cancomm_t * ctx, char const * device);
void        cancomm_disconnect(cancomm_t * ctx);
uint8_t     cancomm_transmit(cancomm_t * ctx, uint32_t id, uint8_t ext, uint8_t len, 
                             uint8_t const * data);
uint8_t     cancomm_receive(cancomm_t * ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                             uint8_t * data, uint32_t * timestamp);


#ifdef __cplusplus
}
#endif

#endif /* CANCOMM_H */
/*********************************** end of cancomm.h **********************************/
