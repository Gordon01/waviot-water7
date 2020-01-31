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

const WVT_W7_Parameter_t parameter = WVT_W7_IN_CMD_ADDITIONAL_PARAMETERS;

TEST_GROUP(WATER7)
{
	int32_t saved_value;
	
    void setup()
    {
	    WVT_ROM_Read_Parameter(parameter, &saved_value);
    }
    
    void teardown()
    {
	    WVT_ROM_Save_Parameter(parameter, saved_value);
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

TEST(WATER7, BoudariesCheck)
{
	uint8_t ret_comparison;
	const int16_t min_value_param = 6000;
	const int16_t max_value_param = 18000;

	ret_comparison = WVT_W7_Check_Current_Range(4000, min_value_param , max_value_param);
	CHECK_EQUAL(1, ret_comparison);
	ret_comparison = WVT_W7_Check_Current_Range(20000, min_value_param, max_value_param);
	CHECK_EQUAL(2, ret_comparison);
	ret_comparison = WVT_W7_Check_Current_Range(10000, min_value_param, max_value_param);
	CHECK_EQUAL(0, ret_comparison);
}

TEST(WATER7, BoundaryEvent)
{
	WVT_W7_Value_State_Event_t ret_event;
	uint32_t control_mask = 1 + 2;  	// Обе проверки
	const uint8_t previous_range = 0;
	const uint8_t alarm_range = 1;
	
	ret_event = WVT_W7_Pick_Boundary_Event(1, previous_range, control_mask);
	CHECK_EQUAL(WVT_W7_VALUE_BECAME_LOWER, ret_event);
	ret_event = WVT_W7_Pick_Boundary_Event(2, previous_range, control_mask);
	CHECK_EQUAL(WVT_W7_VALUE_BECAME_HIGHER, ret_event);
	ret_event = WVT_W7_Pick_Boundary_Event(0, previous_range, control_mask);
	CHECK_EQUAL(WVT_W7_VALUE_NO_CHANGE, ret_event);
	ret_event = WVT_W7_Pick_Boundary_Event(0, alarm_range, control_mask);
	CHECK_EQUAL(WVT_W7_VALUE_RETURNED_TO_NORMAL, ret_event);
	ret_event = WVT_W7_Pick_Boundary_Event(0, alarm_range, 0);
	CHECK_EQUAL(WVT_W7_VALUE_NO_CHANGE, ret_event);
}

TEST(WATER7, AdditionalParameters)
{
	const int32_t no_additional_parameters = 0;
	const int32_t one_additional_parameters = 31;
	const int32_t five_additional_parameters = 0x10410410;
	const uint8_t five_parameters[5] = { 0x10, 0x10, 0x10, 0x10, 0x10 };
	uint8_t read_parameters[5];
	uint8_t read_num_of_parameters;
	
	WVT_ROM_Save_Parameter(parameter, no_additional_parameters);
	read_num_of_parameters = WVT_W7_Get_Additional_Parameters(read_parameters);
	CHECK_EQUAL(0, read_num_of_parameters);
	
	WVT_ROM_Save_Parameter(parameter, one_additional_parameters);
	read_num_of_parameters = WVT_W7_Get_Additional_Parameters(read_parameters);
	CHECK_EQUAL(1, read_num_of_parameters);
	
	WVT_ROM_Save_Parameter(parameter, five_additional_parameters);
	read_num_of_parameters = WVT_W7_Get_Additional_Parameters(read_parameters);
	CHECK_EQUAL(5, read_num_of_parameters);
	MEMCMP_EQUAL(five_parameters, read_parameters, 5);
}