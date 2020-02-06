#include "WVT_Water7.h"

WVT_W7_Callbacks_t externals_functions = { 0 };

WVT_W7_Error_t WVT_W7_Single_Parameter(
    uint16_t parameter_addres,
	WVT_W7_Parameter_Action_t action,
	uint8_t * responce_buffer);
static uint8_t WVT_W7_Parse(uint8_t * data, uint16_t length, uint8_t * responce_buffer);

/**
 * @brief	Отправляет стартовый пакет, указывающий на начало работы устройства
 *			после включения питания или перезагрузки
 *
 * @param   resets		   	Число перезагрузок устройства
 */
void WVT_W7_Start(int32_t resets)
{
    uint8_t output_buffer[8];
    uint8_t output_length;
    
	output_length = WVT_W7_Event(WVT_W7_EVENT_RESET, resets, output_buffer);
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
static uint8_t WVT_W7_Parse(uint8_t * data, uint16_t length, uint8_t * responce_buffer)
{
    WVT_W7_Error_t return_code = WVT_W7_ERROR_CODE_OK;
    uint16_t responce_length;
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
        
	    if (    (length == WVT_W7_READ_MULTIPLE_LENGTH)
            &&  ((WVT_W7_MULTI_DATA_OFFSET + (number_of_parameters * WVT_W7_PARAMETER_WIDTH)) <= WVT_W7_CALLBACK_BUF_SIZE) 
            &&	((addres + number_of_parameters) <= WVT_W7_PAR_LENGTH)	)
        {
            // Тип сообщения, адрес начала последовательности и длинна последовательности
	        // заполняются из входящего пакета
            for (uint8_t i = 0 ; i < WVT_W7_MULTI_DATA_OFFSET ; i++)
            {
                responce_buffer[i] = data[i];
            }
            
            uint16_t current_parameter = 0;
            while (	(return_code == WVT_W7_ERROR_CODE_OK)
                &&	(current_parameter <= number_of_parameters)	)
            {
                return_code = WVT_W7_Single_Parameter((addres + current_parameter), WVT_W7_PARAMETER_READ, 
		            (responce_buffer + WVT_W7_MULTI_DATA_OFFSET + (current_parameter * WVT_W7_PARAMETER_WIDTH)));
                current_parameter++;
            }
	        responce_length = WVT_W7_READ_MULTIPLE_LENGTH + (number_of_parameters * WVT_W7_PARAMETER_WIDTH);
        }
        else
        {
            return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
        }
        break;
    case WVT_W7_PACKET_TYPE_WRITE_MULTIPLE:
	    addres = (data[1] << 8) + data[2];
	    number_of_parameters = (data[3] << 8) + data[4];
        
	    if (    (length == ((number_of_parameters * WVT_W7_PARAMETER_WIDTH) + WVT_W7_MULTI_DATA_OFFSET)) 
		    &&  (addres + number_of_parameters <= WVT_W7_PAR_LENGTH)   )
	    {
		    // Тип сообщения, адрес начала последовательности и длинна последовательности
			// заполняются из входящего пакета
			for (uint8_t i = 0 ; i < WVT_W7_MULTI_DATA_OFFSET ; i++)
		    {
			    responce_buffer[i] = data[i];
		    }
            
		    uint16_t current_parameter = 0;
		    while (     (return_code == WVT_W7_ERROR_CODE_OK)
		            &&	(current_parameter <= number_of_parameters) )
		    {
			    return_code = WVT_W7_Single_Parameter((addres + current_parameter),
				    WVT_W7_PARAMETER_WRITE, 
				    (data + WVT_W7_MULTI_DATA_OFFSET + (current_parameter * WVT_W7_PARAMETER_WIDTH)));
			    current_parameter++;
		    }
		    // Не опечатка
		    responce_length = WVT_W7_READ_MULTIPLE_LENGTH;
	    }
	    else
	    {
		    return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
	    }
        break;
    case WVT_W7_PACKET_TYPE_READ_SINGLE:
        addres = (data[1] << 8) + data[2];
	    
	    if (    (length == WVT_W7_READ_SINGLE_LENGTH) 
		     && (addres < WVT_W7_PAR_LENGTH)    )
        {
	        // Тип сообщения и адрес заполняются из входящего пакета
            for(uint8_t i = 0 ; i < WVT_W7_SINGLE_DATA_OFFSET ; i++)
	        {
		        responce_buffer[i] = data[i];
	        }
	        
	        return_code = WVT_W7_Single_Parameter(addres, 
		        WVT_W7_PARAMETER_READ,
		        (responce_buffer + WVT_W7_SINGLE_DATA_OFFSET));
	        responce_length = WVT_W7_READ_SINGLE_LENGTH + WVT_W7_PARAMETER_WIDTH;
        }
        else
        {
            return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
        }
        break;
    case WVT_W7_PACKET_TYPE_WRITE_SINGLE:
	    addres = (data[1] << 8) + data[2];
	    
	    if (    (length == WVT_W7_WRITE_SINGLE_LENGTH) 
		     && (addres < WVT_W7_PAR_LENGTH))
	    {
		    // Тип сообщения и адрес заполняются из входящего пакета
			for(uint8_t i = 0 ; i < WVT_W7_SINGLE_DATA_OFFSET ; i++)
		    {
			    responce_buffer[i] = data[i];
		    }
	        
		    return_code = WVT_W7_Single_Parameter(addres, 
			    WVT_W7_PARAMETER_WRITE,
			    (data + WVT_W7_SINGLE_DATA_OFFSET));
		    responce_length = WVT_W7_WRITE_SINGLE_LENGTH;
	    }
	    else
	    {
		    return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
	    }
        break;
    case WVT_W7_PACKET_TYPE_FW_UPDATE:
	    if (length < 2)
	    {
		    return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
		    break;
	    }
	    
	    return_code = externals_functions.rfl_handler(data, length, responce_buffer, &responce_length);
       
	    break;
    case WVT_W7_PACKET_TYPE_CONTROL:
        if (length != 7)
	    {
		    return_code = WVT_W7_ERROR_CODE_INVALID_LENGTH;
		    break;
	    }
	    
	    return_code = externals_functions.rfl_command(data, length, responce_buffer, &responce_length);
	    
        break;
    default:
	    return_code = WVT_W7_ERROR_CODE_INVALID_TYPE;
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
 * @brief	Читает или записывает один параметр из EEPROM в буфер.
 *			Для чтения и записи используется один и тот же указатель на буфер
 *			Данные размещаются начиная с нулевого смещения и занимают четыре байта
 *			Порядок байт: от старшего к младшему
 *
 * @param 	   		parameter_addres	   	Адрес параметра
 * @param 	   		action	   				Действие: чтение или запись
 * @param [in/out]	responce_buffer			Из этого буфера будут прочитанны или записаны данные
 *
 * @returns	- WVT_W7_ERROR_CODE_OK		        Параметр успешно записан в EEPROM
 *          - WVT_W7_ERROR_CODE_INVALID_ADDRESS Передан нулевой указатель
 * 			- WVT_W7_ERROR_CODE_LL_ERROR	    Произошла ошибка при записи
 */
WVT_W7_Error_t WVT_W7_Single_Parameter(
    uint16_t parameter_addres,
    WVT_W7_Parameter_Action_t action,
    uint8_t * responce_buffer)
{
    if (responce_buffer == 0)
    {
	    return WVT_W7_ERROR_CODE_INVALID_ADDRESS;
    }
    
	WVT_W7_Error_t rom_operation_result;
    int32_t value;
    
    switch (action)
    {
    case WVT_W7_PARAMETER_READ:
	    rom_operation_result = externals_functions.rom_read((WVT_W7_Parameter_t) parameter_addres, &value) ;
	    if (rom_operation_result == WVT_W7_ERROR_CODE_OK)
        {
            responce_buffer[0] = (value >> 24);
            responce_buffer[1] = (value >> 16);
            responce_buffer[2] = (value >> 8);
            responce_buffer[3] =  value;		
        }
        break;
    case WVT_W7_PARAMETER_WRITE:
        value =   (responce_buffer[0] << 24) 
                + (responce_buffer[1] << 16)
                + (responce_buffer[2] << 8) 
                +  responce_buffer[3];
	    rom_operation_result = externals_functions.rom_write((WVT_W7_Parameter_t) parameter_addres, value);
        break;
    }	
    
	return rom_operation_result;
}

/**
 * @brief	    Формирует пакет о событии
 *
 * @param 	   	event		   	Тип события.
 * @param 	   	payload		   	Полезная нагрузка события
 * @param [out]	responce_buffer	Выходной буфер с сообщением NB-Fi.
 *
 * @returns	Число байт, записанных в выходной буфер.
 */
uint8_t WVT_W7_Event(uint16_t event, uint16_t payload, uint8_t * responce_buffer)
{
    responce_buffer[0] = WVT_W7_PACKET_TYPE_EVENT;
    responce_buffer[1] = (event >> 8);
    responce_buffer[2] =  event;
    responce_buffer[3] = (payload >> 8);
    responce_buffer[4] =  payload;
    
    return 5;
}

/**
 * @brief	    Распаковывает значение настройки дополнительных параметров
 *              Возвращает число параметров, настроенных для отправки и записывает
 *              их адреса во входной массив
 *
 * @param [out]	parameters	    Массив, куда будут помещены адреса обнаруженных параметров (5 байт)
 * @param 	   	setting		   	Упакованное значение настройки (из EEPROM)
 *
 * @returns	    Число обнаруженных параметров
 */
static uint8_t WVT_W7_Parse_Additional_Parameters(uint8_t * parameters, int32_t setting)
{
    uint8_t parameter_number = 0;
    const uint8_t parameter_mask = 0b00111111;

    while (setting & parameter_mask)
    {
        parameters[parameter_number] = setting & parameter_mask; 
        parameter_number++;
        setting = (setting >> 6);
    }
    
    return parameter_number;
}

/**
 * @brief		Формирует массив, готовый, для "приклеивания" к регулярному сообщению
 *
 * @param 	   	address		Адрес параметра
 * @param [out]	data		Указатель на буфер с выходными данными
 */
static void WVT_W7_Additional_Parameter(uint16_t address, uint8_t * data)
{
    int32_t parameter_value;
    externals_functions.rom_read((WVT_W7_Parameter_t) address, &parameter_value) ;
    
    data[0] = address;
    data[1] = (parameter_value >> 24);
    data[2] = (parameter_value >> 16);
    data[3] = (parameter_value >> 8);
    data[4] = (parameter_value);
}

/**
 * @brief		Формирует короткое регулярное сообщение
 *              Если настроены дополнительные параметры, то они будут добавлены 
 *              к регулярному сообщению
 *
 * @param [out]	responce_buffer		    Указатель на буфер с выходными данными
 * @param 	   	payload		            Адрес параметра
 * @param       additional_parameters   Упакованное значение настройки (из EEPROM)
 *                                      Значение 0 отключит отправку дополнительных параметров
 * 
 * @returns	    Число записанных байт
 */
uint8_t WVT_W7_Short_Regular(uint8_t * responce_buffer, int32_t payload, int32_t additional_parameters)
{
    uint8_t parameters[5];
	uint8_t number_of_additional_params = WVT_W7_Parse_Additional_Parameters(parameters, 
		additional_parameters);
    
    responce_buffer[0] = (WVT_W7_REGULAR_MESSAGE_FLAG);
    responce_buffer[1] = 0;	// FIXME
    responce_buffer[2] = 0;
    
	responce_buffer[3] = (payload >> 24);
	responce_buffer[4] = (payload >> 16);
	responce_buffer[5] = (payload >> 8);
	responce_buffer[6] = (payload);
    
	while (number_of_additional_params--)
    {
	    WVT_W7_Additional_Parameter(parameters[number_of_additional_params], 
		    ((responce_buffer + 7) + (number_of_additional_params * 5)));
    }
    
    return (7 + (5 * number_of_additional_params));
}	