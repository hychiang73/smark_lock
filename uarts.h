/****************************************Copyright (c)************************************************
**                                      [艾克姆科技]
**                                        IIKMSIK 
**                            官方店铺：https://acmemcu.taobao.com
**                            官方论坛：http://www.e930bbs.com                                 
**--------------File Info-----------------------------------------------------------------------------
** File          name:my_ble_uarts.h
** Last modified Date:          
** Last       Version:		   
** Descriptions      :串口透传服务头文件			
**---------------------------------------------------------------------------------------------------*/
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

//定义串口透传服务实例，该实例完成2件事情
//1：定义了static类型串口透传服务结构体变量，为串口透传服务结构体分配了内存
//2：注册了BLE事件监视者，这使得串口透传程序模块可以接收BLE协议栈的事件，从而可以在ble_uarts_on_ble_evt()事件回调函数中处理自己感兴趣的事件
#define BLE_UARTS_DEF(_name, _uarts_max_clients)                                      \
    BLE_LINK_CTX_MANAGER_DEF(CONCAT_2(_name, _link_ctx_storage),  \
                             (_uarts_max_clients),                  \
                             sizeof(ble_uarts_client_context_t));   \
	  static ble_uarts_t _name =                                     \
	  {                                                             \
        .p_link_ctx_storage = &CONCAT_2(_name, _link_ctx_storage) \
    };                                                            \
    NRF_SDH_BLE_OBSERVER(_name ## _obs,                           \
                         BLE_NUS_BLE_OBSERVER_PRIO,               \
                         ble_uarts_on_ble_evt,                    \
                         &_name)

//定义串口透传服务128位UUID基数
#define UARTS_BASE_UUID      {{0x40, 0xE3, 0x4A, 0x1D, 0xC2, 0x5F, 0xB0, 0x9C, 0xB7, 0x47, 0xE6, 0x43, 0x00, 0x00, 0x53, 0x86}} 
//定义服务和特征的16位UUID
#define BLE_UUID_UARTS_SERVICE 0x000A              //串口透传服务16位UUID
#define BLE_UUID_UARTS_TX_CHARACTERISTIC 0x000B    //TX特征16位UUID           
#define BLE_UUID_UARTS_RX_CHARACTERISTIC 0x000C    //RX特征16位UUID

//定义串口透传事件类型，这是用户自己定义的，供应用程序使用
typedef enum
{
    BLE_UARTS_EVT_RX_DATA,      //接收到新的数据
    BLE_UARTS_EVT_TX_RDY,       //准备就绪，可以发送新数据
	  BLE_NUS_EVT_COMM_STARTED,   //通知已经使能
    BLE_NUS_EVT_COMM_STOPPED,   //通知已经禁止
} ble_uarts_evt_type_t;	
	
/* Forward declaration of the ble_nus_t type. */
typedef struct ble_uarts_s ble_uarts_t;


//串口透传服务BLE_NUS_EVT_RX_DATA事件数据结构体，该结构体用于当BLE_NUS_EVT_RX_DATA产生时将接收的数据信息传递给处理函数
typedef struct
{
    uint8_t const * p_data; //指向存放接收数据的缓存
    uint16_t        length; //接收的数据长度
} ble_uarts_evt_rx_data_t;

//记录对端设备是否使能了RX特征的通知
typedef struct
{
    bool is_notification_enabled; 
} ble_uarts_client_context_t;

//串口透传服务事件结构体
typedef struct
{
    ble_uarts_evt_type_t       type;        //事件类型
    ble_uarts_t                * p_uarts;   //指向串口透传实例的指针
    uint16_t                   conn_handle; //连接句柄
    ble_uarts_client_context_t * p_link_ctx;//指向link context
    union
    {
        ble_uarts_evt_rx_data_t rx_data; //BLE_NUS_EVT_RX_DATA事件数据
    } params;
} ble_uarts_evt_t;



//定义操作码长度
#define OPCODE_LENGTH        1
//定义句柄长度
#define HANDLE_LENGTH        2

//定义最大传输数据长度（字节数）
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_UARTS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_UARTS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif

//定义函数指针类型ble_uarts_data_handler_t
typedef void (* ble_uarts_data_handler_t) (ble_uarts_evt_t * p_evt);


//串口服务初始化结构体
typedef struct
{
    ble_uarts_data_handler_t data_handler; //处理接收数据的事件句柄
} ble_uarts_init_t;


//串口透传服务结构体，包含所需要的信息
struct ble_uarts_s
{
    uint8_t                         uuid_type;          //UUID类型
    uint16_t                        service_handle;     //串口透传服务句柄（由协议栈提供）
    ble_gatts_char_handles_t        tx_handles;         //TX特征句柄
    ble_gatts_char_handles_t        rx_handles;         //RX特征句柄
	  blcm_link_ctx_storage_t * const p_link_ctx_storage; //指向存储所有当前连接及其context的“link context”的句柄的指针
    ble_uarts_data_handler_t        data_handler;       //处理接收数据的事件句柄
};




void ble_uarts_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_uarts_init(ble_uarts_t * p_uarts, ble_uarts_init_t const * p_uarts_init);
uint32_t ble_uarts_data_send(ble_uarts_t * p_uarts,
                           uint8_t   * p_data,
                           uint16_t  * p_length,
                           uint16_t    conn_handle);



#ifdef __cplusplus
}
#endif

#endif // BLE_MY_UARTS_H__


