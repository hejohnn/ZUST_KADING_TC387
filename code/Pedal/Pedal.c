/*
 * Pedal.c
 *
 *  Created on: 2026年4月11日
 *      Author: 17706
 */

#include "Pedal.h"

#define PEDAL_ADC_CHANNEL        (ADC8_CH15_A47)
#define PEDAL_ADC_MAX_VALUE      (4095U)
#define PEDAL_ADC_REF_MV         (3300U)
#define PEDAL_SAMPLE_COUNT       (16U)
#define PEDAL_WARMUP_COUNT       (2U)
#define PEDAL_VALID_SPAN_MIN     (20U)
#define PEDAL_IDLE_RAW_WINDOW    (8U)
#define PEDAL_PERCENT_DEADBAND   (2U)
#define PEDAL_PERCENT_LPF_SHIFT  (2U)

typedef struct
{
	uint16 raw;
	uint16 voltage_mv;
	uint16 min;
	uint16 max;
	uint16 span;
	uint16 percent_raw;
	uint16 percent;
	uint8 signal_valid;
	uint8 percent_filtered_inited;
	uint16 percent_filtered;
} pedal_state_t;

static pedal_state_t pedal_state;

static uint16 pedal_calc_percent_raw(pedal_state_t *state)
{
	uint16 pedal_percent_raw = 0U;

	if(state->raw < state->min)
	{
		state->min = state->raw;
	}
	if(state->raw > state->max)
	{
		state->max = state->raw;
	}

	if(state->max > state->min)
	{
		state->span = state->max - state->min;
		if(state->span >= PEDAL_VALID_SPAN_MIN)
		{
			state->signal_valid = 1U;
			if(state->raw <= (uint16)(state->min + PEDAL_IDLE_RAW_WINDOW))
			{
				pedal_percent_raw = 0U;
			}
			else
			{
				pedal_percent_raw = (uint16)(((uint32)(state->raw - state->min) * 100U) / (uint32)(state->span));
			}
			if(pedal_percent_raw > 100U)
			{
				pedal_percent_raw = 100U;
			}
		}
		else
		{
			state->signal_valid = 0U;
		}
	}
	else
	{
		state->signal_valid = 0U;
	}

	return pedal_percent_raw;
}

static uint16 pedal_filter_percent(pedal_state_t *state)
{
	if(!state->signal_valid)
	{
		state->percent_filtered = 0U;
		state->percent_filtered_inited = 0U;
		return 0U;
	}

	if(!(state->percent_filtered_inited))
	{
		state->percent_filtered = state->percent_raw;
		state->percent_filtered_inited = 1U;
		return state->percent_filtered;
	}

	{
		uint16 diff = (state->percent_raw > state->percent_filtered) ?
					  (state->percent_raw - state->percent_filtered) :
					  (state->percent_filtered - state->percent_raw);

		if(diff > PEDAL_PERCENT_DEADBAND)
		{
			state->percent_filtered = (uint16)((((uint32)(state->percent_filtered) * ((1U << PEDAL_PERCENT_LPF_SHIFT) - 1U))
										  + (uint32)state->percent_raw
										  + (1U << (PEDAL_PERCENT_LPF_SHIFT - 1U)))
										 >> PEDAL_PERCENT_LPF_SHIFT);
		}
	}

	return state->percent_filtered;
}

void Pedal_Init(void)
{
	uint8 i = 0U;

	adc_init(PEDAL_ADC_CHANNEL, ADC_12BIT);

	pedal_state.raw = 0U;
	pedal_state.voltage_mv = 0U;
	pedal_state.min = PEDAL_ADC_MAX_VALUE;
	pedal_state.max = 0U;
	pedal_state.span = 0U;
	pedal_state.percent_raw = 0U;
	pedal_state.percent = 0U;
	pedal_state.signal_valid = 0U;
	pedal_state.percent_filtered_inited = 0U;
	pedal_state.percent_filtered = 0U;

	for(i = 0U; i < PEDAL_WARMUP_COUNT; i++)
	{
		(void)adc_mean_filter_convert(PEDAL_ADC_CHANNEL, PEDAL_SAMPLE_COUNT);
	}
}

void Pedal_Update(void)
{
	pedal_state.raw = adc_mean_filter_convert(PEDAL_ADC_CHANNEL, PEDAL_SAMPLE_COUNT);
	pedal_state.voltage_mv = (uint16)(((uint32)pedal_state.raw * PEDAL_ADC_REF_MV) / PEDAL_ADC_MAX_VALUE);
	pedal_state.percent_raw = pedal_calc_percent_raw(&pedal_state);
	pedal_state.percent = pedal_filter_percent(&pedal_state);
}

uint16 Pedal_GetRaw(void)
{
	return pedal_state.raw;
}

uint16 Pedal_GetVoltageMv(void)
{
	return pedal_state.voltage_mv;
}

uint16 Pedal_GetPercent(void)
{
	return pedal_state.percent;
}

uint16 Pedal_GetPercentRaw(void)
{
	return pedal_state.percent_raw;
}

uint16 Pedal_GetMinRaw(void)
{
	return pedal_state.min;
}

uint16 Pedal_GetMaxRaw(void)
{
	return pedal_state.max;
}

uint8 Pedal_IsSignalValid(void)
{
	return pedal_state.signal_valid;
}




