
## <b>Ux_Host_MSC application description</b>

This application provides an example of Azure RTOS USBX stack usage. It shows how to develop USB Host Mass Storage "MSC" able to enumerate and communicates with a removable usb flash disk.

The application is designed to behave as an USB MSC Host able to operate with an USB flash disk using the Bulk Only Transfer (BOT) and Small Computer System Interface (SCSI) transparent commands combined with a file system Azure RTOS FileX.

The main entry function tx_application_define() is then called by ThreadX during kernel start, at this stage, all USBx resources are initialized, the MSC Class driver is registered.
The application creates two threads :

  - usbx_app_thread_entry    (Priority : 25; Preemption threshold : 25) used to initialize USB DRD HAL HCD driver and start the Host.
  - msc_process_thread_entry (Priority : 30; Preemption threshold : 30) used to proceed to file operations once the device is properly enumerated.

####  <b>Expected success behavior</b>

When an usb flash disk is plugged to STM32G0C1E-EV board, a Message will be displayed on the uart HyperTerminal showing  the Vendor ID and the Product ID of the attached device.
After enumeration phase , the host proceed to file operations :

  - Create a "Test.txt" file.
  - Write  a small text in the created file.
  - Read the written text and check data integrity
  - Close the File

During the file operations process a message will be displayed on the HyperTerminal to indicates the outcome of each operation  (Create/Write/Read/Close) .
If all operations were successful a message will be displayed on the HyperTerminal to indicates the end of operations.

#### <b>Error behaviors</b>

Errors are detected such as (Unsupported device, Enumeration Fail, File operations fail)and the corresponding message is displayed on the HyperTerminal .

#### <b>Assumptions if any</b>

User is familiar with USB 2.0 "Universal Serial BUS" Specification and Mass storage class Specification.

#### <b>Known limitations</b>

### <b>Notes</b>
 - When the power source jumper JP24 is configured to D5V position (with this configuration the mother board is powered from the daughter board),
   there is no risk to get an over voltage on USB Vbus connector CN7. When the ST-Link is used as power source there is a risk to get an over voltage in connector CN7.
   In such power configuration, software safety protection is required to detect the overvoltage. Please refer to SAFETY PROTECTION CODE BEGIN section in app_usbx_host.c.
 - User shall remove the jumpers JP3 and JP4.

#### <b>ThreadX usage hints</b>

 - ThreadX uses the Systick as time base, thus it is mandatory that the HAL uses a separate time base through the TIM IPs.
 - ThreadX is configured with 100 ticks/sec by default, this should be taken into account when using delays or timeouts at application. It is always possible to reconfigure it in the "tx_user.h", the "TX_TIMER_TICKS_PER_SECOND" define,but this should be reflected in "tx_initialize_low_level.S" file too.
 - ThreadX is disabling all interrupts during kernel start-up to avoid any unexpected behavior, therefore all system related calls (HAL, BSP) should be done either at the beginning of the application or inside the thread entry functions.
 - ThreadX offers the "tx_application_define()" function, that is automatically called by the tx_kernel_enter() API.
   It is highly recommended to use it to create all applications ThreadX related resources (threads, semaphores, memory pools...)  but it should not in any way contain a system API call (HAL or BSP).
 - Using dynamic memory allocation requires to apply some changes to the linker file.
   ThreadX needs to pass a pointer to the first free memory location in RAM to the tx_application_define() function,
   using the "first_unused_memory" argument.
   This require changes in the linker files to expose this memory location.
    + For EWARM add the following section into the .icf file:
     ```
	 place in RAM_region    { last section FREE_MEM };
	 ```
    + For MDK-ARM:
	```
    either define the RW_IRAM1 region in the ".sct" file
    or modify the line below in "tx_low_level_initilize.s to match the memory region being used
        LDR r1, =|Image$$RW_IRAM1$$ZI$$Limit|
	```
    + For STM32CubeIDE add the following section into the .ld file:
	```
    ._threadx_heap :
      {
         . = ALIGN(8);
         __RAM_segment_used_end__ = .;
         . = . + 64K;
         . = ALIGN(8);
       } >RAM_D1 AT> RAM_D1
	```

       The simplest way to provide memory for ThreadX is to define a new section, see ._threadx_heap above.
       In the example above the ThreadX heap size is set to 64KBytes.
       The ._threadx_heap must be located between the .bss and the ._user_heap_stack sections in the linker script.
       Caution: Make sure that ThreadX does not need more than the provided heap memory (64KBytes in this example).
       Read more in STM32CubeIDE User Guide, chapter: "Linker script".

    + The "tx_initialize_low_level.S" should be also modified to enable the "USE_DYNAMIC_MEMORY_ALLOCATION" flag.

### <b>Keywords</b>

Connectivity, USBXHost, FILEX, ThreadX, MSC, Mass Storage, BOT, SCSI, Removable drive, UART/USART


### <b>Hardware and Software environment</b>

  - This application runs on STM32G0xx devices.
  - This application has been tested with STMicroelectronics STM32G0C1E-EV boards Revision MB1312 A-01
    and can be easily tailored to any other supported device and development board.

  - STM32G0C1E-EV board Set-up
    - Plug the USB key into the STM32G0C1E-EV board through 'USB micro Type C-Male
      to A-Female' cable to the connector:CN7
    - Connect ST-Link cable to the PC USB port to display data on the HyperTerminal.

    A virtual COM port will then appear in the HyperTerminal:

     - Hyperterminal configuration
       - Data Length = 8 Bits
       - One Stop Bit
       - No parity
       - BaudRate = 115200 baud
       - Flow control: None

### <b>How to use it ?</b>

In order to make the program work, you must do the following :

 - Open your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Open the configured uart HyperTerminal in order to display debug messages.
 - Run the application