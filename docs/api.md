This section provides a full reference of all the functions, macros and types that LibCanComm offers.

## Types

### can_comm_t

```c
typedef void * cancomm_t
```

Opaque pointer for the CAN communication context.

## Macros

| Macro                     | Description                                                 |
| ------------------------- | ----------------------------------------------------------- |
| `CANCOMM_TRUE`            | Boolean true value.                                         |
| `CANCOMM_FALSE`           | Boolean false value.                                        |
| `CANCOMM_FLAG_CANFD_MSG`  | Bit flag to indicate that the message is a CAN FD message.  |
| `CANCOMM_FLAG_CANERR_MSG` | Bit flag to indicate that the message is a CAN error frame. |

## Functions

### cancomm_new

```c
cancomm_t cancomm_new(void)
```

Creates a new CAN communication context. All subsequent library functions need this context. The context makes it possible for multiple applications to make use of this library.

| Return value                                          |
| ----------------------------------------------------- |
| Newly created context, if successful. NULL otherwise. |

```c linenums="1" title="Example - Create a new context:"
cancomm_t canCommCtx;

/* Create a new CAN communication context. */
canCommCtx = cancomm_new();
if (canCommCtx != NULL)
{
  printf("[INFO] Created CAN communication context.\n");
}
else
{
  printf("[ERROR] Could not create CAN communication context.\n");
}
```

### cancomm_free

```c
void cancomm_free(cancomm_t ctx)
```

