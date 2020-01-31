#include <stdlib.h>
#include <string.h>

#include "wtimer.h"
#include "main.h"
#include "water7.h"
#include "meter.h"
#include "nbfi.h"
#include "rf.h"
#include "nbfi_config.h"

static struct wtimer_desc water7fastdl_off_desc;
static struct wtimer_desc water7crx_off_desc;

static void (*_get_data)(int32_t * data);
static void (*_set_data)(int32_t * data);
static void (*_save_data)(uint8_t * data);
static int32_t (*_rfl)(uint32_t addr, uint32_t len, uint32_t index, uint8_t *data, uint8_t cmd);

static water7_state_str _state;
static water7_params_str *_params = (water7_params_str *) &_state.parameters_array[WATER7_PARAMS_OFFSET];

static void bigendian_cpy(uint8_t * from, uint8_t * to, uint8_t len)
{
	for (uint8_t i = 0; i != len; i++)
	{
		to[i] = from[len - i - 1];
	}
}

static void UpdatePeriodTable(void)
{
	for (uint8_t i = 0; i < HOURS_PER_DAY; i++)
		_state.PeriodTable[i] = _params->period / HOURS_PER_DAY * (i + 1);
}

static void hours2bits(int32_t *RegularMas, uint8_t *Reg2Bits)
{
	uint32_t maxVal = 1;

	for (uint8_t i = 0; i < HOURS_PER_DAY; i++)
	{
		if (RegularMas[i] > maxVal)
			maxVal = RegularMas[i];
	}

	for (uint8_t i = 0; i < 12; i++)
	{
		Reg2Bits[i] = 0;

		if (RegularMas[0 + i * 2])
			Reg2Bits[i] |= ((((uint32_t)RegularMas[0 + i * 2] * 15 * 99 / maxVal / 100) + 1) << 4) & 0xF0;

		if (RegularMas[1 + i * 2])
			Reg2Bits[i] |= (((uint32_t)RegularMas[1 + i * 2] * 15 * 99 / maxVal / 100) + 1) & 0x0F;
	}
}

static void hours2bits_1bit(int32_t *RegularMas, uint8_t *Reg2Bits)
{
	uint32_t maxVal = 1;

	for (uint8_t i = 0; i < HOURS_PER_DAY; i++)
	{
		if (RegularMas[i] > maxVal)
			maxVal = RegularMas[i];
	}

	for (uint8_t i = 0; i < 3; i++)
	{
		Reg2Bits[i] = 0;
		for (uint8_t j = 0; j < 8; j++)
		{
			if (RegularMas[i * 8 + j])
				Reg2Bits[i] |= (0x01 << (7 - j));
		}
	}
}

static uint16_t Water7PeriodToUINT16(uint32_t period)
{
	uint16_t days = period / SECONDS_PER_DAY;
	if (days > 0x7FF)
		days = 0x7FF;
	period %= SECONDS_PER_DAY;
	uint16_t hours = period / SECONDS_PER_HOUR;

	return (days << 5) | (hours & 0x1F);
}

static void Water7Regular(void)
{
	uint8_t buf[WATER7_REGULAR_BUF_SIZE];
	uint8_t index = 0;

	for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
	{
		uint32_t need_send = _params->send_flag[i >> 5] & ((uint32_t)1 << (i & 0x1F));
		uint32_t need_send_hour = _params->send_hour_flag[i >> 5] & ((uint32_t)1 << (i & 0x1F));
		uint8_t len = 0;

		if (need_send)
			len += 5;
		if (need_send_hour)
			len += 12 + 2;
		if (len + index > WATER7_REGULAR_BUF_SIZE)
			break;

		if (need_send)
		{
			if (index == 0)
			{
				buf[index++] = CMD_REGULAR | (i & 0x3F) | (need_send_hour ? 0x40 : 0);
				uint16_t tmp = Water7PeriodToUINT16(_params->period);
				bigendian_cpy((uint8_t *)&tmp, (uint8_t *)&buf[index], 2);
				index += 2;
			}
			else
				buf[index++] = (i & 0x3F) | (need_send_hour ? 0x40 : 0);

			bigendian_cpy((uint8_t *)&_state.parameters_array[WATER7_USERDATA_OFFSET + i], (uint8_t *)&buf[index], 4);
			index += 4;

			if (need_send_hour)
			{
				int16_t tmp = _state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevRegular[i];
				bigendian_cpy((uint8_t *)&tmp, (uint8_t *)&buf[index], 2);
				index += 2;
				hours2bits(&_state.RegularMas[i][0], &buf[index]);
				index += 12;
			}
		}
	}

	if (index == 0)
		buf[index++] = CMD_REGULAR;

	NBFi_Send(buf, index);
	_params->mesNum++;

	for (uint8_t i = 0; i < index; i++)
		_params->BufPrev[i] = buf[i];

	_params->length_buf = index;
}

