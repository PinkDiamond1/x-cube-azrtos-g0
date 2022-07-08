/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_usbx_host.c
  * @author  MCD Application Team
  * @brief   USBX host applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_usbx_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32g0c1e_eval_pwr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_QUEUE_SIZE                               1
#define USBX_APP_STACK_SIZE                          1024
#define USBX_MEMORY_SIZE                             (64 * 1024)
#define APP_STACK_SIZE                               512
#define VBUS_SAFETY_HIGH        5500U /* VBUS should not exceed 5V +10% */
#define VBUS_SAFETY_LOW         4650U /* VBUS should not lower than 5V -7% */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern HCD_HandleTypeDef                 hhcd_USB_DRD_FS;
TX_THREAD                                ux_app_thread;
TX_THREAD                                keyboard_app_thread;
TX_THREAD                                mouse_app_thread;
TX_THREAD                                ucpd_app_thread;
TX_THREAD                                 safety_app_thread;
TX_QUEUE                                 ux_app_MsgQueue;
TX_QUEUE                                 ux_app_MsgQueue_UCPD;
UX_HOST_CLASS_HID                        *hid;
UX_HOST_CLASS_HID_CLIENT                 *hid_client;
UX_HOST_CLASS_HID_MOUSE                  *mouse;
UX_HOST_CLASS_HID_KEYBOARD               *keyboard;
#if defined ( __ICCARM__ ) /* IAR Compiler */
  #pragma data_alignment=4
#endif /* defined ( __ICCARM__ ) */
__ALIGN_BEGIN USB_MODE_STATE        USB_Host_State_Msg __ALIGN_END;
#if defined ( __ICCARM__ ) /* IAR Compiler */
  #pragma data_alignment=4