Releases the context. Should be called for each CAN communication context, created with function [`cancomm_new()`](#cancomm_new), once you no longer need it.

| Parameter | Description                |
| --------- | -------------------------- |
| `ctx`     | CAN communication context. |


```c linenums="1" title="Example - Release a context:"
/* Release the CAN communication context. */
if (canCommCtx != NULL)
{
  cancomm_free(canCommCtx);
  printf("[INFO] Released CAN communication context.\n");
}
```

### cancomm_connect

```c
uint8_t cancomm_connect(cancomm_t ctx, char const * device)
```

Connects to the specified SocketCAN device. Note that you can use the functions [`cancomm_devices_buildlist()`](#cancomm_devices_buildlist) and [`cancomm_devices_name()`](#cancomm_devices_name) to determine the names of the SocketCAN devices known to the system. Alternatively, you can run command `ip addr` in the terminal to find out about the SocketCAN devices know to the system. This function automatically figures out if the SocketCAN device supports CAN FD, in addition to CAN classic.

| Parameter | Description                                                  |
| --------- | ------------------------------------------------------------ |
| `ctx`     | CAN communication context.                                   |
| `device`  | Null terminated string with the SocketCAN device name, e.g. `"can0"`. |

| Return value                                                 |
| ------------------------------------------------------------ |
| `CANCOMM_TRUE` if successfully connected to the SocketCAN device, `CANCOMM_FALSE` otherwise. |

```c linenums="1" title="Example 1 - Connect to the SocketCAN device with a specific name:"
/* Connect to SocketCAN device with name "vcan0". */
if (cancomm_connect(canCommCtx, "vcan0") == CANCOMM_TRUE)
{
  printf("[INFO] Connected to CAN device.\n");
}
```

```c linenums="1" title="Example 2 - Connect to the first SocketCAN device found on the system:"
/* Build list with all SocketCAN devices currently known to the system
 * and only continue if at least one is present.
 */
if (cancomm_devices_buildlist(canCommCtx) > 0)
{
  /* Obtain the name of the first SocketCAN device that was found. */
  char * canDevice = cancomm_devices_name(canCommCtx, 0);
  /* Connect to this SocketCAN device. */
  if (cancomm_connect(canCommCtx, canDevice) == CANCOMM_TRUE)
  {
    printf("[INFO] Connected to CAN device '%s'.\n", canDevice);
  }
}
```

### cancomm_disconnect

```c
void cancomm_disconnect(cancomm_t ctx)
```

Disconnects from the SocketCAN device.

| Parameter | Description                |
| --------- | -------------------------- |
| `ctx`     | CAN communication context. |

```c linenums="1" title="Example - Disconnect from a SocketCAN device:"
cancomm_disconnect(canCommCtx);
printf("[INFO] Disconnected from CAN device.\n");
```

### cancomm_transmit

```c
uint8_t cancomm_transmit(cancomm_t ctx, uint32_t id, uint8_t ext, uint8_t len, 
                         uint8_t const * data, uint8_t flags, uint64_t * timestamp)
```

Submits a CAN message for transmission.

| Parameter   | Description                                                  |
| ----------- | ------------------------------------------------------------ |
| `ctx`       | CAN communication context.                                   |
| `id`        | CAN message identifier.                                      |
| `ext`       | Set to `CANCOMM_FALSE` for an 11-bit message identifier, to `CANCOMM_TRUE` for 29-bit. |
| `len`       | Number of CAN message data bytes. Max 8 for a CAN classic message, max 64 for a CAN FD message. |
| `data`      | Pointer to array with data bytes.                            |
| `flags`     | Bit flags for providing additional information about how to transmit the message:<br>- `CANCOMM_FLAG_CANFD_MSG`: The message is CAN FD and not CAN classic. Ignored for non CAN FD SocketCAN devices. |
| `timestamp` | Pointer to where the timestamp (microseconds) of the message is stored. |

| Return value                                                 |
| ------------------------------------------------------------ |
| `CANCOMM_TRUE` if successfully submitted the message for transmission. `CANCOMM_FALSE` otherwise. |

```c linenums="1" title="Example 1 - Transmit a CAN classic message with 11-bit identifier:"
uint32_t canId = 0x123;
uint8_t  canExt = CANCOMM_FALSE; /* 11-bit identifier */
uint8_t  canData[] = { 0x01, 0x02, 0x55, 0xAA };
uint8_t  canFlags = 0; /* CAN classic. */
uint64_t canTimestamp = 0;
    
/* Transmit the CAN message. */
if (cancomm_transmit(canCommCtx, canId, canExt, 4, canData, canFlags, &canTimestamp) == CANCOMM_TRUE)
{
  printf("[INFO] Transmitted message at %llu us.\n", canTimestamp);    
}
```

```c linenums="1" title="Example 2 - Transmit a CAN classic message with 29-bit identifier:"
uint32_t canId = 0x123A5;
uint8_t  canExt = CANCOMM_TRUE; /* 29-bit identifier */
uint8_t  canData[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
uint8_t  canFlags = 0; /* CAN classic. */
uint64_t canTimestamp = 0;
    
/* Transmit the CAN message. */
if (cancomm_transmit(canCommCtx, canId, canExt, 8, canData, canFlags, &canTimestamp) == CANCOMM_TRUE)
{
  printf("[INFO] Transmitted message at %llu us.\n", canTimestamp);    
}
```

```c linenums="1" title="Example 3 - Transmit a CAN FD message with 11-bit identifier:"
uint32_t canId = 0x234;
uint8_t  canExt = CANCOMM_FALSE; /* 11-bit identifier */
uint8_t  canData[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
uint8_t  canFlags = CANCOMM_FLAG_CANFD_MSG; /* CAN FD. */
uint64_t canTimestamp = 0;
    
/* Transmit the CAN message. */
if (cancomm_transmit(canCommCtx, canId, canExt, 16, canData, canFlags, &canTimestamp) == CANCOMM_TRUE)
{
  printf("[INFO] Transmitted message at %llu us.\n", canTimestamp);    
}
```

### cancomm_receive

```c
uint8_t cancomm_receive(cancomm_t ctx, uint32_t * id, uint8_t * ext, uint8_t * len, 
                        uint8_t * data, uint8_t * flags, uint64_t * timestamp)
```

Reads a possibly received CAN message or CAN error frame in a non-blocking manner.

| Parameter   | Description                                                  |
| ----------- | ------------------------------------------------------------ |
| `ctx`       | CAN communication context.                                   |
| `id`        | Pointer to where the CAN message identifier is stored.       |
| `ext`       | Pointer to where the CAN identifier type is stored. `CANCOMM_FALSE` for an 11-bit message identifier, `CANCOMM_TRUE` for 29-bit. |
| `len`       | Pointer to where the number of CAN message data bytes is stored. |
| `data`      | Pointer to array where the data bytes are stored.            |
| `flags`     | Pointer to where the bit flags are stored for providing additional information about the received message:<br>-`CANCOMM_FLAG_CANFD_MSG`: The message is CAN FD and not CAN classic.<br>- `CANCOMM_FLAG_CANERR_MSG`: The message is a CAN error frame. |
| `timestamp` | Pointer to where the timestamp (microseconds) of the message is stored. |

| Return value                                                 |
| ------------------------------------------------------------ |
| `CANCOMM_TRUE` if a new message was received and copied. `CANCOMM_FALSE` otherwise. |

```c linenums="1" title="Example - Receiving CAN messages and error frames:"
uint32_t canId = 0;
uint8_t  canExt = CANCOMM_FALSE;
uint8_t  canLen = 0;
uint8_t  canData[64];
uint8_t  canFlags = 0;
uint64_t canTimestamp = 0;

/* Check for the reception of a CAN message or error frame. */
if (cancomm_receive(canCommCtx, &canId, &canExt, &canLen, &canData[0], &canFlags, 
                    &canTimestamp) == CANCOMM_TRUE)
{
  /* Was is an error frame? */
  if ((canFlags & CANCOMM_FLAG_CANERR_MSG) == CANCOMM_FLAG_CANERR_MSG)
  {
    printf("[INFO] Error frame received at %llu us.\n", canTimestamp);    
  }
  /* It was a CAN message. */
  else
  {
    /* Was it a CAN FD message? */
    if ((canFlags & CANCOMM_FLAG_CANFD_MSG) == CANCOMM_FLAG_CANFD_MSG)
    {
      printf("[INFO] Received CAN FD message with ID %Xh at %llu us.\n", 
             canId, canTimestamp);      
    }
    /* It was a CAN classic message. */
    else
    {
      printf("[INFO] Received CAN classic message with ID %Xh at %llu us.\n", 
             canId, canTimestamp);      
    }
  }
}
```

### cancomm_devices_buildlist

```c
uint8_t cancomm_devices_buildlist(cancomm_t ctx)
```

Builds a list with all the CAN device names currently present on the system. Basically an internal array with strings such as `"can0"`, `"vcan0"`, etc. Afterwards, you can call [`cancomm_devices_name()`](#cancomm_devices_name) to retrieve the name of a specific SocketCAN device, using its array index.

| Parameter | Description                |
| --------- | -------------------------- |
| `ctx`     | CAN communication context. |

| Return value                                                 |
| ------------------------------------------------------------ |
| The total number of CAN devices currently present on the system, or `0` if none were found or in case of an error. |

```c linenums="1" title="Example - Detecting all SocketCAN devices known to the system:"
uint32_t  canDeviceCnt;

canDeviceCnt = cancomm_devices_buildlist(canCommCtx);
```

### cancomm_devices_name

```c
char * cancomm_devices_name(cancomm_t ctx, uint8_t idx)
```

Obtains the SocketCAN device name at the specified index of the internal list with CAN devices, created by function [`cancomm_devices_buildlist()`](#cancomm_devices_buildlist). You could use this SocketCAN device name when calling [`cancomm_connect()`](#cancomm_connect).

Note that you should call [`cancomm_devices_buildlist()`](#cancomm_devices_buildlist) at least once, before calling this function.

| Parameter | Description                            |
| --------- | -------------------------------------- |
| `ctx`     | CAN communication context.             |
| `idx`     | Zero based index into the device list. |

| Return value                                                 |
| ------------------------------------------------------------ |
| The CAN device name at the specified index, or `NULL` in case of an error. |

```c linenums="1" title="Example - Listing all SocketCAN devices known to the system:"
uint32_t  canDeviceCnt;
uint32_t  canDeviceIdx;
char *    canDevice;

printf("[INFO] Detecting SocketCAN devices: ");
canDeviceCnt = cancomm_devices_buildlist(canCommCtx);
for (canDeviceIdx = 0; canDeviceIdx < canDeviceCnt; canDeviceIdx++)
{
  printf("'%s' ", cancomm_devices_name(canCommCtx, canDeviceIdx));
}
printf("(%d found).\n", canDeviceCnt);
```

