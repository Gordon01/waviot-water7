#include <TinyEmbeddedTest.h>
#include <stdint.h>

#include "WVT_Water7.h"
#include "WVT_EEPROM.h"

/* Правило для используемых в тесте параметров, значение которых изменяется
 * во время его работы:
 * - Переменные типа WVT_W7_Parameter_t должны быть определены ниже
 * - Изначальные значения должны быть сохранены в setup() и восстановлены в teardown()
 * - Использование WVT_ROM_Save_Parameter() с аргументом-именованной константой не допускается	
 */

const WVT_W7_Parameter_t additional_parameters = WVT_W7_IN_CMD_ADDITIONAL_PARAMETERS;
const WVT_W7_Parameter_t message_frequency = WVT_W7_IN_CMD_MESSAGE_FREQUENCY;

TEST_GROUP(WATER7)
{
	int32_t saved_additional_parameters;
	int32_t saved_message_frequency;
	
    void setup()
    {
	    WVT_ROM_Read_Parameter(additional_parameters, &saved_additional_parameters);
	    WVT_ROM_Read_Parameter(message_frequency, &saved_message_frequency);
    }
    
    void teardown()
    {
	    WVT_ROM_Save_Parameter(additional_parameters, saved_additional_parameters);
	    WVT_ROM_Save_Parameter(message_frequency, saved_message_frequency);
    }
	
	void TestSetup(TestInstance *)
	{
		asm("nop");
	}
	
	void TestTeardown(TestInstance *)
	{
		asm("nop");
	}
};

TEST(WATER7, NormalRead)
{
	uint8_t responce_buffer[8];
	uint8_t responce_length;
	uint8_t input_data[3] = { WVT_W7_PACKET_TYPE_READ_SINGLE, 0, WVT_W7_IN_CMD_RESETS_NUMBER };
	
	responce_length = WVT_W7_Parse(input_data, 3, responce_buffer);
	
	CHECK_EQUAL(7, responce_length);
	MEMCMP_EQUAL(input_data, responce_buffer, 3);
}

TEST(WATER7, IllegalRead)
{
	uint8_t responce_buffer[8];
	uint8_t responce_length;
	uint8_t input_data[3] = { WVT_W7_PACKET_TYPE_READ_SINGLE, 0xFD, 0xE4 };
	uint8_t output_data[2] = { (WVT_W7_PACKET_TYPE_READ_SINGLE | WVT_W7_ERROR_FLAG), 
		WVT_W7_ERROR_CODE_INVALID_ADDRESS};
	
	responce_length = WVT_W7_Parse(input_data, 3, responce_buffer);
	
	CHECK_EQUAL(2, responce_length);
	MEMCMP_EQUAL(output_data, responce_buffer, 2);
}

TEST(WATER7, AdditionalParameters)
{
	const int32_t no_additional_parameters = 0;
	const int32_t one_additional_parameters = 31;
	const int32_t five_additional_parameters = 0x10410410;
	const uint8_t five_parameters[5] = { 0x10, 0x10, 0x10, 0x10, 0x10 };
	uint8_t read_parameters[5];
	uint8_t read_num_of_parameters;
	
	WVT_ROM_Save_Parameter(additional_parameters, no_additional_parameters);
	read_num_of_parameters = WVT_W7_Get_Additional_Parameters(read_parameters);
	CHECK_EQUAL(0, read_num_of_parameters);
	
	WVT_ROM_Save_Parameter(additional_parameters, one_additional_parameters);
	read_num_of_parameters = WVT_W7_Get_Additional_Parameters(read_parameters);
	CHECK_EQUAL(1, read_num_of_parameters);
	
	WVT_ROM_Save_Parameter(additional_parameters, five_additional_parameters);
	read_num_of_parameters = WVT_W7_Get_Additional_Parameters(read_parameters);
	CHECK_EQUAL(5, read_num_of_parameters);
	MEMCMP_EQUAL(five_parameters, read_parameters, 5);
}

static uint32_t CheckTriggerCount()
{
	uint32_t trigger_count = 0;
	
	for (int hour = 0; hour < 24; hour++)
	{
		for (int minute = 0; minute < 120; minute++)
		{
			if (WVT_W7_Perform_Regular(hour, (minute / 2)))
			{
				trigger_count++;
			}	
		}
	}
	
	return trigger_count;
}

TEST(WATER7, MessageFrequencyEveryHour)
{
	const int32_t every_hour = 24;
	uint8_t result;
	
	WVT_ROM_Save_Parameter(message_frequency, every_hour);
	uint32_t trigger_count = CheckTriggerCount();

	CHECK_EQUAL(every_hour, trigger_count);
}


TEST(WATER7, MessageFrequencyTwiceADay)
{
	const int32_t twice_a_day = 2;
	
	WVT_ROM_Save_Parameter(message_frequency, twice_a_day);
	uint32_t trigger_count = CheckTriggerCount();

	CHECK_EQUAL(twice_a_day, trigger_count);
}

TEST(WATER7, MessageFrequencyTwiceAHour)
{
	const int32_t twice_a_hour = 48;
	uint8_t result;
	
	WVT_ROM_Save_Parameter(message_frequency, twice_a_hour);
	uint32_t trigger_count = CheckTriggerCount();

	CHECK_EQUAL(twice_a_hour, trigger_count);
}