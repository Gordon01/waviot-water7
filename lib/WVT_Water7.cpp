#include <stm32l0xx_hal.h>
#include <map>

#include "nbfi.h"
#include "nbfi_defines.h"

#include "WVT_Radio.h"
#include "WVT_Water7.h"
#include "WVT_EEPROM.h"
#include "WVT_ADC.h"
#include "WVT_Hall.h"

extern nbfi_state_t nbfi_state;
uint8_t wvt_w7_can_sleep = 1;

HAL_StatusTypeDef WVT_W7_Single_Parameter(WVT_W7_Single_Parameter_t parameter, 
	WVT_W7_Parameter_Action_t action,
	uint8_t * responce_buffer, 
	uint8_t * responce_length);
uint8_t WVT_W7_Value_Event(WVT_W7_Switch_Event_t event, uint32_t current, uint8_t * responce_buffer);
uint8_t WVT_W7_Send_Normal_Regular(uint8_t * responce_buffer);
uint16_t WVT_W7_Get_Current_Reading(void);
void WVT_W7_Additional_Parameter(uint16_t address, uint8_t * data);

std::map<WVT_W7_Packet_t, uint8_t> wvt_packet_lengths = { 
	{ WVT_W7_PACKET_TYPE_READ_SINGLE, 3 },
	{ WVT_W7_PACKET_TYPE_WRITE_SINGLE, 7 },
};

void WVT_W7_Init()
{
	uint8_t output_buffer[8];
	uint8_t output_length;
	
	int32_t resets;
	WVT_ROM_Read_Parameter(WVT_W7_IN_CMD_RESETS_NUMBER, &resets);
	
	output_length = WVT_W7_Uplink_Event(WVT_W7_EVENT_RESET, resets, output_buffer);
	NBFi_Send(output_buffer, output_length);
}

uint8_t WVT_W7_Can_Sleep(void)
{
	return wvt_w7_can_sleep;
}

void WVT_Radio_Callback(uint8_t * data, uint16_t length)
{
	uint8_t responce_buffer[16];
	
	uint8_t responce_length = WVT_W7_Parse(data, length, responce_buffer);
	
	if (responce_length)
	{
		NBFi_Send(responce_buffer, responce_length);
	}
}

/**
 * @brief	Обрабатывает входящий NB-Fi пакет
 *
 * @param [in] 	data		   	Указатель на буфер с выходными данными.
 * @param 	   	length		   	Чило байт во входном буфере.
 * @param [out]	responce_buffer	Указатель на буфер с выходными данными.
 *
 * @returns	Число зачисанных байт в буфер с выходными данными.
 */
inline uint8_t WVT_W7_Parse(uint8_t * data, uint16_t length, uint8_t * responce_buffer)
{
	WVT_W7_Packet_t packet_type = (WVT_W7_Packet_t) data[0];
	WVT_W7_Single_Parameter_t parameter;
	HAL_StatusTypeDef return_code;
	uint8_t responce_length;
	
	if (	(wvt_packet_lengths.find(packet_type) != wvt_packet_lengths.end())
		&&	(wvt_packet_lengths[packet_type] == length) )
	{
		// У пакета верная длинна
		switch (packet_type)
		{
		case WVT_W7_PACKET_TYPE_WRITE_SINGLE:
			parameter.address = (data[1] << 8) + data[2];
			parameter.value = (data[3] << 24) + (data[4] << 16) + (data[5] << 8) + (data[6]);
			return_code = WVT_W7_Single_Parameter(parameter,
				WVT_W7_PARAMETER_WRITE,
				responce_buffer,
				&responce_length);
			break;
		case WVT_W7_PACKET_TYPE_READ_SINGLE:
			parameter.address = (data[1] << 8) + data[2];
			//parameter.value = (data[3] << 24) + (data[4] << 16) + (data[5] << 8) + (data[6]);
			return_code = WVT_W7_Single_Parameter(parameter,
				WVT_W7_PARAMETER_READ,
				responce_buffer,
				&responce_length);
			break;
		default:
			return_code = HAL_ERROR;
			responce_buffer[1] = WVT_W7_ERROR_CODE_INVALID_TYPE;
			responce_length = 2;
			break;
		}
	}
	else
	{
		return_code = HAL_ERROR;
		responce_buffer[1] = WVT_W7_ERROR_CODE_INVALID_LENGTH;
		responce_length = 2;
	}
	
	if (return_code == HAL_OK)
	{
		responce_buffer[0] = packet_type;
		return responce_length;
	}
	else
	{
		responce_buffer[0] = (packet_type + WVT_W7_ERROR_FLAG);
		return responce_length;
	}
}

