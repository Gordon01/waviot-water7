#include "WVT_Water7.h"

WVT_W7_Callbacks_t externals_functions = { 0 };

WVT_W7_Error_t WVT_W7_Single_Parameter(
	uint16_t parameter_addres,
	WVT_W7_Parameter_Action_t action,
	uint8_t * responce_buffer);
uint8_t WVT_W7_Send_Normal_Regular(uint8_t * responce_buffer);
uint16_t WVT_W7_Get_Current_Reading(void);
void WVT_W7_Additional_Parameter(uint16_t address, uint8_t * data);

/**
 * @brief	Отправляет стартовый пакет, указывающий на начало работы устройства
 *			после включения питания или перезагрузки
 *
 * @param [in] 	resets		   	Число перезагрузок устройства
 */
void WVT_W7_Start(int32_t resets)
{
	uint8_t output_buffer[8];
	uint8_t output_length;
	
	output_length = WVT_W7_Uplink_Event(WVT_W7_EVENT_RESET, resets, output_buffer);
	externals_functions.nbfi_send(output_buffer, output_length);
}

/**
 * @brief	Обрабатывает входящий пакет и отсылает ответ в случае необходимости
 *			Должна вызываться при приеме downlink-пакета. 
 *						
 *
 * @param [in] 	data		   	Указатель на буфер с выходными данными.
 * @param 	   	length		   	Чило байт во входном буфере.
 */
