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
#include "portable.h"

uart_tx_message_t msg;

static osMessageQueueId_t uart_tx_queue;
static osThreadId_t uart_tx_thread;
static TaskHandle_t uart_tx_task_handle = NULL;

static void mcu_uart_tx_task(void *pvargs);

void mcu_uart_init(void) 
{
    static const osMessageQueueAttr_t queue_attributes = {
        .name = "mcu_uart_tx_queue"
    };
    static const osThreadAttr_t thread_attributes = {
        .name = "mcu_uart_tx",
        .priority = osPriorityNormal,
        .stack_size = 1024    
    };

    (void)MCU_UART_TX_BUFFER_SIZE;
    BaseType_t ret;
    uart_tx_queue = osMessageQueueNew(8U, sizeof(uart_tx_message_t),&queue_attributes);
    app_assert(uart_tx_queue != NULL, "Failed to create MCU UART TX queue\r\n");

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
    if(ret != pdPASS) {
        app_log("UART Task creation failed, free heap: %lu\r\n", (unsigned long)xPortGetFreeHeapSize());
    } else {
        app_log("UART Task creation done, free heap: %lu\r\n", (unsigned long)xPortGetFreeHeapSize());
    }
}

static void mcu_uart_tx_task(void *pvargs)
{
    (void)pvargs;
    app_log("MCU UART TX task started!\r\n");

    osStatus_t queue_status;
    sl_status_t write_status;
    //uart_tx_message_t msg;
    while(true) {
        queue_status = osMessageQueueGet(uart_tx_queue, &msg, NULL, osWaitForever);
        if(queue_status != osOK) {
            continue;
        }

        write_status = sl_iostream_write(sl_iostream_mcu_uart_handle, msg.data, msg.lenght);
        if(write_status != SL_STATUS_OK) {
            app_log_error("MCU UART Tx write failed: 0x%081x\r\n", (unsigned int)write_status);
        } else {
            app_log_debug("MCU UART Tx: %u bytes\r\n", (unsigned int)msg.lenght);
        }
    }
}

sl_status_t mcu_uart_send(const uint8_t data[], size_t len)
{
    if(/*(data == NULL) ||*/ (len == 0)) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    while (len > 0) {
        uart_tx_message_t msg;
        size_t chunck_len = len;

        if(chunck_len > sizeof(msg.data)) {
            chunck_len = sizeof(msg.data);
        }
        msg.lenght = (uint16_t)chunck_len;

        for(size_t i=0; i < chunck_len; i++) {
            msg.data[i] = data[i];
        }

        osStatus_t status = osMessageQueuePut(uart_tx_queue, &msg, 0U, 0U);
        if (status != osOK) {
            return SL_STATUS_FULL;
        }

        data += chunck_len;
        len -= chunck_len;
    }

    return SL_STATUS_OK;
}