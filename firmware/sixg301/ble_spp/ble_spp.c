#include "ble_spp.h"

#include "app_log.h"
#include "app_assert.h"
#include "cmsis_os2.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"
#include "gatt_db.h"
#include "sl_status.h"

#define BLE_TX_STACK_SIZE               1024U

static volatile uint8_t current_connection = 0xFFU;
static volatile bool current_notifications_enabled = false;

//extern osMessageQueueId_t uart_rx_queue;    // MCU -> BLE Rx Queue
osMessageQueueId_t ble_rx_queue;    // MCU -> BLE Rx Queue
static osThreadId_t ble_tx_thread;

static void ble_tx_task(void *pvargs);

/*
 * BLE UART communication Init
 */
void ble_uart_init(osMessageQueueId_t queue)
{
    // Define the MCU UART Rx task
    static const osThreadAttr_t ble_tx_thread_attributes = {
        .name = "ble_tx",
        .priority = osPriorityNormal,
        .stack_size = BLE_TX_STACK_SIZE    
    };

    ble_rx_queue = queue;
    // Create the BLE Tx Task
    ble_tx_thread = osThreadNew(ble_tx_task, NULL, &ble_tx_thread_attributes);
    app_assert(ble_tx_thread != NULL, "Failed to create MCU UART TX task\r\n");
}

 /*
 *  MCU -> BLE UART Rx Task
 */
static void ble_tx_task(void *pvargs)
{
    uart_rx_message_t msg;
    (void)pvargs;
    osDelay(1000U);
    app_log("MCU UART Rx Task started\r\n");

    while (true) {
        osStatus_t status = osMessageQueueGet(ble_rx_queue, // uart_rx_queue, 
            &msg, NULL,
            osWaitForever);
        if (status != osOK) {
            continue;
        }

        if((current_connection == 0xFFU) || !current_notifications_enabled) {
            app_log_warning("UART Rx dropped: BLE not ready, len = %u\r\n", msg.length);
            continue;
        }

        sl_status_t sc = sl_bt_gatt_server_send_notification(current_connection, 
            gattdb_My_SPP_Read,
            msg.length,
            msg.data);
        if(sc != SL_STATUS_OK) {
            app_log_warning("BLE Notification failed: 0x%08lx\r\n", (unsigned long)sc);
        } else {
            app_log_debug("MCU UART Tx: %u bytes\r\n", msg.length);
        }
    }
}

/*
 *  MCU UART Set the BLE state
 */
void mcu_uart_set_ble_state(uint8_t connection, bool notifications_enabled)
{
    current_connection = connection;
    current_notifications_enabled = notifications_enabled;
}

#if MCU_UART_RX_ENABLED == 0
sl_status_t mcu_uart_test_receive(const uint8_t *data, size_t len)
{
    uart_rx_message_t msg;

    if ((data == NULL) || (len == 0) || (len > sizeof(msg.data))) {
        return SL_STATUS_INVALID_PARAMETER;
    }

    memcpy(msg.data, data, len);
    msg.length = (uint16_t)len;

    osStatus_t status = osMessageQueuePut(uart_rx_queue, &msg, 0U, 0U);
    return (status == osOK) ? SL_STATUS_OK : SL_STATUS_FULL;
}
#endif