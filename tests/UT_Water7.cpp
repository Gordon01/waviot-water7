#define CATCH_CONFIG_MAIN
#include <stdint.h>
#include "../lib/WVT_Water7.h"
#include "catch.hpp"

TEST_CASE("Additional parameters", "[additional_parameters]")
{
	const int32_t no_additional_parameters = 0;
	const int32_t one_additional_parameters = 31;
	const int32_t five_additional_parameters = 0x10410410;
	//const uint8_t five_parameters[5] = { 0x10, 0x10, 0x10, 0x10, 0x10 };
	uint8_t read_parameters[5];
	
	REQUIRE(WVT_W7_Parse_Additional_Parameters(read_parameters, no_additional_parameters) == 0);
	
	REQUIRE(WVT_W7_Parse_Additional_Parameters(read_parameters, one_additional_parameters) == 1);
	
	REQUIRE(WVT_W7_Parse_Additional_Parameters(read_parameters, five_additional_parameters) == 5);
	//MEMCMP_EQUAL(five_parameters, read_parameters, 5);
}