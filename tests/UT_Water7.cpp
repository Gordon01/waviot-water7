#define CATCH_CONFIG_MAIN
#include <stdint.h>
#include "../lib/WVT_Water7.h"
#include "catch.hpp"

uint8_t read_buffer[500];

TEST_CASE("Reset", "[reset]")
{
	//WVT_W7_Start()
}

TEST_CASE("Additional parameters", "[additional_parameters]")
{
	const int32_t no_additional_parameters = 0;
	const int32_t one_additional_parameters = 31;
	const int32_t five_additional_parameters = 0x10410410;
	const uint8_t five_parameters[5] = { 0x10, 0x10, 0x10, 0x10, 0x10 };
	
	CHECK(WVT_W7_Parse_Additional_Parameters(read_buffer, no_additional_parameters) == 0);
	
	CHECK(WVT_W7_Parse_Additional_Parameters(read_buffer, one_additional_parameters) == 1);
	
	CHECK(WVT_W7_Parse_Additional_Parameters(read_buffer, five_additional_parameters) == 5);
	CHECK(memcmp(five_parameters, read_buffer, 5) == 0);
}