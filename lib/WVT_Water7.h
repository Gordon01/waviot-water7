﻿#ifndef WVT_WATER7_H_
#define WVT_WATER7_H_
#pragma once

#include <stdint.h>

/* ОБЩИЕ СТАНДАРТИЗИРОВАННЫЕ КОНСТАНТЫ ПРОТОКОЛА WATER7 */

/** @brief	This flag will be added to message type to indicate 
 **			error in request */
#define WVT_W7_ERROR_FLAG					0x40
#define WVT_W7_REGULAR_MESSAGE_FLAG			0x80

#define WVT_W7_ERROR_RESPONCE_LENGTH		2UL
#define WVT_W7_READ_MULTIPLE_LENGTH 	   	5UL
#define WVT_W7_READ_SINGLE_LENGTH   	   	3UL
#define WVT_W7_WRITE_SINGLE_LENGTH   	   	7UL

#define WVT_W7_REGULAR_BUF_SIZE			    128	/*!< regular buffer size */
#define WVT_W7_CALLBACK_BUF_SIZE		    128	/*!< callbak buffer size */
#define WVT_W7_PAR_LENGTH				    320	/*!< parameters array length (must be equal or bigger than sizeof(_AQUA_TAGS_TO_SAVE) / 4 and request length) */
#define WVT_W7_PARAMETER_WIDTH              4   /*!< Число байт, выделенное под храниние параметра */
#define WVT_W7_MULTI_DATA_OFFSET            5   /*!< Начало данных в пакетах с несколькими параметрами */
#define WVT_W7_SINGLE_DATA_OFFSET           3   /*!< Начало данных в пакетах с одним параметром */

/** @brief	The following enum define error codes to 
 **			specify type of error */
typedef enum
{
    WVT_W7_ERROR_CODE_OK				= 0x00,
    WVT_W7_ERROR_CODE_INVALID_TYPE		= 0x01,
    WVT_W7_ERROR_CODE_INVALID_ADDRESS	= 0x02,
    WVT_W7_ERROR_CODE_INVALID_VALUE		= 0x03,
    WVT_W7_ERROR_CODE_LL_ERROR			= 0x04,
    WVT_W7_ERROR_CODE_READ_ONLY		    = 0x05,
    WVT_W7_ERROR_CODE_INVALID_LENGTH    = 0x06
} WVT_W7_Error_t;


/** @brief	Values that represent water7 protocol packet types */
typedef enum
{
    WVT_W7_PACKET_TYPE_READ_MULTIPLE	= 0x03,
    WVT_W7_PACKET_TYPE_WRITE_SINGLE		= 0x06,
    WVT_W7_PACKET_TYPE_READ_SINGLE		= 0x07,
    WVT_W7_PACKET_TYPE_WRITE_MULTIPLE	= 0x10,
    WVT_W7_PACKET_TYPE_ECHO				= 0x19,
    WVT_W7_PACKET_TYPE_EVENT			= 0x20,
    WVT_W7_PACKET_TYPE_CONTROL			= 0x27,
    WVT_W7_PACKET_TYPE_FW_UPDATE		= 0x29
} WVT_W7_Packet_t;

/* СПЕЦИФИЧНЫЕ ДЛЯ УСТРОЙСТВА КОНСТАНТЫ ПРОТОКОЛА WATER7 */

/** @brief	Это просто текстовые соответствия для удобной записи индексов 
 **			для массива wvt_parameters */
typedef enum 
{
    WVT_W7_IN_CMD_STATUS						= 0,

    WVT_W7_IN_CMD_RESETS_NUMBER					= 59,
    WVT_W7_IN_CMD_ENERGY_CONSUMED				= 60,
    WVT_W7_IN_CMD_BATTERY_VOLTAGE				= 61,
    WVT_W7_IN_CMD_DEVICE_TEMPERATURE			= 62,
    WVT_W7_IN_CMD_MESSAGE_COUNT					= 63,
        
    WVT_W7_IN_CMD_CONTROL_FILTER				= 64,
    WVT_W7_IN_CMD_ADDITIONAL_PARAMETERS			= 65,
    WVT_W7_IN_CMD_MESSAGE_FREQUENCY				= 66,
    
    
    WVT_W7_UNIT_TEST_PARAMETER					= 65535
} WVT_W7_Parameter_t;

/** @brief Типы событий, о которых модем может сообщать */
typedef enum
{
    WVT_W7_EVENT_RESET						= 0,		/*!< Холодная перезагрука модема */
    WVT_W7_EVENT_PING						= 1,		/*!< Проверка связи */
    WVT_W7_EVENT_CH1						= 2,		/*!< Событие канала 1 */
    WVT_W7_EVENT_CH2						= 3,		/*!< Событие канала 2 */
    WVT_W7_EVENT_NORMAL						= 60,
    WVT_W7_EVENT_TRIP						= 61,
    
} WVT_W7_Event_t;
    
typedef enum
{
    WVT_W7_OK      = 0x00U,
    WVT_W7_ERROR   = 0x01U,
    WVT_W7_BUSY    = 0x02U,
    WVT_W7_TIMEOUT = 0x03U
} WVT_W7_Status_t;

typedef enum
{
    WVT_W7_PARAMETER_READ,
    WVT_W7_PARAMETER_WRITE
} WVT_W7_Parameter_Action_t;

typedef struct 
{
    uint16_t address;
    int32_t value;
} WVT_W7_Single_Parameter_t;

typedef struct
{
    void(*nbfi_send)(uint8_t* payload, uint8_t length);						/*!< Внешняя функция отправки готового пакета */
    WVT_W7_Error_t(*rom_read)(WVT_W7_Parameter_t address, int32_t * value); /*!< Внешняя функция чтения данных из постоянной памяти */
    WVT_W7_Error_t(*rom_write)(WVT_W7_Parameter_t address, int32_t value);	/*!< Внешняя функция записи данных в постоянную память */
	WVT_W7_Error_t(*rfl_handler)(uint8_t * data, uint16_t length, 
		uint8_t * responce_buffer, uint16_t * bytes_written);               /*!< Внешняя функция удаленного обновления прошивки */
	WVT_W7_Error_t(*rfl_command)(uint8_t * data, uint16_t length, 
		uint8_t * responce_buffer, uint16_t * bytes_written);
} WVT_W7_Callbacks_t;

#ifdef __cplusplus
extern "C" {
#endif
	
	void WVT_W7_Start(int32_t resets);
	void WVT_Radio_Callback(uint8_t * data, uint16_t length);
	void WVT_W7_Register_Callbacks(WVT_W7_Callbacks_t callbacks);
	uint8_t WVT_W7_Parse(uint8_t * data, uint16_t length, uint8_t * responce_buffer);
	uint8_t WVT_W7_Short_Regular(uint8_t * responce_buffer, int32_t payload, int32_t additional_parameters);
	uint8_t WVT_W7_Event(uint16_t event, uint16_t payload, uint8_t * responce_buffer);
	uint8_t WVT_W7_Parse_Additional_Parameters(uint8_t * parameters, int32_t setting);
    
#ifdef __cplusplus
}
#endif
#endif 