static void Water7RegularShort(void)
{
	uint8_t buf[WATER7_REGULAR_BUF_SIZE];
	uint8_t index = 0, sended_message = 0;

	for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
	{
		uint32_t need_send = _params->send_flag[i >> 5] & ((uint32_t)1 << (i & 0x1F));
		uint32_t need_send_hour = _params->send_hour_flag[i >> 5] & ((uint32_t)1 << (i & 0x1F));
		index = 0;

		if (need_send)
		{
			buf[index++] = CMD_REGULAR | (i & 0x3F) | (need_send_hour ? 0x40 : 0);
			bigendian_cpy((uint8_t *)&_state.parameters_array[WATER7_USERDATA_OFFSET + i], (uint8_t *)&buf[index], 4);
			index += 4;

			if (need_send_hour)
			{
				hours2bits_1bit(&_state.RegularMas[i][0], &buf[index]);
				index += 3;
			}
			else
			{
				for (uint8_t i = 0; i < 3; i++)
					buf[index++] = 0;
			}
			NBFi_Send(buf, index);
			_params->mesNum++;

			for (uint8_t i = 0; i < index; i++)
				_params->BufPrev[i] = buf[i];

			_params->length_buf = index;
			sended_message++;
		}
	}

	if (sended_message == 0)
	{
		buf[index++] = CMD_REGULAR;
		NBFi_Send(buf, index);
		_params->mesNum++;

		for (uint8_t i = 0; i < index; i++)
			_params->BufPrev[i] = buf[i];

		_params->length_buf = index;
		sended_message++;
	}
}

void Water7PushFunc(uint8_t func_type, void *func)
{
	switch(func_type)
	{
	case WATER7_FUNC_SET_DATA:
		_set_data = func;
		break;
	case WATER7_FUNC_GET_DATA:
		_get_data = func;
		break;
	case WATER7_FUNC_SAVE_DATA:
		_save_data = func;
		break;
	case WATER7_FUNC_RFL:
		_rfl = func;
		break;
	default:
		break;
	}
}

void Water7Init(water7_params_str * params)
{
	if (!params)
	{
		memset((uint8_t *)_params, 0, sizeof(water7_params_str));
		_params->send_flag[0] = 1;
		_params->send_hour_flag[0] = 1;
		_params->period = ONEDAY;
	}
	else
		memcpy((uint8_t *)_params, (uint8_t *)params, sizeof(water7_params_str));
	UpdatePeriodTable();
}

uint8_t Water7Loop(void)
{
	return 0;
}

uint8_t Water7isCanSleep(void)
{
	return 1;
}

