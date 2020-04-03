#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_UARTS)
#include "uarts.h"

#include <stdlib.h>
#include <string.h>
#include "app_error.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"
#include "nrf_log.h"

#define BLE_UARTS_MAX_RX_CHAR_LEN        BLE_UARTS_MAX_DATA_LEN
#define BLE_UARTS_MAX_TX_CHAR_LEN        BLE_UARTS_MAX_DATA_LEN


/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the SoftDevice.
 *
 * @param[in] p_nus     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(ble_uarts_t * p_uarts, ble_evt_t const * p_ble_evt)
{
    ret_code_t                 err_code;
    ble_uarts_evt_t              evt;
    ble_gatts_value_t          gatts_val;
    uint8_t                    cccd_value[2];
    ble_uarts_client_context_t * p_client = NULL;

    err_code = blcm_link_ctx_get(p_uarts->p_link_ctx_storage,
                                 p_ble_evt->evt.gap_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gap_evt.conn_handle);
    }

    /* Check the hosts CCCD value to inform of readiness to send data using the RX characteristic */
    memset(&gatts_val, 0, sizeof(ble_gatts_value_t));
    gatts_val.p_value = cccd_value;
    gatts_val.len     = sizeof(cccd_value);
    gatts_val.offset  = 0;

    err_code = sd_ble_gatts_value_get(p_ble_evt->evt.gap_evt.conn_handle,
                                      p_uarts->rx_handles.cccd_handle,
                                      &gatts_val);

    if ((err_code == NRF_SUCCESS)     &&
        (p_uarts->data_handler != NULL) &&
        ble_srv_is_notification_enabled(gatts_val.p_value))
    {
        if (p_client != NULL)
        {
            p_client->is_notification_enabled = true;
        }

        memset(&evt, 0, sizeof(ble_uarts_evt_t));
        evt.type        = BLE_NUS_EVT_COMM_STARTED;
        evt.p_uarts       = p_uarts;
        evt.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        evt.p_link_ctx  = p_client;

        p_uarts->data_handler(&evt);
    }
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the SoftDevice.
 *
 * @param[in] p_nus     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_uarts_t * p_uarts, ble_evt_t const * p_ble_evt)
{
    ret_code_t                    err_code;
	  ble_uarts_evt_t               evt;
	  ble_uarts_client_context_t    * p_client;
    ble_gatts_evt_write_t const   * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

	  err_code = blcm_link_ctx_get(p_uarts->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
    }
	
    memset(&evt, 0, sizeof(ble_uarts_evt_t));

    evt.p_uarts       = p_uarts;
    evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
		evt.p_link_ctx  = p_client;
 
		if ((p_evt_write->handle == p_uarts->tx_handles.cccd_handle) &&
        (p_evt_write->len == 2))
    {
        if (p_client != NULL)
        {
            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                p_client->is_notification_enabled = true;
                evt.type                          = BLE_NUS_EVT_COMM_STARTED;
            }
            else
            {
                p_client->is_notification_enabled = false;
                evt.type                          = BLE_NUS_EVT_COMM_STOPPED;
            }

            if (p_uarts->data_handler != NULL)
            {
                p_uarts->data_handler(&evt);
            }

        }
    }
    else if ((p_evt_write->handle == p_uarts->rx_handles.value_handle) &&
             (p_uarts->data_handler != NULL))
    {
			  evt.type                  = BLE_UARTS_EVT_RX_DATA;
        evt.params.rx_data.p_data = p_evt_write->data;
        evt.params.rx_data.length = p_evt_write->len;
        p_uarts->data_handler(&evt);
    }
    else
    {
       // Do Nothing. This event is not relevant for this service.
    }
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_HVN_TX_COMPLETE event from the SoftDevice.
 *
 * @param[in] p_nus     Nordic UART Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_hvx_tx_complete(ble_uarts_t * p_uarts, ble_evt_t const * p_ble_evt)
{
    ret_code_t                   err_code;
	  ble_uarts_evt_t              evt;
	  ble_uarts_client_context_t   * p_client;
    
    err_code = blcm_link_ctx_get(p_uarts->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
    }

	  //if (p_client->is_notification_enabled)
		//{
				memset(&evt, 0, sizeof(ble_uarts_evt_t));
				evt.type        = BLE_UARTS_EVT_TX_RDY;
				evt.p_uarts     = p_uarts;
				evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
				evt.p_link_ctx  = p_client;
				p_uarts->data_handler(&evt);
		//}
}