void WVT_Radio_Callback(uint8_t * data, uint16_t length)
{
	uint8_t responce_buffer[WVT_W7_CALLBACK_BUF_SIZE];
	
	uint8_t responce_length = WVT_W7_Parse(data, length, responce_buffer);
	
	if (responce_length)
	{
		externals_functions.nbfi_send(responce_buffer, responce_length);
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
	WVT_W7_Error_t return_code;
	uint8_t responce_length;
	uint32_t addres;
	uint32_t number_of_parameters;
	
	// Должны быть переданы верные указатели на данные
	if ((data && length && responce_buffer) == 0)
	{
		return 0;	
	}
	
	WVT_W7_Packet_t packet_type = (WVT_W7_Packet_t) data[0];
	
	switch (packet_type)
	{
	case WVT_W7_PACKET_TYPE_READ_MULTIPLE:
		addres = (data[1] << 8) + data[2];
		number_of_parameters = (data[3] << 8) + data[4];
		
		if (	(length == 5)
			&&  ((5 + (number_of_parameters * 4)) <= WVT_W7_CALLBACK_BUF_SIZE) 
			&&	((addres + number_of_parameters) <= WVT_W7_PAR_LENGTH)	)
		{
			// Тип сообщения, адрес начала последовательности, длинна последовательности
			for (uint8_t i = 0; i < 5; i++)
			{
				responce_buffer[i] = data[i];
			}
			
			
			return_code = WVT_W7_Single_Parameter(data, WVT_W7_PARAMETER_READ, responce_buffer, &responce_length);
		}
		else
		{
			return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
		}
		
		if ((length == 5) && ((5 + len * 4) <= WATER7_CALLBACK_BUF_SIZE) && (addr + len <= WATER7_PAR_LENGTH))
		{
			for (uint8_t i = 0; i < 5; i++)
				tmpbuf[i] = data[i];
			for (uint8_t i = 0; i < len; i++)
				bigendian_cpy((uint8_t *)&_state.parameters_array[addr + i], (uint8_t *)&tmpbuf[5 + i * 4], 4);
			NBFi_Send(tmpbuf, 5 + len * 4);
		}

		break;
	case WVT_W7_PACKET_TYPE_WRITE_MULTIPLE:
		bigendian_cpy((uint8_t *)&data[1], (uint8_t *)&addr, 2);
		bigendian_cpy((uint8_t *)&data[3], (uint8_t *)&len, 2);
		if ((length == (len * 4 + 5)) && (addr + len <= WATER7_PAR_LENGTH))
		{
			for (uint8_t i = 0; i < len; i++)
				bigendian_cpy((uint8_t *)&data[5 + i * 4], (uint8_t *)&_state.parameters_array[addr + i], 4);
			for (uint8_t i = 0; i < 5; i++)
				tmpbuf[i] = data[i];
			NBFi_Send(tmpbuf, 5);
		}
		else
		{
			tmpbuf[0] = data[0] | CMD_ERR;
			tmpbuf[1] = 0x02;
			NBFi_Send(tmpbuf, 2);
		}
		break;
	case WVT_W7_PACKET_TYPE_READ_SINGLE:
		addres = (data[1] << 8) + data[2];
		if ((length == 3) && (addres < WVT_W7_PAR_LENGTH))
		{
			return_code = WVT_W7_Single_Parameter(data, WVT_W7_PARAMETER_READ, responce_buffer, &responce_length);
		}
		else
		{
			return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
		}
		break;
	case WVT_W7_PACKET_TYPE_WRITE_SINGLE:
		addres = (data[1] << 8) + data[2];
		if ((length == 7) && (addres < WVT_W7_PAR_LENGTH))
		{
			return_code = WVT_W7_Single_Parameter(data, WVT_W7_PARAMETER_WRITE, responce_buffer, &responce_length);
		}
		else
		{
			return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
		}
		break;
	case WVT_W7_PACKET_TYPE_FW_UPDATE:
		if (length < 2)
			break;
		NBFi_Switch_Mode(CRX);
		ScheduleTask(&water7crx_off_desc, &Water7crx_off, RELATIVE, SECONDS(WATER7_CRX_TIMEOUT));
		switch (data[1])
		{
		case RFL_CMD_WRITE_HEX:
			bigendian_cpy((uint8_t *)&data[2], (uint8_t *)&addr32, 4);
			bigendian_cpy((uint8_t *)&data[6], (uint8_t *)&len, 2);
			if (_rfl)
				_rfl(addr32, len, 0, &data[8], data[1]);
			for (uint8_t i = 0; i < 8; i++)
				tmpbuf[i] = data[i];
			NBFi_Send(tmpbuf, 8);
			break;
		case RFL_CMD_READ_HEX:
			bigendian_cpy((uint8_t *)&data[2], (uint8_t *)&addr32, 4);
			bigendian_cpy((uint8_t *)&data[6], (uint8_t *)&len, 2);
			if (len > WATER7_CALLBACK_BUF_SIZE - 8)
			{
				tmpbuf[0] = data[0] | CMD_ERR;
				tmpbuf[1] = 0x02;
				NBFi_Send(tmpbuf, 2);
			}
			else
			{
				for (uint8_t i = 0; i < 8; i++)
					tmpbuf[i] = data[i];
				if (_rfl)
					_rfl(addr32, len, 0, &tmpbuf[8], data[1]);
				NBFi_Send(tmpbuf, len + 8);
			}
			break;
		case RFL_CMD_WRITE_HEX_INDEX:
			bigendian_cpy((uint8_t *)&data[2], (uint8_t *)&addr32, 4);
			bigendian_cpy((uint8_t *)&data[6], (uint8_t *)&len, 2);
			bigendian_cpy((uint8_t *)&data[8], (uint8_t *)&index, 2);
			if (_rfl)
				ret = _rfl(addr32, len, index, &data[10], data[1]);
			if (ret >= 0)
			{
				memcpy(tmpbuf, data, 2);
				bigendian_cpy((uint8_t *) &ret, &tmpbuf[2], 4);
				NBFi_Send(tmpbuf, 6);
			}
			break;
		case RFL_CMD_GET_CRC:
			if (length != 10)
				break;
			bigendian_cpy((uint8_t *)&data[2], (uint8_t *)&addr32, 4);
			bigendian_cpy((uint8_t *)&data[6], (uint8_t *)&tmp32, 4);
			if (_rfl)
				ret = _rfl(addr32, tmp32, 0, 0, data[1]);
			memcpy(tmpbuf, data, 2);
			bigendian_cpy((uint8_t *) &ret, &tmpbuf[2], 4);
			NBFi_Send(tmpbuf, 6);
			break;
		case RFL_CMD_EXEC_PATCH0:
		case RFL_CMD_EXEC_PATCH1:
		case RFL_CMD_EXEC_PATCH2:
			bigendian_cpy((uint8_t *)&data[2], (uint8_t *)&addr32, 4);
			if (_rfl)
				ret = _rfl(addr32, 0, 0, 0, data[1]);
			bigendian_cpy((uint8_t *) &ret, &tmpbuf[2], 4);
			NBFi_Send(tmpbuf, 6);
			break;
		case RFL_CMD_CLEAR_INDEX:
		case RFL_CMD_CHECK_UPDATE:
		case RFL_CMD_CLEAR_CACHE:
		case RFL_CMD_CPY_ACTUAL:
		case RFL_CMD_SOFT_RESET:
		case RFL_CMD_MASS_ERASE:
		case RFL_CMD_GET_INDEX:
		case RFL_CMD_GET_VERSION:
		default:
			if (_rfl)
				ret = _rfl(0, 0, 0, 0, data[1]);
			memcpy(tmpbuf, data, 2);
			bigendian_cpy((uint8_t *) &ret, &tmpbuf[2], 4);
			NBFi_Send(tmpbuf, 6);
			break;
		}
	case WVT_W7_PACKET_TYPE_CONTROL:
		if (length != 7)
			break;
		bigendian_cpy((uint8_t *)&data[1], (uint8_t *)&cmd, 2);
		bigendian_cpy((uint8_t *)&data[3], (uint8_t *)&tmp32, 4);

		switch (cmd)
		{
		case CTRL_SETFASTDL:
			Water7fastdl_on();
			memcpy(tmpbuf, data, length);
			NBFi_Send(tmpbuf, length);
			break;
		case CTRL_RESETFASTDL:
			wtimer0_remove(&water7fastdl_off_desc);
			Water7fastdl_off(0);
			memcpy(tmpbuf, data, length);
			NBFi_Send(tmpbuf, length);
			break;
		case CTRL_RESET:
			if (_rfl)
				_rfl(0, 0, 0, 0, RFL_CMD_SOFT_RESET);
			memcpy(tmpbuf, data, length);
			NBFi_Send(tmpbuf, length);
			break;
		case CTRL_SAVE:
			if (_save_data)
				_save_data((uint8_t *)_state.parameters_array);
			memcpy(tmpbuf, data, length);
			NBFi_Send(tmpbuf, length);
			break;
		default:
			tmpbuf[0] = data[0];
			tmpbuf[1] = (uint8_t)CTRL_NOCTRL;
			tmpbuf[2] = (uint8_t)CTRL_NOCTRL;
			tmpbuf[3] = tmpbuf[4] = 0;
			tmpbuf[5] = data[3];
			tmpbuf[6] = data[4];
			NBFi_Send(tmpbuf, length);
			break;
		}
		break;
	default:
		return_code = HAL_ERROR;
		responce_buffer[1] = WVT_W7_ERROR_CODE_INVALID_TYPE;
		responce_length = WVT_W7_ERROR_RESPONCE_LENGTH;
		break;
	}
	
	if (return_code == WVT_W7_ERROR_CODE_OK)
	{
		responce_buffer[0] = packet_type;
		return responce_length;
	}
	else
	{
		responce_buffer[0] = (packet_type + WVT_W7_ERROR_FLAG);
		responce_buffer[1] = return_code;
		return WVT_W7_ERROR_RESPONCE_LENGTH;
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
WVT_W7_Error_t WVT_W7_Single_Parameter(
	uint16_t parameter_addres,
	WVT_W7_Parameter_Action_t action,
	uint8_t * responce_buffer)
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
	
	externals_functions.nbfi_send(output_buffer, output_length);
}	

void WVT_W7_Send_Ext_Info(uint8_t channel)
{
	uint8_t SendBuf[8];

	externals_functions.nbfi_send(SendBuf, 8);
}