/**
 * @brief	Writes single parameter to EEPROM and returns NB-Fi packet with responce
 *
 * @param 	   	parameter	   	Parameter structure (address and value)
 * @param [out]	responce_buffer	Output buffer with NB-Fi packet.
 * @param [out]	responce_length	Length of the responce.
 *
 * @returns	- HAL_OK		Parameter successefully written to ROM
 * 			- HAL_ERROR		Error. See 2nd byte in NB-Fi packet.
 */
HAL_StatusTypeDef WVT_W7_Single_Parameter(WVT_W7_Single_Parameter_t parameter, 
	WVT_W7_Parameter_Action_t action,
	uint8_t * responce_buffer, 
	uint8_t * responce_length)
{
	if (responce_buffer == NULL)
	{
		responce_length = 0;
		return HAL_ERROR;
	}
	
	HAL_StatusTypeDef ret;
	WVT_W7_Error_t error;
	
	switch (action)
	{
	case WVT_W7_PARAMETER_READ:
		error = WVT_ROM_Read_Parameter((WVT_W7_Parameter_t) parameter.address, &parameter.value);
		break;
	case WVT_W7_PARAMETER_WRITE:
		// Пытаемся сохранить параметр в EEPROM
		error = WVT_ROM_Save_Parameter((WVT_W7_Parameter_t) parameter.address, parameter.value);
		break;
	}	
	
	if (error == WVT_W7_ERROR_CODE_OK)
	{
		responce_buffer[1] = (parameter.address >> 8);
		responce_buffer[2] =  parameter.address;	
		responce_buffer[3] = (parameter.value >> 24);
		responce_buffer[4] = (parameter.value >> 16);
		responce_buffer[5] = (parameter.value >> 8);
		responce_buffer[6] =  parameter.value;			
		
		ret = HAL_OK;
		*responce_length = 7;
	}
	else
	{
		responce_buffer[1] = error;
		
		ret = HAL_ERROR;
		*responce_length = 2;
	}
	
	return ret;
}

/**
 * @brief	Отправляет uplink-сообщение о событии в модеме
 *
 * @param 	   	event		   	Тип события.
 * @param 	   	payload		   	Полезная нагрузка события
 * @param [out]	responce_buffer	Выходной буфер с сообщением NB-Fi.
 *
 * @returns	Число байт, записанных в выходной буфер.
 */
uint8_t WVT_W7_Uplink_Event(WVT_W7_Event_t event, uint16_t payload, uint8_t * responce_buffer)
{
	responce_buffer[0] = WVT_W7_PACKET_TYPE_EVENT;
	responce_buffer[1] = (event >> 8);
	responce_buffer[2] =  event;
	responce_buffer[3] = (payload >> 8);
	responce_buffer[4] =  payload;
	
	return 5;
}

uint8_t WVT_W7_Get_Additional_Parameters(uint8_t * parameters)
{
	int32_t additional_parameters;
	uint8_t parameter_number = 0;
	uint8_t current_parameter;
	const uint8_t parameter_mask = 0b00111111;
	
	WVT_ROM_Read_Parameter(WVT_W7_IN_CMD_ADDITIONAL_PARAMETERS, &additional_parameters);
	
	while (additional_parameters & parameter_mask)
	{
		parameters[parameter_number] = additional_parameters & parameter_mask; 
		parameter_number++;
		additional_parameters = (additional_parameters >> 6);
	}
	
	return parameter_number;
}

uint8_t WVT_W7_Send_Normal_Regular(uint8_t * responce_buffer)
{
	const uint32_t fifo = WVT_FIFO_Get();
	int32_t frequency;
	uint8_t parameters[5];
	
	WVT_ROM_Read_Parameter(WVT_W7_IN_CMD_MESSAGE_FREQUENCY, &frequency);
	uint8_t number_of_additional_params = WVT_W7_Get_Additional_Parameters(parameters);
	
	const uint8_t hours = (frequency >> 8);
	
	responce_buffer[0] = (WVT_W7_REGULAR_MESSAGE_FLAG);
	responce_buffer[1] = 0;	// периодичность отправки в днях всегда 0
	responce_buffer[2] = hours;
	
	responce_buffer[3] = (fifo >> 24);
	responce_buffer[4] = (fifo >> 16);
	responce_buffer[5] = (fifo >> 8);
	responce_buffer[6] = (fifo);
	
	for (uint8_t i = 0; i < number_of_additional_params; i++)
	{
		WVT_W7_Additional_Parameter(parameters[i], ((responce_buffer + 7) + (i * 5)));
	}
	
	return (7 + (5 * number_of_additional_params));
}