#endif /* defined ( __ICCARM__ ) */
__ALIGN_BEGIN ux_app_devInfotypeDef       ux_dev_info  __ALIGN_END;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
extern void MX_USB_DRD_FS_HCD_Init(void);
extern void Error_Handler(void);
/* USER CODE END PFP */
/**
  * @brief  Application USBX Host Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_USBX_Host_Init(VOID *memory_ptr)
{
  UINT ret = UX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN MX_USBX_Host_MEM_POOL */
  /* USER CODE END MX_USBX_Host_MEM_POOL */

  /* USER CODE BEGIN MX_USBX_Host_Init */
  CHAR *pointer;

  /* Allocate the stack for thread 0. */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Initialize USBX memory. */
  if (ux_system_initialize(pointer, USBX_MEMORY_SIZE, UX_NULL, 0) != UX_SUCCESS)

  {
    return UX_ERROR;
  }

  /* register a callback error function */
  _ux_utility_error_callback_register(&ux_host_error_callback);

  /* Allocate the stack for thread 0. */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main App thread. */
  if (tx_thread_create(&ux_app_thread, "thread 0", usbx_app_thread_entry, 0,
                       pointer, USBX_APP_STACK_SIZE, 25, 25, 0,
                       TX_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Allocate the stack for the ucpd app thread. */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the ucpd app thread.  */
  if (tx_thread_create(&ucpd_app_thread, "ucpd_app_thread", ucpd_app_thread_entry, 0,
                       pointer, USBX_APP_STACK_SIZE, 20, 20, 0,
                       TX_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Allocate the stack for thread 1. */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the HID mouse App thread. */
  if (tx_thread_create(&mouse_app_thread, "thread 1", hid_mouse_thread_entry, 0,
                       pointer, USBX_APP_STACK_SIZE, 30, 30, 1,
                       TX_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Allocate the stack for thread 2. */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       USBX_APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the HID Keyboard App thread. */
  if (tx_thread_create(&keyboard_app_thread, "keyboard_app_thread", hid_keyboard_thread_entry, 0,
                       pointer, USBX_APP_STACK_SIZE, 30, 30, 0,
                       TX_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

   /* Allocate Memory for the SAFETY PROTECTION thread  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       APP_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the SAFETY PROTECTION thread.  */
  if (tx_thread_create(&safety_app_thread, "safety_app_thread", safety_protection_thread_entry, 0,
                       pointer, APP_STACK_SIZE, 30, 30, 0,
                       TX_AUTO_START) != TX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }

  /* Allocate Memory for the Queue */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       APP_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the MsgQueue */
  if (tx_queue_create(&ux_app_MsgQueue, "Message Queue app", TX_1_ULONG,
                      pointer, APP_QUEUE_SIZE * sizeof(ULONG)) != TX_SUCCESS)
  {
    return TX_QUEUE_ERROR;
  }

/* Allocate Memory for the ux_app_Queue_UCPD  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,
                       APP_QUEUE_SIZE * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the MsgQueue for ucpd_app_thread   */
  if (tx_queue_create(&ux_app_MsgQueue_UCPD, "Message Queue UCPD", TX_1_ULONG,
                      pointer, APP_QUEUE_SIZE * sizeof(ULONG)) != TX_SUCCESS)
  {
    return TX_QUEUE_ERROR;
  }
  /* USER CODE END MX_USBX_Host_Init */

  return ret;
}

/* USER CODE BEGIN 1 */

/**
  * @brief  UCPD Application_thread_entry .
  * @param  ULONG arg
  * @retval Void
  */
void  ucpd_app_thread_entry(ULONG arg)
{

  while (1)
  {
    /* wait for message queue from callback event */
    if(tx_queue_receive(&ux_app_MsgQueue_UCPD, &USB_Host_State_Msg, TX_WAIT_FOREVER)!= TX_SUCCESS)
    {
     Error_Handler();
    }
    /* Check if received message equal to START_USB_HOST */
    if (USB_Host_State_Msg == START_USB_HOST)
    {
    /* Enable USB Global Interrupt*/
    HAL_HCD_Start(&hhcd_USB_DRD_FS);

    tx_thread_sleep(MS_TO_TICK(10));

    /* Enable power supply over VBUS */
    BSP_PWR_VBUSOn(TYPE_C_PORT_1);
    }
    /* Check if received message equal to STOP_USB_HOST */
    else if (USB_Host_State_Msg == STOP_USB_HOST)
    {
      /* Stop USB Host */
      HAL_HCD_Stop(&hhcd_USB_DRD_FS);
    }
    /* Else Error */
    else
    {
      /*Error*/
      Error_Handler();
    }
   tx_thread_sleep(MS_TO_TICK(10));
  }
}
/**
  * @brief  Application_thread_entry .
  * @param  ULONG arg
  * @retval Void
  */
void  usbx_app_thread_entry(ULONG arg)
{
  /* Initialize USBX_Host */
  MX_USB_Host_Init();

  /* Start Application Message */
  USBH_UsrLog(" **** USB OTG FS HID Host **** \n");
  USBH_UsrLog("USB Host library started.\n");

  /* Wait for Device to be attached */
  USBH_UsrLog("Starting HID Application");
  USBH_UsrLog("Connect your HID Device\n");

  while (1)
  {
    /* Wait for a hid device to be connected */
    if (tx_queue_receive(&ux_app_MsgQueue, &ux_dev_info, TX_WAIT_FOREVER)!= TX_SUCCESS)
    {
     /*Error*/
     Error_Handler();
    }

    if (ux_dev_info.Dev_state == Device_connected)
    {
      switch (ux_dev_info.Device_Type)
      {
        case Mouse_Device :
          mouse = hid_client-> ux_host_class_hid_client_local_instance;
          USBH_UsrLog("HID_Mouse_Device");
          USBH_UsrLog("PID: %#x ", (UINT)mouse ->ux_host_class_hid_mouse_hid->ux_host_class_hid_device->ux_device_descriptor.idProduct);
          USBH_UsrLog("VID: %#x ", (UINT)mouse ->ux_host_class_hid_mouse_hid->ux_host_class_hid_device->ux_device_descriptor.idVendor);
          USBH_UsrLog("USB HID Host Mouse App...");
          USBH_UsrLog("Mouse is ready...\n");
          break;

        case Keyboard_Device :
          keyboard = hid_client-> ux_host_class_hid_client_local_instance;
          USBH_UsrLog("HID_Keyboard_Device");
          USBH_UsrLog("PID: %#x ", (UINT)keyboard ->ux_host_class_hid_keyboard_hid->ux_host_class_hid_device->ux_device_descriptor.idProduct);
          USBH_UsrLog("VID: %#x ", (UINT)keyboard ->ux_host_class_hid_keyboard_hid->ux_host_class_hid_device->ux_device_descriptor.idVendor);
          USBH_UsrLog("USB HID Host Keyboard App...");
          USBH_UsrLog("keyboard is ready...\n");
          break;

        case Unknown_Device :
          USBH_ErrLog("Unsupported USB device");
          break;

        default :
          break;
      }
    }
    else
    {
      /* clear hid_client local instance */
      mouse = NULL;
      keyboard = NULL;
    }
  }
}

/**
* @brief ux_host_event_callback
* @param ULONG event
           This parameter can be one of the these values:
             1 : UX_DEVICE_INSERTION
             2 : UX_DEVICE_REMOVAL
             3 : UX_HID_CLIENT_INSERTION
             4 : UX_HID_CLIENT_REMOVAL
         UX_HOST_CLASS * Current_class
         VOID * Current_instance
* @retval Status
*/
UINT ux_host_event_callback(ULONG event, UX_HOST_CLASS *Current_class, VOID *Current_instance)
{
  UINT status;
  UX_HOST_CLASS *hid_class;

  switch (event)
  {
    case UX_DEVICE_INSERTION :
      /* Get current Hid Class */
      status = ux_host_stack_class_get(_ux_system_host_class_hid_name, &hid_class);

      if (status == UX_SUCCESS)
      {
        if ((hid_class == Current_class) && (hid == NULL))
        {
          /* Get current Hid Instance */
          hid = Current_instance;
          /* Get the HID Client */
          hid_client = hid ->ux_host_class_hid_client;

          if (hid->ux_host_class_hid_client->ux_host_class_hid_client_status != (ULONG) UX_HOST_CLASS_INSTANCE_LIVE)
          {
            ux_dev_info.Device_Type = Unknown_Device;
          }
          /* Check the HID_client if this is a HID mouse device. */
          if (ux_utility_memory_compare(hid_client -> ux_host_class_hid_client_name,
                                        _ux_system_host_class_hid_client_mouse_name,
                                        ux_utility_string_length_get(_ux_system_host_class_hid_client_mouse_name)) == UX_SUCCESS)
          {
            /* update HID device Type */
            ux_dev_info.Device_Type = Mouse_Device;

            /* put a message queue to usbx_app_thread_entry */
            tx_queue_send(&ux_app_MsgQueue, &ux_dev_info, TX_NO_WAIT);
          }

          /* Check the HID_client if this is a HID keyboard device. */
          else if (ux_utility_memory_compare(hid_client -> ux_host_class_hid_client_name,
                                             _ux_system_host_class_hid_client_keyboard_name,
                                             ux_utility_string_length_get(_ux_system_host_class_hid_client_keyboard_name)) == UX_SUCCESS)
          {
            /* update HID device Type */
            ux_dev_info.Device_Type = Keyboard_Device;

            /* put a message queue to usbx_app_thread_entry */
            tx_queue_send(&ux_app_MsgQueue, &ux_dev_info, TX_NO_WAIT);
          }
          else
          {
            ux_dev_info.Device_Type = Unknown_Device;
            ux_dev_info.Dev_state = Device_connected;
            tx_queue_send(&ux_app_MsgQueue, &ux_dev_info, TX_NO_WAIT);
          }
        }
      }
      else
      {
        /* No HID class found */
        USBH_ErrLog("NO HID Class found");
      }
      break;

    case UX_DEVICE_REMOVAL :

      if (Current_instance == hid)
      {
        /* Free Instance */
        hid = NULL;
        USBH_UsrLog("USB Device Unplugged");
        ux_dev_info.Dev_state   = No_Device;
        ux_dev_info.Device_Type = Unknown_Device;
      }
      break;

    case UX_HID_CLIENT_INSERTION :
      USBH_UsrLog("HID Client Plugged");
      ux_dev_info.Dev_state = Device_connected;
      break;

    case UX_HID_CLIENT_REMOVAL:
      USBH_UsrLog("HID Client Unplugged");
      ux_dev_info.Dev_state   =  Device_disconnected;
      ux_dev_info.Device_Type =  Unknown_Device;
      tx_queue_send(&ux_app_MsgQueue, &ux_dev_info, TX_NO_WAIT);

      break;

    default:
      break;

  }

  return (UINT) UX_SUCCESS;
}

/**
* @brief ux_host_error_callback
* @param ULONG event
         UINT system_context
         UINT error_code
* @retval Status
*/
VOID ux_host_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
  switch (error_code)
  {
    case UX_DEVICE_ENUMERATION_FAILURE :

      ux_dev_info.Device_Type = Unknown_Device;
      ux_dev_info.Dev_state   = Device_connected;
      tx_queue_send(&ux_app_MsgQueue, &ux_dev_info, TX_NO_WAIT);
      break;

    case  UX_NO_DEVICE_CONNECTED :
      USBH_UsrLog("USB Device disconnected");
      break;

    default:
      break;
  }
}

/**
  * @brief MX_USB_Host_Init
  *        Initialization of USB device.
  * Init USB Host Library, add supported class and start the library
  * @retval None
  */
UINT MX_USB_Host_Init(void)
{
  UINT ret = UX_SUCCESS;
  /* USER CODE BEGIN USB_Host_Init_PreTreatment_0 */
  /* USER CODE END USB_Host_Init_PreTreatment_0 */

  /* Initialize the hardware resources used by the Type-C power delivery (PD) */
  BSP_PWR_VBUSInit(TYPE_C_PORT_1);

  /* Set a fixed/variable PDO and manage the power control */
  BSP_PWR_VBUSSetVoltage_Fixed(TYPE_C_PORT_1, 5000U, 1, BSP_PWR_DCDC_PRECISION);

  /* The code below is required for installing the host portion of USBX. */
  if (ux_host_stack_initialize(ux_host_event_callback) != UX_SUCCESS)
  {
    ret = UX_ERROR;
  }

  /* Register hid class. */
  if (ux_host_stack_class_register(_ux_system_host_class_hid_name,
                                   _ux_host_class_hid_entry) != UX_SUCCESS)
  {
    ret = UX_ERROR;
  }

  /* Register HID Mouse client */
  if (ux_host_class_hid_client_register(_ux_system_host_class_hid_client_mouse_name,
                                        ux_host_class_hid_mouse_entry) != UX_SUCCESS)
  {
    ret = UX_ERROR;
  }

  /* Register HID Mouse client */
  if (ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name,
                                        ux_host_class_hid_keyboard_entry) != UX_SUCCESS)
  {
    ret = UX_ERROR;
  }

  /* Initialize the LL driver */
  MX_USB_DRD_FS_HCD_Init();

  /* Register all the USB host controllers available in this system.  */
  if (ux_host_stack_hcd_register(_ux_system_host_hcd_stm32_name,
                                 _ux_hcd_stm32_initialize,
                                 USB_DRD_PMAADDR,
                                 (ULONG)&hhcd_USB_DRD_FS) != UX_SUCCESS)
  {
    ret = UX_ERROR;
  }



  /* USER CODE BEGIN USB_Host_Init_PreTreatment_1 */
  /* USER CODE END USB_Host_Init_PreTreatment_1 */

  /* USER CODE BEGIN USB_Host_Init_PostTreatment */
  /* USER CODE END USB_Host_Init_PostTreatment */
  return ret;
}

/**
  * @brief  Function implementing the safety_protection_thread thread.
  * @param  thread_input: Not used
  * @retval None
  */
VOID safety_protection_thread_entry(ULONG thread_input)
{

  (void) thread_input;
  /* SAFETY PROTECTION CODE BEGIN */
  /* Infinite loop */
  while (1)
  {
    /* Safety protection code to be sure that VBUS is in the defined thresholds */
    if (BSP_PWR_VBUSIsOn(TYPE_C_PORT_1))
    {
      uint32_t voltage = 0U;
      /* Measure the VBUS power */
      if (BSP_ERROR_NONE == BSP_USBPD_PWR_VBUSGetVoltage(TYPE_C_PORT_1, &voltage))
      {
        /* VBUS should not exceed 5V +10% and not lower than 5V -7% */
        if((voltage > VBUS_SAFETY_HIGH) || (voltage < VBUS_SAFETY_LOW))
        {
          USBH_UsrLog("USB Vbus safety error please verify the power supply then reset the board");
          /* Remove the VBUS to prevent spike power on 19V plug */
          BSP_PWR_VBUSOff(TYPE_C_PORT_1);
          Error_Handler();
        }
      }
    }
    /* Sleep for 500s */
    tx_thread_sleep(MS_TO_TICK(50));
  }
  /* SAFETY PROTECTION CODE END */
}
/* USER CODE END 1 */
