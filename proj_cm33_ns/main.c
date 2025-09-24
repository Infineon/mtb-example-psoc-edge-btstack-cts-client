/*****************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application in the CM33 CPU
*
* Related Document : See README.md
*
*******************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"
#include "cy_pdl.h"
#include "mtb_hal.h"
#include "retarget_io_init.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cy_time.h"
#include "cycfg_bt_settings.h"
#include "cts_client.h"
#include "cybsp_bt_config.h"
#include "wiced_bt_stack.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define BLINKY_LED_DELAY_MSEC            (1000U)

/* The timeout value in microsecond used to wait for core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC         (10U)

/* LED blink timer clock value in Hz  */
#define LED_BLINK_TIMER_CLOCK_HZ         (10000U)

/* LED blink timer period value */
#define LED_BLINK_TIMER_PERIOD           (9999U)

/* The timeout value in microsecond used to wait for core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC          (10U)
#define BT_PIN                            (0U)
#define WIFI_PIN                          (6U)
#define CONST_VALUE                       (0U)
#define OUT_VALUE                         (1U)

#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)
                                        
/*******************************************************************************
* Global Variables
*******************************************************************************/

#define LPTIMER_0_WAIT_TIME_USEC               (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority. */
#define APP_LPTIMER_INTERRUPT_PRIORITY         (1U)

/******************************************************************************
* Function Definitions
******************************************************************************/
TaskHandle_t  button_task_handle;
static mtb_hal_lptimer_t lptimer_obj;
/* RTC HAL object */
static mtb_hal_rtc_t rtc_obj;


/*******************************************************************************
* Function Name: setup_clib_support
********************************************************************************
* Summary:
*    1. This function configures and initializes the Real-Time Clock (RTC).
*    2. It then initializes the RTC HAL object to enable CLIB support library 
*       to work with the provided Real-Time Clock (RTC) module.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_clib_support(void)
{
    /* RTC Initialization */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);

    /* Initialize the ModusToolbox CLIB support library */
    mtb_clib_support_init(&rtc_obj);
}

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for LPTimer instance.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
* 1. This function first configures and initializes an interrupt for LPTimer.
* 2. Then it initializes the LPTimer HAL object to be used in the RTOS
*    tickless idle mode implementation to allow the device enter deep sleep
*    when idle task runs. LPTIMER_0 instance is configured for CM33 CPU.
* 3. It then passes the LPTimer object to abstraction RTOS library that
*    implements tickless idle mode
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
     /* Interrupt configuration structure for LPTimer */
     cy_stc_sysint_t lptimer_intr_cfg =
     {
             .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
             .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
     };

     /* Initialize the LPTimer interrupt and specify the interrupt handler. */
     cy_en_sysint_status_t interrupt_init_status =
             Cy_SysInt_Init(&lptimer_intr_cfg,
                     lptimer_interrupt_handler);

     /* LPTimer interrupt initialization failed. Stop program execution. */
     if(CY_SYSINT_SUCCESS != interrupt_init_status)
     {
         handle_app_error();
     }

     /* Enable NVIC interrupt. */
     NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

     /* Initialize the MCWDT block */
     cy_en_mcwdt_status_t mcwdt_init_status =
             Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW,
                     &CYBSP_CM33_LPTIMER_0_config);

     /* MCWDT initialization failed. Stop program execution. */
     if(CY_MCWDT_SUCCESS != mcwdt_init_status)
     {
         handle_app_error();
     }

     /* Enable MCWDT instance */
     Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
             CY_MCWDT_CTR_Msk,
             LPTIMER_0_WAIT_TIME_USEC);

     /* Setup LPTimer using the HAL object and desired configuration as defined
      * in the device configurator. */
     cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj,
             &CYBSP_CM33_LPTIMER_0_hal_config);

     /* LPTimer setup failed. Stop program execution. */
     if(CY_RSLT_SUCCESS != result)
     {
         handle_app_error();
     }

     /* Pass the LPTimer object to abstraction RTOS library that implements
      * tickless idle mode
      */
     cyabs_rtos_set_lptimer(&lptimer_obj);
}

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function of the CM33 non-secure application.
* Entry point to the application. Set device configuration and start BT
* stack initialization.  The actual application initialization will happen
* when stack reports that BT device is ready.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
     cy_rslt_t result;
     BaseType_t rtos_result;
     /* Initialize the device and board peripherals */
     result = cybsp_init();

     /* Board init failed. Stop program execution */
     if (CY_RSLT_SUCCESS != result)
     {
         handle_app_error();
     }
     
     /* Setup CLIB support library. */
     setup_clib_support();

     /* Setup the LPTimer instance for CM33 CPU. */
     setup_tickless_idle_timer();

     /* Initialize retarget-io middleware */
     init_retarget_io();

     printf("\x1b[2J\x1b[;H");
     printf("************ "
             "PSOC Edge MCU : Current Time Service (CTS) - Client "
             "*************\n\n");

     /* Disabling the1 WIFI Domain */
     Cy_GPIO_Write(GPIO_PRT11, WIFI_PIN, CONST_VALUE);

     /* Register call back and configuration with stack */
     result = wiced_bt_stack_init(app_bt_management_callback,
             &cy_bt_cfg_settings);

     /* Check if stack initialization was successful */
     if( WICED_BT_SUCCESS == result)
     {
         printf("Bluetooth Stack Initialization Successful \n");
     }
     else
     {
         printf("Bluetooth Stack Initialization failed!! \n");
         handle_app_error();
     }

     /* Enable global interrupts */
     __enable_irq();

     /* Create Button Task for processing button presses */
     rtos_result = xTaskCreate(button_task,"button_task", BUTTON_TASK_STACK_SIZE,
             NULL, BUTTON_TASK_PRIORITY, &button_task_handle);
     if( pdPASS != rtos_result)
     {
         printf("Failed to create Button task! \n");
         handle_app_error();
     }

    /* Set idle power mode configuration of SOCMEM to deepsleep */
    Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

        /* Enable CM55. */
        /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
        Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

     /* Start the FreeRTOS scheduler */
     vTaskStartScheduler();

     /* The application should never reach here */
     handle_app_error();

}

/* [] END OF FILE */