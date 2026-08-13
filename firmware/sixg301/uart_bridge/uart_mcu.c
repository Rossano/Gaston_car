#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#include "app_log.h"
#include "app_assert.h"
#include "sl_status.h"
#include <string.h>

#include "uart_mcu.h"
#include "ble_spp/ble_spp.h"

/*
 *  Module Defines
 */
#define MCU_UART_TX_QUEUE_DEPTH         8U
#define MCU_UART_RX_QUEUE_DEPTH         8U

#define MCU_UART_TX_CHUNK_SIZE          128U
#define MCU_UART_RX_CHUNK_SIZE          128U

#define MCU_UART_TX_STACK_SIZE          1024U
#define MCU_UART_RX_STACK_SIZE          1024U
//#define BLE_TX_STACK_SIZE               1024U


/*
 *  Global Variables for MCU UART
 */
uart_tx_message_t tx_msg;                       // TX exchange message variable
uart_rx_message_t rx_msg;                       // RX exchange message variable

static osMessageQueueId_t uart_tx_queue;        // BLE -> MCU Tx Queue
static osThreadId_t uart_tx_thread;             // MCU TX Task (BLE -> MCU) 
osMessageQueueId_t uart_rx_queue;               // MCU -> BLE Rx Queue
static osThreadId_t ble_tx_thread;
#if MCU_UART_RX_ENABLED
static osThreadId_t uart_rx_thread;             // MCU RX Task (MCU -> BLE)
#endif

//static volatile uint8_t current_connection = 0xFFU;
//static volatile bool current_notifications_enabled = false;

/*
 *  Function prototypes
*/
static void mcu_uart_tx_task(void *pvargs);
void mcu_uart_set_ble_state(uint8_t connection, bool notifications_enabled);

/*
 *  MCU UART TX Init function
 */
void mcu_uart_init(void) 
{
    // Define the UART Tx queue
    static const osMessageQueueAttr_t tx_queue_attributes = {
        .name = "mcu_uart_tx_queue"
    };
    app_assert(sl_iostream_mcu_uart_handle != NULL, "MCU UART handle is NULL\r\n");
    // Define the MCU UART Tx task
    static const osThreadAttr_t tx_thread_attributes = {
        .name = "mcu_uart_tx",
        .priority = osPriorityNormal,
        .stack_size = MCU_UART_TX_STACK_SIZE
    };
    // Define the UART Rx queue
    static const osMessageQueueAttr_t rx_queue_attributes = {
        .name = "uart_to_ble_queue"
    };
    app_assert(sl_iostream_mcu_uart_handle != NULL, "MCU UART RX handle is NULL\r\n");
    // // Define the MCU UART Rx task
    // static const osThreadAttr_t ble_tx_thread_attributes = {
    //     .name = "ble_tx",
    //     .priority = osPriorityNormal,
    //     .stack_size = BLE_TX_STACK_SIZE    
    // };
    (void)MCU_UART_TX_BUFFER_SIZE;
    // Create the MCU UART Tx queue
    uart_tx_queue = osMessageQueueNew(8U, 
        sizeof(uart_tx_message_t),
        &tx_queue_attributes);
    app_assert(uart_tx_queue != NULL, "Failed to create MCU UART TX queue\r\n");
    (void)MCU_UART_RX_BUFFER_SIZE;
    // Create the MCU UART Rx queue
    uart_rx_queue = osMessageQueueNew(8U, 
        sizeof(uart_rx_message_t),
        &rx_queue_attributes);
    app_assert(uart_tx_queue != NULL, "Failed to create MCU UART RX queue\r\n");

    // // Create the BLE Tx Task
    // ble_tx_thread = osThreadNew(ble_tx_task, NULL, &ble_tx_thread_attributes);
    // app_assert(uart_tx_thread != NULL, "Failed to create MCU UART TX task\r\n");
    
#if MCU_UART_RX_ENABLED
    static const osThreadAttr_t uart_rx_thread_attributes = {
        .name = "mcu_uart_rx",
        .priority = osPriorityAboveNormal,
        .stack_szie = MCU_UART_RX_TASK_STACK_SIZE
    };
    // Create the MCU UART Rx Task
    uart_rx_thread = osThreadNew(mcu_uart_rx_task, NULL, &uart_rx_thread_attributes);
    app_assert(uart_rx_thread != NULL, "Failed to create MCU UART RX task\r\n");
#endif
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
        queue_status = osMessageQueueGet(uart_tx_queue, &tx_msg, NULL, osWaitForever);
        if(queue_status != osOK) {
            // If there is an error redo the loop
            continue;
        }

        // Write the received message
        write_status = sl_iostream_write(sl_iostream_mcu_uart_handle, tx_msg.data, tx_msg.length);
        if(write_status != SL_STATUS_OK) {
            app_log_error("MCU UART Tx write failed: 0x%081x\r\n", (unsigned int)write_status);
        } else {
            app_log_debug("MCU UART Tx: %u bytes\r\n", (unsigned int)tx_msg.length);
        }
    }
}

#if MCU_UART_RX_ENABLED
/*
 *  MCU UART Rx Task
 */
static void mcu_uart_rx_task(void *pvargs)
{
    uart_rx_message_t msg;
    (void)pvargs;
    app_log("MCU UART Rx Task started\r\n");

    while (true) {
        size_t bytes_read = 0;

        sl_status_t sc = sl_iostream_read(sliostream_mcu_uart_handle, msg.data, sizeof(msg.data), &bytes_read);
        if((sc == SL_STATUS_OK) && (bytes_read > 0)) {
            msg.length = (uint16_t)bytes_read;
            osStatus_t status = osMessageQueuePut(uart_rx_queue, 
                &msg, 
                0U,
                0U);
            if(status != osOK) {
                app_log_warning("UART to BLE queue full, dropped %u bytes \r\n", (unsigned int)bytes_read);
            }
        } else {
            osDelay(10U);
        }
        
    }
}

#endif

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
        if(chunck_len > sizeof(tx_msg.data)) {
            chunck_len = sizeof(tx_msg.data);
        }
        tx_msg.length = (uint16_t)chunck_len;
        memcpy(tx_msg.data, data, chunck_len);

        // Send the data over the Tx queue
        osStatus_t status = osMessageQueuePut(uart_tx_queue, &tx_msg, 0U, 0U);
        if (status != osOK) {
            return SL_STATUS_FULL;
        }

        // Get the remaining data
        data += chunck_len;
        len -= chunck_len;
    }

    return SL_STATUS_OK;
}

// /*
//  *  MCU UART Set the BLE state
//  */
// void mcu_uart_set_ble_state(uint8_t connection, bool notifications_enabled)
// {
//     current_connection = connection;
//     current_notifications_enabled = notifications_enabled;
// }

sl_status_t mcu_uart_test_receive(const uint8_t *data, size_t len)
{
    uart_rx_message_t msg;

    if((data == NULL) || (len == 0) || (len > sizeof(msg.data))) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    memcpy(msg.data, data, len);
    msg.length = (uint16_t)len;

    osStatus_t status = osMessageQueuePut(uart_rx_queue, &msg, 0U, 0U);

    return (status == osOK) ? SL_STATUS_OK : SL_STATUS_FULL;
}