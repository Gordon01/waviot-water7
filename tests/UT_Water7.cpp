#ifdef SYSPROGS_TEST_PLATFORM_EMBEDDED
#include <TinyEmbeddedTest.h>
#endif

#include <stdint.h>
#include "lib/WVT_Water7.h"

/* Правило для используемых в тесте параметров, значение которых изменяется
 * во время его работы:
 * - Переменные типа WVT_W7_Parameter_t должны быть определены ниже
 * - Изначальные значения должны быть сохранены в setup() и восстановлены в teardown()
 * - Использование WVT_ROM_Save_Parameter() с аргументом-именованной константой не допускается	
 */

const uint16_t additional_parameters = WVT_W7_IN_CMD_ADDITIONAL_PARAMETERS;
const uint16_t message_frequency = WVT_W7_IN_CMD_MESSAGE_FREQUENCY;

TEST_GROUP(WATER7)
{
	int32_t saved_additional_parameters;
	int32_t saved_message_frequency;
	
    void setup()
    {
	    asm("nop");
    }
    
    void teardown()
    {
	    asm("nop");
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
	
	read_num_of_parameters = WVT_W7_Parse_Additional_Parameters(read_parameters, no_additional_parameters);
	CHECK_EQUAL(0, read_num_of_parameters);
	
	read_num_of_parameters = WVT_W7_Parse_Additional_Parameters(read_parameters, one_additional_parameters);
	CHECK_EQUAL(1, read_num_of_parameters);
	
	read_num_of_parameters = WVT_W7_Parse_Additional_Parameters(read_parameters, five_additional_parameters);
	CHECK_EQUAL(5, read_num_of_parameters);
	MEMCMP_EQUAL(five_parameters, read_parameters, 5);
}