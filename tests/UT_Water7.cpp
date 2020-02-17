#include <stdint.h>
#include <string.h>
#include "../lib/WVT_Water7.h"
#include "catch.hpp"

uint8_t read_buffer[500];

/**
 * Стартовый пакет состоит из одного пакета-события
 * 
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

TEST_CASE("Event", "[event]")
{
	const uint8_t packet_length = 5;
	const uint16_t event = 47789;
	const uint16_t payload = 48879;
	uint8_t event_packet[packet_length] = { 0x20, 0xBA, 0xAD, 0xBE, 0xEF };

    CHECK(WVT_W7_Event(event, payload, read_buffer) == packet_length);
	CHECK(memcmp(event_packet, read_buffer, packet_length) == 0);
}

TEST_CASE("Short regular", "[short_regular]")
{
	const uint8_t packet_length_no_additional = 7;
	//const uint8_t packet_length_one_additional = packet_length_no_additional + 5;
	const uint16_t schedule = 0xBEEF;
	const int32_t payload = 0x7ACEFEED;
    //const uint8_t parameter_address = 63;
	uint8_t short_packet[packet_length_no_additional]           = { 0x80, 0xBE, 0xEF, 0x7A, 0xCE, 0xFE, 0xED };
	//uint8_t short_packet_with_one[packet_length_one_additional] = {
    //    0x80, 0xBE, 0xEF, 0x7A, 0xCE, 0xFE, 0xED, parameter_addres, 0x00, 0x00, 0x00, 0x00 };

    CHECK(WVT_W7_Short_Regular(read_buffer, schedule, payload, 0) == packet_length_no_additional);
	CHECK(memcmp(short_packet, read_buffer, packet_length_no_additional) == 0);

    //CHECK(WVT_W7_Short_Regular(read_buffer, schedule, payload, parameter_addres) == packet_length_one_additional);
	//CHECK(memcmp(short_packet_with_one, read_buffer, packet_length_one_additional) == 0);
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
