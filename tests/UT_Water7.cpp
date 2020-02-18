#include <stdint.h>
#include <string.h>
#include "../lib/WVT_Water7.h"
#include "catch.hpp"

uint8_t read_buffer[300];
WVT_W7_Error_t ext_rom_read(uint16_t address, int32_t * value);
WVT_W7_Error_t ext_rom_write(uint16_t address, int32_t value);

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

WVT_W7_Error_t ext_rom_read(uint16_t address, int32_t * value)
{
    *value = static_cast<uint16_t>(address);
    
    return WVT_W7_ERROR_CODE_OK;
}

WVT_W7_Error_t ext_rom_write(uint16_t address, int32_t value)
{
    value++;
    address++;
    
    return WVT_W7_ERROR_CODE_OK;
}

TEST_CASE("Callbacks", "[callbacks]")
{
    WVT_W7_Callbacks_t callbacks;

    callbacks.rom_read = ext_rom_read;
    callbacks.rom_write = ext_rom_write;

    WVT_W7_Register_Callbacks(callbacks);
}

TEST_CASE("Event", "[event]")
{
	const uint8_t packet_length = 5;
	const uint16_t event        = 0xBAAD;
	const uint16_t payload      = 0xBEEF;
	uint8_t event_packet[packet_length] = { 
    //  тип | идентификатор | значение
        0x20, 0xBA, 0xAD,     0xBE, 0xEF };

    CHECK(WVT_W7_Event(event, payload, read_buffer) == packet_length);
	CHECK(memcmp(event_packet, read_buffer, packet_length) == 0);
}

TEST_CASE("Short regular", "[short_regular]")
{
	const uint8_t packet_length_no_additional = 7;
	const uint8_t packet_length_one_additional = packet_length_no_additional + 5;
	const uint8_t packet_length_five_additional = packet_length_no_additional + (5 * 5);
	const uint16_t schedule = 0xBEEF;
	const int32_t payload = 0x7ACEFEED;
    const uint8_t parameter_address = 63;
	const int32_t five_additional_parameters = 0x10410410;
    
	uint8_t short_packet[packet_length_no_additional]           = { 
        0x80, 0xBE, 0xEF,  0x7A, 0xCE, 0xFE, 0xED };
    //  тип | расписание | значение 4 байта
	uint8_t short_packet_with_one[packet_length_one_additional] = {
    //  тип | расписание | значение 4 байта      | адрес доппараметра | его значение 4 байта
        0x80, 0xBE, 0xEF,  0x7A, 0xCE, 0xFE, 0xED, parameter_address,   0x00, 0x00, 0x00, parameter_address };

    CHECK(WVT_W7_Short_Regular(read_buffer, schedule, payload, 0) == packet_length_no_additional);
	CHECK(memcmp(short_packet, read_buffer, packet_length_no_additional) == 0);

    CHECK(WVT_W7_Short_Regular(read_buffer, schedule, payload, parameter_address) == packet_length_one_additional);
	CHECK(memcmp(short_packet_with_one, read_buffer, packet_length_one_additional) == 0);

    CHECK(WVT_W7_Short_Regular(read_buffer, schedule, payload, five_additional_parameters) == packet_length_five_additional);
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

TEST_CASE("Normal work", "[parser]")
{
    auto parameter_addres = GENERATE(0, 10, 63, 100, 65000);
    uint8_t read_single[3] = { 
    //  тип | параметр
        0x07, 0x00, 0x00 };
    uint8_t read_multiple[5] = { 
    //  тип | начало    |  длинна
        0x03, 0x00, 0x00, 0x00, 0x00};
    uint8_t write_single[7] = { 
    //  тип | параметр  | значение
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t write_multiple[5 + (4 * 10)] = { 
    //  тип | параметр  | длинна
        0x10, 0x00, 0x00, 0x00, 10 };


    SECTION("read_single") 
    {
        read_single[1] = static_cast<uint8_t>(parameter_addres >> 8);
        read_single[2] = static_cast<uint8_t>(parameter_addres);
        CHECK(WVT_W7_Parse(read_single, sizeof(read_single), read_buffer) == 7);
	    CHECK(memcmp(read_single, read_buffer, sizeof(read_single)) == 0);
        uint32_t parameter_value = 
                static_cast<uint32_t>(read_buffer[3] << 24)
            +   static_cast<uint32_t>(read_buffer[4] << 16)
            +   static_cast<uint32_t>(read_buffer[5] << 8)
            +   static_cast<uint32_t>(read_buffer[6]);
        CHECK(parameter_value == parameter_addres);
    }

    SECTION("read_multiple") 
    {
        uint8_t length = 20;
        read_multiple[1] = static_cast<uint8_t>(parameter_addres >> 8);
        read_multiple[2] = static_cast<uint8_t>(parameter_addres);
        read_multiple[4] = length;
        CHECK(WVT_W7_Parse(read_multiple, sizeof(read_multiple), read_buffer) == (5 + (length * 4)));

	    for(int i = 0; i < length; i++)
        {
            int buffer_position = 5 + (i * 4);
            uint32_t parameter_value = 
                    static_cast<uint32_t>(read_buffer[buffer_position] << 24)
                +   static_cast<uint32_t>(read_buffer[buffer_position + 1] << 16)
                +   static_cast<uint32_t>(read_buffer[buffer_position + 2] << 8)
                +   static_cast<uint32_t>(read_buffer[buffer_position + 3]);
            CHECK(parameter_value == (i + parameter_addres));
        }
    }

    SECTION("write_single") 
    {
        write_single[1] = static_cast<uint8_t>(parameter_addres >> 8);
        write_single[2] = static_cast<uint8_t>(parameter_addres);
        CHECK(WVT_W7_Parse(write_single, sizeof(write_single), read_buffer) == 7);
	    CHECK(memcmp(write_single, read_buffer, sizeof(write_single)) == 0);
    }

    SECTION("write_multiple") 
    {
        write_multiple[1] = static_cast<uint8_t>(parameter_addres >> 8);
        write_multiple[2] = static_cast<uint8_t>(parameter_addres);
        CHECK(WVT_W7_Parse(write_multiple, sizeof(write_multiple), read_buffer) == 5);
	    CHECK(memcmp(write_multiple, read_buffer, 5) == 0);
    }
}

TEST_CASE("Error handling", "[parser]")
{
    uint8_t read_single[3] = { 
    //  тип | параметр
        0x07, 0x00, 0x00 };
    uint8_t error_packet[2];
    //const uint8_t illegal_function = 0xFE;

    // Неверная длинна
    CHECK(WVT_W7_Parse(read_single, (sizeof(read_single) - 1), read_buffer) == 2);
    error_packet[0] = (read_single[0] | 0x40);
    error_packet[1] = 0x06;
	CHECK(memcmp(error_packet, read_buffer, sizeof(error_packet)) == 0);
}