void ble_uarts_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
	  if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_uarts_t * p_uarts = (ble_uarts_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_uarts, p_ble_evt);
            break;
			
			  case BLE_GATTS_EVT_WRITE:
            on_write(p_uarts, p_ble_evt);
            break;

        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            on_hvx_tx_complete(p_uarts, p_ble_evt);
            break;

        default:
            break;
    }
}

uint32_t ble_uarts_init(ble_uarts_t * p_uarts, ble_uarts_init_t const * p_uarts_init)
{
    ret_code_t            err_code;
    ble_uuid_t            ble_uuid;
    ble_uuid128_t         nus_base_uuid = UARTS_BASE_UUID;
    ble_add_char_params_t add_char_params;

    VERIFY_PARAM_NOT_NULL(p_uarts);
    VERIFY_PARAM_NOT_NULL(p_uarts_init);

    p_uarts->data_handler = p_uarts_init->data_handler;

    err_code = sd_ble_uuid_vs_add(&nus_base_uuid, &p_uarts->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_uarts->uuid_type;
    ble_uuid.uuid = BLE_UUID_UARTS_SERVICE;

	  // Add a custom base UUID.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_uarts->service_handle);
    VERIFY_SUCCESS(err_code);

		// Add the RX Characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = BLE_UUID_UARTS_RX_CHARACTERISTIC;
    add_char_params.uuid_type                = p_uarts->uuid_type;
    add_char_params.max_len                  = BLE_UARTS_MAX_RX_CHAR_LEN;
    add_char_params.init_len                 = sizeof(uint8_t);
    add_char_params.is_var_len               = true;
    add_char_params.char_props.write         = 1;
    add_char_params.char_props.write_wo_resp = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;

    err_code = characteristic_add(p_uarts->service_handle, &add_char_params, &p_uarts->rx_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the TX Characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BLE_UUID_UARTS_TX_CHARACTERISTIC;
    add_char_params.uuid_type         = p_uarts->uuid_type;
    add_char_params.max_len           = BLE_UARTS_MAX_TX_CHAR_LEN;
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.is_var_len        = true;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    return characteristic_add(p_uarts->service_handle, &add_char_params, &p_uarts->tx_handles);
}

uint32_t ble_uarts_data_send(ble_uarts_t * p_uarts,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle)
{
    ret_code_t                 err_code;
	  ble_gatts_hvx_params_t     hvx_params;
	  ble_uarts_client_context_t * p_client;

    VERIFY_PARAM_NOT_NULL(p_uarts);
	
	  err_code = blcm_link_ctx_get(p_uarts->p_link_ctx_storage, conn_handle, (void *) &p_client);
    VERIFY_SUCCESS(err_code);

	  if ((conn_handle == BLE_CONN_HANDLE_INVALID) || (p_client == NULL))
    {
        return NRF_ERROR_NOT_FOUND;
    }

    if (!p_client->is_notification_enabled)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (*p_length > BLE_UARTS_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    memset(&hvx_params, 0, sizeof(hvx_params));
    hvx_params.handle = p_uarts->tx_handles.value_handle;
    hvx_params.p_data = p_data;
    hvx_params.p_len  = p_length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(conn_handle, &hvx_params);
}

#endif // NRF_MODULE_ENABLED(BLE_DIS)