/**
 * @brief		Формирует массив, готовый, для "приклеивания" к регулярному сообщению
 *
 * @param 	   	address		Адрес параметра
 * @param [out]	data		Указатель на буфер с выходными данными
 */
void WVT_W7_Additional_Parameter(uint16_t address, uint8_t * data)
{
	int32_t parameter_value;
	WVT_ROM_Read_Parameter((WVT_W7_Parameter_t) address, &parameter_value) ;
	
	data[0] = address;
	data[1] = (parameter_value >> 24);
	data[2] = (parameter_value >> 16);
	data[3] = (parameter_value >> 8);
	data[4] = (parameter_value);
}

void WVT_Select_NBFi_mode(GPIO_PinState vext)
{
	static uint8_t swithcing_minute = 0;
	nbfi_mode_t new_mode;
	extern RTC_HandleTypeDef wvt_hrtc;
	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 }; 
	
	HAL_RTC_GetTime(&wvt_hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&wvt_hrtc, &sDate, RTC_FORMAT_BIN);
	
	if (vext == GPIO_PIN_SET) 
	{
		new_mode = CRX;
	}
	else
	{
		new_mode = DRX;
	}
	
	if (swithcing_minute != sTime.Minutes)
	{
		if (NBFi_Switch_Mode(new_mode))
		{
			swithcing_minute = sTime.Minutes;
		}
	}
}

void WVT_W7_Per_Second()
{
	WVT_Alarms_t alarms = { WVT_W7_SWITCH_NORMAL };
	uint8_t output_buffer[5];
	
	extern RTC_HandleTypeDef wvt_hrtc;
	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 }; 
	
	HAL_RTC_GetTime(&wvt_hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&wvt_hrtc, &sDate, RTC_FORMAT_BIN);
	
	if (WVT_W7_Perform_Regular(sTime.Hours, sTime.Minutes))
	{
		WVT_W7_Send_Regular();
	}
	WVT_Alarm_Service(&alarms);
	
	if (alarms.ch1_event)
	{
		uint8_t output_length = WVT_W7_Uplink_Event(WVT_W7_EVENT_CH1, alarms.ch1_state, output_buffer);
		NBFi_Send(output_buffer, output_length);
	}
	
	if (alarms.ch2_event)
	{
		uint8_t output_length = WVT_W7_Uplink_Event(WVT_W7_EVENT_CH2, alarms.ch2_state, output_buffer);
		NBFi_Send(output_buffer, output_length);
	}
		
	if (alarms.events)
	{
		WVT_W7_Send_Regular();
	}
}

uint8_t WVT_W7_Perform_Regular(uint8_t sHours, uint8_t sMinutes)
{	
	static uint8_t triggered = 0;
	int32_t message_frequency;
	uint8_t ret = 0;

	WVT_ROM_Read_Parameter(WVT_W7_IN_CMD_MESSAGE_FREQUENCY, &message_frequency);
	
	// Частота отправки равна числу минут в день, деленных на необходимое число сообщений
	message_frequency = (60 * 24) / message_frequency;
	
	const uint32_t minutes_since_beginning = (sHours * 60) + sMinutes;
	
	const uint8_t time_has_come = (minutes_since_beginning % message_frequency) == 0;
	
	if (time_has_come)
	{
		if (triggered == 0)
		{
			ret = 1;
			triggered++;
		}
	}
	else
	{
		triggered = 0;
	}
	
	return ret;
}

/** @brief	Здесь должны отправляться ежечасные (или более редкие) пакеты
 **			с текущими показаниями. */
inline void WVT_W7_Send_Regular()
{
	uint8_t output_buffer[16];
	
	uint8_t output_length = WVT_W7_Send_Normal_Regular(output_buffer);
	
	NBFi_Send(output_buffer, output_length);
}	

void WVT_W7_Send_Ext_Info(uint8_t channel)
{
	uint8_t SendBuf[8];

	NBFi_Send(SendBuf, 8);
}