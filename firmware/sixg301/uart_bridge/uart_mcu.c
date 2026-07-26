#include "uart_mcu.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "cmsis_os2.h"
//#include "portmacrocommon.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_status.h"
//#include "portable.h"
#include <string.h>

/*
 *  Global Variables for MCU UART
 */
uart_tx_message_t msg;                      // TX exchange message variable

static osMessageQueueId_t uart_tx_queue;    // BLE -> MCU Tx Queue
static osThreadId_t uart_tx_thread;         // MCU TX Task 
//static TaskHandle_t uart_tx_task_handle = NULL;

/*
 *  Function prototypes
*/
static void mcu_uart_tx_task(void *pvargs);

/*
 *  MCU UART TX Init function
 */
void mcu_uart_init(void) 
{
    // Define the UART Tx queue
    static const osMessageQueueAttr_t queue_attributes = {
        .name = "mcu_uart_tx_queue"
    };
    app_assert(sl_iostream_mcu_uart_handle != NULL, "MCU UART handle is NULL\r\n");
    // Define the MCU UART Tx task
    static const osThreadAttr_t thread_attributes = {
        .name = "mcu_uart_tx",
        .priority = osPriorityNormal,
        .stack_size = 1024    
    };

    (void)MCU_UART_TX_BUFFER_SIZE;
    //BaseType_t ret;
    // Create the MCU UART Tx queue
    uart_tx_queue = osMessageQueueNew(8U, sizeof(uart_tx_message_t),&queue_attributes);
    app_assert(uart_tx_queue != NULL, "Failed to create MCU UART TX queue\r\n");

    // Create the MCU UART Tx Task
    uart_tx_thread = osThreadNew(mcu_uart_tx_task, NULL, &thread_attributes);
    app_assert(uart_tx_thread != NULL, "Failed to create MCU UART TX task\r\n");
    // ret = xTaskCreate(
    //     mcu_uart_tx_task,
    //     "uart Tx task", 
    //     512, 
    //     NULL, 
    //     tskIDLE_PRIORITY + 2, 
    //     &uart_tx_task_handle);
    // //app_assert(ret != 0, "Failed to create MCU UART TX task\r\n");
    // if(ret != pdPASS) {
    //     app_log("UART Task creation failed, free heap: %lu\r\n", (unsigned long)xPortGetFreeHeapSize());
    // } else {
    //     app_log("UART Task creation done, free heap: %lu\r\n", (unsigned long)xPortGetFreeHeapSize());
    // }
}

/*
 *  MCU UART Tx Task
 */
static void mcu_uart_tx_task(void *pvargs)
{
    (void)pvargs;
    app_log("MCU UART TX task started!\r\n");

    // Internal variable declaration
    osStatus_t queue_status;
    sl_status_t write_status;

    // Task loop
    while(true) {
        // Waits a message from UART Tx Queue
        queue_status = osMessageQueueGet(uart_tx_queue, &msg, NULL, osWaitForever);
        if(queue_status != osOK) {
            // If there is an error redo the loop
            continue;
        }

        // Write the received message
        write_status = sl_iostream_write(sl_iostream_mcu_uart_handle, msg.data, msg.length);
        if(write_status != SL_STATUS_OK) {
            app_log_error("MCU UART Tx write failed: 0x%081x\r\n", (unsigned int)write_status);
        } else {
            app_log_debug("MCU UART Tx: %u bytes\r\n", (unsigned int)msg.length);
        }
    }
}

/*
 *  Function to send a message over UART
 */
sl_status_t mcu_uart_send(const uint8_t *data, size_t len)
{
    // Return the error if there are no data
    if((data == NULL) || (len == 0)) {
        return SL_STATUS_INVALID_PARAMETER;
    }
    // Return the error if the uart Tx queue is not ready
    if(uart_tx_queue == NULL) {
        return SL_STATUS_NOT_INITIALIZED;
    }

    // Chop the data and send them 
    while (len > 0) {
        size_t chunck_len = len;

        // Copy the data to the message structure
        if(chunck_len > sizeof(msg.data)) {
            chunck_len = sizeof(msg.data);
        }
        msg.length = (uint16_t)chunck_len;
        memcpy(msg.data, data, chunck_len);

        // Send the data over the Tx queue
        osStatus_t status = osMessageQueuePut(uart_tx_queue, &msg, 0U, 0U);
        if (status != osOK) {
            return SL_STATUS_FULL;
        }

        // Get the remaining data
        data += chunck_len;
        len -= chunck_len;
    }

    return SL_STATUS_OK;
}