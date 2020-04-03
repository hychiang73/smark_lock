/**@file
 *
 * @defgroup ble_nus Nordic UART Service
 * @{
 * @ingroup  ble_sdk_srv
 * @brief    Nordic UART Service implementation.
 *
 * @details The Nordic UART Service is a simple GATT-based service with TX and RX characteristics.
 *          Data received from the peer is passed to the application, and the data received
 *          from the application of this service is sent to the peer as Handle Value
 *          Notifications. This module demonstrates how to implement a custom GATT-based
 *          service and characteristics using the SoftDevice. The service
 *          is used by the application to send and receive ASCII text strings to and from the
 *          peer.
 *
 * @note    The application must register this module as BLE event observer using the
 *          NRF_SDH_BLE_OBSERVER macro. Example:
 *          @code
 *              ble_nus_t instance;
 *              NRF_SDH_BLE_OBSERVER(anything, BLE_NUS_BLE_OBSERVER_PRIO,
 *                                   ble_nus_on_ble_evt, &instance);
 *          @endcode
 */

#ifndef BLE_MY_UARTS_H__
#define BLE_MY_UARTS_H__
#include <stdint.h>
#include <stdbool.h>
#include "sdk_config.h"
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "ble_link_ctx_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@brief   Macro for defining a ble_nus instance.
 *
 * @param     _name            Name of the instance.
 * @param[in] _nus_max_clients Maximum number of NUS clients connected at a time.
 * @hideinitializer
 */
#define BLE_UARTS_DEF(_name, _uarts_max_clients)                  \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage),  \
                             (_uarts_max_clients),                \
                             sizeof(ble_uarts_client_context_t)); \
	  static ble_uarts_t _name =                                    \
	  {                                                             \
        .p_link_ctx_storage = &CONCAT_2(_name, _link_ctx_storage) \
    };                                                            \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_uarts_on_ble_evt,                    \
                         &_name)

// 128bis UUID
#define UARTS_BASE_UUID      {{0x40, 0xE3, 0x4A, 0x1D, 0xC2, 0x5F, 0xB0, 0x9C, 0xB7, 0x47, 0xE6, 0x43, 0x00, 0x00, 0x53, 0x86}} 
// 16bit UUID of characteristic and service
#define BLE_UUID_UARTS_SERVICE 0x000A
#define BLE_UUID_UARTS_TX_CHARACTERISTIC 0x000B           
#define BLE_UUID_UARTS_RX_CHARACTERISTIC 0x000C

#define OPCODE_LENGTH        1
#define HANDLE_LENGTH        2

/**@brief   Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_UARTS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_UARTS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif


/**@brief   Nordic UART Service event types. */
typedef enum
{
    BLE_UARTS_EVT_RX_DATA,      /**< Data received. */
    BLE_UARTS_EVT_TX_RDY,       /**< Service is ready to accept new data to be transmitted. */
	  BLE_NUS_EVT_COMM_STARTED,   /**< Notification has been enabled. */
    BLE_NUS_EVT_COMM_STOPPED,   /**< Notification has been disabled. */
} ble_uarts_evt_type_t;	
	
/* Forward declaration of the ble_nus_t type. */
typedef struct ble_uarts_s ble_uarts_t;


/**@brief   Nordic UART Service @ref BLE_NUS_EVT_RX_DATA event data.
 *
 * @details This structure is passed to an event when @ref BLE_NUS_EVT_RX_DATA occurs.
 */
typedef struct
{
    uint8_t const * p_data; /**< A pointer to the buffer with received data. */
    uint16_t        length; /**< Length of received data. */
} ble_uarts_evt_rx_data_t;


/**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
typedef struct
{
    bool is_notification_enabled; 
} ble_uarts_client_context_t;


/**@brief   Nordic UART Service event structure.
 *
 * @details This structure is passed to an event coming from service.
 */
typedef struct
{
    ble_uarts_evt_type_t       type;        /**< Event type. */
    ble_uarts_t                * p_uarts;   /**< A pointer to the instance. */
    uint16_t                   conn_handle; /**< Connection handle. */
    ble_uarts_client_context_t * p_link_ctx;/**< A pointer to the link context. */
    union
    {
        ble_uarts_evt_rx_data_t rx_data; /**< @ref BLE_NUS_EVT_RX_DATA event data. */
    } params;
} ble_uarts_evt_t;


/**@brief Nordic UART Service event handler type. */
typedef void (* ble_uarts_data_handler_t) (ble_uarts_evt_t * p_evt);


/**@brief   Nordic UART Service initialization structure.
 *
 * @details This structure contains the initialization information for the service. The application
 * must fill this structure and pass it to the service using the @ref ble_nus_init
 *          function.
 */
typedef struct
{
    ble_uarts_data_handler_t data_handler; /**< Event handler to be called for handling received data. */
} ble_uarts_init_t;


/**@brief   Nordic UART Service structure.
 *
 * @details This structure contains status information related to the service.
 */
struct ble_uarts_s
{
    uint8_t                         uuid_type;          /**< UUID type for Nordic UART Service Base UUID. */
    uint16_t                        service_handle;     /**< Handle of Nordic UART Service (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        tx_handles;         /**< Handles related to the TX characteristic (as provided by the SoftDevice). */
    ble_gatts_char_handles_t        rx_handles;         /**< Handles related to the RX characteristic (as provided by the SoftDevice). */
	  blcm_link_ctx_storage_t * const p_link_ctx_storage; /**< Pointer to link context storage with handles of all current connections and its context. */
    ble_uarts_data_handler_t        data_handler;       /**< Event handler to be called for handling received data. */
};

/**@brief   Function for initializing the Nordic UART Service.
 *
 * @param[out] p_nus      Nordic UART Service structure. This structure must be supplied
 *                        by the application. It is initialized by this function and will
 *                        later be used to identify this particular service instance.
 * @param[in] p_nus_init  Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_nus or p_nus_init is NULL.
 */
uint32_t ble_uarts_init(ble_uarts_t * p_uarts, ble_uarts_init_t const * p_uarts_init);


/**@brief   Function for handling the Nordic UART Service's BLE events.
 *
 * @details The Nordic UART Service expects the application to call this function each time an
 * event is received from the SoftDevice. This function processes the event if it
 * is relevant and calls the Nordic UART Service event handler of the
 * application if necessary.
 *
 * @param[in] p_ble_evt     Event received from the SoftDevice.
 * @param[in] p_context     Nordic UART Service structure.
 */
void ble_uarts_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


/**@brief   Function for sending a data to the peer.
 *
 * @details This function sends the input string as an RX characteristic notification to the
 *          peer.
 *
 * @param[in]     p_nus       Pointer to the Nordic UART Service structure.
 * @param[in]     p_data      String to be sent.
 * @param[in,out] p_length    Pointer Length of the string. Amount of sent bytes.
 * @param[in]     conn_handle Connection Handle of the destination client.
 *
 * @retval NRF_SUCCESS If the string was sent successfully. Otherwise, an error code is returned.
 */
uint32_t ble_uarts_data_send(ble_uarts_t * p_uarts,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle);


#ifdef __cplusplus
}
#endif

#endif // BLE_MY_UARTS_H__