void Water7OneSec(uint32_t inc_time)
{
	if (_get_data)
		_get_data(_state.parameters_array);

	_state.Seconds += inc_time;
	if (_state.Seconds >= SECONDS_PER_MINUTE)
	{
		_state.Seconds = 0;
		for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
		{
			if ((_state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevMinute[i]) > _params->par_max[i])
				_params->par_max[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevMinute[i];
			_state.PrevMinute[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
		}

		if (++_state.Minutes >= MINUTES_PER_HOUR)
		{
			_state.Minutes = 0;
			if (++_state.Hours >= HOURS_PER_DAY)
			{
				_state.Hours = 0;
				if (++_state.Days >= DAYS_PER_WEEK)
				{
					_state.Days = 0;
				}
				if (++_state.Days4month >= DAYS_PER_MONTH)
				{
					_state.Days4month = 0;
				}
				_save_data((uint8_t *)_state.parameters_array);
			}
		}
	}

	_state.SecondsRegular += inc_time;
	if (_state.SecondsRegular >= _state.PeriodTable[_state.MasCount] / WATER7_ACCEL_REGULAR)
	{
		if (_state.MasCount < 24)
		{
			for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
			{
				if (_state.parameters_array[WATER7_USERDATA_OFFSET + i] > _state.PrevRegularMas[i])
					_state.RegularMas[i][_state.MasCount] = _state.parameters_array[WATER7_USERDATA_OFFSET + i] - _state.PrevRegularMas[i];
				else
					_state.RegularMas[i][_state.MasCount] = _state.PrevRegularMas[i] - _state.parameters_array[WATER7_USERDATA_OFFSET + i];

				_state.PrevRegularMas[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
			}
			_state.MasCount++;
		}
	}


	if (_state.SecondsRegular >= _params->period / WATER7_ACCEL_REGULAR)
	{
#define WATER7_MAX_NRX_TRY	6
#define WATER7_MAX_DRX_TRY	1
		static uint32_t fault_prev, fault_count, success_prev, nrx_count, drx_count;
		nbfi_state_t * nbfi_state = NBFi_get_state();
		nbfi_settings_t * nbfi_settings = NBFi_get_settings();
		if (nbfi_settings->mode != CRX)
		{
			if (nbfi_state->fault_total > fault_prev && nbfi_state->success_total == success_prev)
				fault_count++;
			fault_prev = nbfi_state->fault_total;
			success_prev = nbfi_state->success_total;
			if (nbfi_settings->mode == NRX)
			{
				if (nrx_count >= WATER7_MAX_NRX_TRY)
				{
					nrx_count = fault_count = 0;
					NBFi_Switch_Mode(DRX);
				}
			}
			else
			{
				if (fault_count >= WATER7_MAX_DRX_TRY)
				{
					nrx_count = fault_count = 0;
					NBFi_Switch_Mode(NRX);
				}
			}

			if (nbfi_settings->mode == NRX)
			{
				drx_count = 0;
				nrx_count++;
			}
			else
			{
				nrx_count = 0;
				drx_count++;
			}

			if (drx_count > WATER7_MAX_DRX_TRY)
				Water7Regular();
			else
				Water7RegularShort();

			Water7ForceEvent();
		}

		_state.SecondsRegular = 0;
		for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
		{
			_state.PrevRegularMas[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
			_state.PrevRegular[i] = _state.parameters_array[WATER7_USERDATA_OFFSET + i];
			_state.MasCount = 0;
		}
	}
}

void Water7fastdl_off(struct wtimer_desc *desc)
{
	nbfi_settings_t *nbfi_settings = NBFi_get_settings();
	if (nbfi_settings->mode == CRX)
	{
		if (!desc)
	        wtimer0_remove(&water7fastdl_off_desc);
		NBFi_Config_Set_FastDl(0, 1);
	}
}

void Water7fastdl_on(void)
{
	nbfi_settings_t *nbfi_settings = NBFi_get_settings();
	if (nbfi_settings->mode == NRX || nbfi_settings->mode == DRX)
	{
		NBFi_Config_Set_FastDl(1, 1);
		ScheduleTask(&water7fastdl_off_desc, &Water7fastdl_off, RELATIVE, SECONDS(WATER7_FASTDL_TIMEOUT));
	}
}

void NBFi_Switch_Mode(uint8_t mode)
{
	nbfi_settings_t *nbfi_settings = NBFi_get_settings();
	if (nbfi_settings->mode != mode)
	{
		if (mode == NRX)
			nbfi_settings->mack_mode = MACK_0;
		else
			nbfi_settings->mack_mode = MACK_1;
		nbfi_settings->mode = mode;
		NBFi_Clear_TX_Buffer();
		if(rf_state == STATE_RX)
			NBFi_RX();
        NBFi_Config_Send_Mode(0, NBFI_PARAM_MODE);
	}
}

void Water7crx_off(struct wtimer_desc *desc)
{
	NBFi_Switch_Mode(DRX);
}

void Water7RXcallback(uint8_t * data, uint16_t length)
{
	uint16_t len = 0, addr = 0, cmd = 0, index = 0;
	uint32_t addr32 = 0, tmp32 = 0;
	int32_t ret = 0;
	uint8_t tmpbuf[WATER7_CALLBACK_BUF_SIZE];

	_state.period_prev = _params->period;

	if (CheckTask(&water7fastdl_off_desc))
		ScheduleTask(&water7fastdl_off_desc, &Water7fastdl_off, RELATIVE, SECONDS(WATER7_FASTDL_TIMEOUT));

	if (_get_data)
		_get_data(_state.parameters_array);

	switch(data[0])
	{
	case CMD_MULTIREAD:
		bigendian_cpy((uint8_t *)&data[1], (uint8_t *)&addr, 2);
		bigendian_cpy((uint8_t *)&data[3], (uint8_t *)&len, 2);
		if ((length == 5) && ((5 + len * 4) <= WATER7_CALLBACK_BUF_SIZE) && (addr + len <= WATER7_PAR_LENGTH))
		{
			for (uint8_t i = 0; i < 5; i++)
				tmpbuf[i] = data[i];
			for (uint8_t i = 0; i < len; i++)
				bigendian_cpy((uint8_t *)&_state.parameters_array[addr + i], (uint8_t *)&tmpbuf[5 + i * 4], 4);
			NBFi_Send(tmpbuf, 5 + len * 4);
		}
		else
		{
			tmpbuf[0] = data[0] | CMD_ERR;
			tmpbuf[1] = 0x02;
			NBFi_Send(tmpbuf, 2);
		}
		break;
	case CMD_MULTIPRESS:
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
	case CMD_SINGLEREAD:
		bigendian_cpy((uint8_t *)&data[1], (uint8_t *)&addr, 2);
		if ((length == 3) && (addr < WATER7_PAR_LENGTH))
		{
			for (uint8_t i = 0; i < length; i++)
				tmpbuf[i] = data[i];
			bigendian_cpy((uint8_t *)&_state.parameters_array[addr], (uint8_t *)&tmpbuf[3], 4);
			NBFi_Send(tmpbuf, 7);
		}
		else
		{
			tmpbuf[0] = data[0] | CMD_ERR;
			tmpbuf[1] = 0x02;
			NBFi_Send(tmpbuf, 2);
		}
		break;
	case CMD_SINGLEPRESS:
		bigendian_cpy((uint8_t *)&data[1], (uint8_t *)&addr, 2);
		if ((length == 7) && (addr < WATER7_PAR_LENGTH))
		{
			for (uint8_t i = 0; i < length; i++)
				tmpbuf[i] = data[i];
			bigendian_cpy((uint8_t *)&data[3], (uint8_t *)&_state.parameters_array[addr], 4);
			NBFi_Send(tmpbuf, length);
		}
		else
		{
			tmpbuf[0] = data[0] | CMD_ERR;
			tmpbuf[1] = 0x02;
			NBFi_Send(tmpbuf, 2);
		}
		break;
	case CMD_RLF:
		if (length < 2)
			break;
		NBFi_Switch_Mode(CRX);
		ScheduleTask(&water7crx_off_desc, &Water7crx_off, RELATIVE, SECONDS(WATER7_CRX_TIMEOUT));
		switch(data[1])
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
	case CMD_CTRL:
		if (length != 7)
			break;
		bigendian_cpy((uint8_t *)&data[1], (uint8_t *)&cmd, 2);
		bigendian_cpy((uint8_t *)&data[3], (uint8_t *)&tmp32, 4);

		switch(cmd)
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
		break;
	}
	if (_state.period_prev != _params->period)
		UpdatePeriodTable();

	if (_set_data)
		_set_data(_state.parameters_array);
}

void Water7PushEvent(uint16_t event, uint16_t payload)
{
	if (_state.events_to_send >= WATER7_MAX_EVENT_COUNT)
		return;

	for (uint8_t i = 0; i < _state.events_to_send; i++)
		if (_state.events[i][0] == event && _state.events[i][1] == payload)
			return;

	_state.events[_state.events_to_send][0] = event;
	_state.events[_state.events_to_send][1] = payload;
	_state.events_to_send++;
}

void Water7ForceEvent(void)
{
	uint8_t tmpbuf[WATER7_MAX_EVENT_COUNT * 4 + 1];
	uint8_t index = 0;

	if (!_state.events_to_send)
		return;

	tmpbuf[index++] = CMD_EVENT;
	for (uint16_t i = 0; i < _state.events_to_send; i++)
	{
		bigendian_cpy((uint8_t *)&_state.events[i][0], (uint8_t *)&tmpbuf[index], 2);
		index += 2;
		bigendian_cpy((uint8_t *)&_state.events[i][1], (uint8_t *)&tmpbuf[index], 2);
		index += 2;
	}
	_state.events_to_send = 0;
	NBFi_Send(tmpbuf, index);
}

void Water7SendEvent(uint16_t event, uint16_t payload)
{
	uint8_t tmpbuf[10];
	uint8_t index = 0;

	tmpbuf[index++] = CMD_EVENT;
	bigendian_cpy((uint8_t *)&event, (uint8_t *)&tmpbuf[index], 2);
	index += 2;
	bigendian_cpy((uint8_t *)&payload, (uint8_t *)&tmpbuf[index], 2);
	index += 2;

	NBFi_Send(tmpbuf, index);
}

void Water7ForceData(void)
{
	uint8_t buf[WATER7_REGULAR_BUF_SIZE];
	uint8_t index = 0;

	for (uint8_t i = 0; i < WATER7_PAR_COUNT; i++)
	{
		uint32_t need_send = _params->send_flag[i >> 5] & ((uint32_t)1 << (i & 0x1F));
		uint8_t len = 0;

		if (need_send)
			len += 5;
		if (len + index > WATER7_REGULAR_BUF_SIZE)
			break;

		if (need_send)
		{
			if (index == 0)
			{
				buf[index++] = CMD_REGULAR | (i & 0x3F);
				uint16_t tmp = Water7PeriodToUINT16(_params->period);
				bigendian_cpy((uint8_t *)&tmp, (uint8_t *)&buf[index], 2);
				index += 2;
			}
			else
				buf[index++] = (i & 0x3F);

			bigendian_cpy((uint8_t *)&_state.parameters_array[WATER7_USERDATA_OFFSET + i], (uint8_t *)&buf[index], 4);
			index += 4;
		}
	}

	if (index == 0)
		buf[index++] = CMD_REGULAR;

	NBFi_Send(buf, index);
}
