#include <stdint.h>
#include <string.h>
#include "../lib/WVT_Water7.h"
#include "catch.hpp"

uint8_t read_buffer[500];

/**
 * Стартовый пакет состоит из одного пакета о перезагрузке со следующими параметрами:
 * - event_id = 0x0000
 * - payload = число перезагрузок
 * 
 * Отрицательный аргумент или 0 выдают пакет с payload == 0x0000
 */
TEST_CASE("Reset", "[reset]")
{
	const uint8_t packet_length = 5;
	const uint16_t resets = 65500;
	uint8_t reset_packet[packet_length] = { 0x20, 0x00, 0x00, 0x00, 0x00 };

	CHECK(WVT_W7_Start(0, read_buffer) == packet_length);
	CHECK(memcmp(reset_packet, read_buffer, packet_length) == 0);

	CHECK(WVT_W7_Start(-1, read_buffer) == packet_length);
	CHECK(memcmp(reset_packet, read_buffer, packet_length) == 0);

	reset_packet[packet_length - 2] = static_cast<uint8_t>(resets >> 8);
	reset_packet[packet_length - 1] = static_cast<uint8_t>(resets);
	CHECK(WVT_W7_Start(resets, read_buffer) == packet_length);
	CHECK(memcmp(reset_packet, read_buffer, packet_length) == 0);
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
