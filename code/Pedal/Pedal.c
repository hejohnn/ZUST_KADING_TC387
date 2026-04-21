/*
 * Pedal.c - 踏板 ADC 采样 + 自标定 min/max + 百分比 LPF
 *
 * 算法:
 *   1. 16 次均值滤波 ADC -> raw (0..4095)
 *   2. 自动学习 min/max, 当 span >= PEDAL_VALID_SPAN_MIN 才认为信号有效
 *   3. 接近 min 的 IDLE_RAW_WINDOW 内强制 0%, 避免静态噪声抖动
 *   4. 百分比一阶 IIR (alpha=1/4), diff <= 死区时保持不变
 */

#include "Pedal.h"

#define PEDAL_ADC_CHANNEL        (ADC8_CH15_A47)
#define PEDAL_ADC_MAX_VALUE      (4095U)
#define PEDAL_SAMPLE_COUNT       (16U)
#define PEDAL_WARMUP_COUNT       (2U)
#define PEDAL_VALID_SPAN_MIN     (20U)
#define PEDAL_IDLE_RAW_WINDOW    (8U)
#define PEDAL_PERCENT_DEADBAND   (2U)
#define PEDAL_PERCENT_LPF_SHIFT  (2U)

typedef struct
{
	uint16 raw;
	uint16 min;
	uint16 max;
	uint16 percent;
	uint8  signal_valid;
	uint8  filter_primed;
} pedal_state_t;

static pedal_state_t pedal_state;

static uint16 pedal_calc_percent_raw(pedal_state_t *s)
{
	uint16 span;

	if(s->raw < s->min) s->min = s->raw;
	if(s->raw > s->max) s->max = s->raw;

	if(s->max <= s->min)
	{
		s->signal_valid = 0U;
		return 0U;
	}

	span = (uint16)(s->max - s->min);
	if(span < PEDAL_VALID_SPAN_MIN)
	{
		s->signal_valid = 0U;
		return 0U;
	}

	s->signal_valid = 1U;
	if(s->raw <= (uint16)(s->min + PEDAL_IDLE_RAW_WINDOW))
	{
		return 0U;
	}

	{
		uint32 pct = ((uint32)(s->raw - s->min) * 100U) / (uint32)span;
		return (pct > 100U) ? 100U : (uint16)pct;
	}
}

static uint16 pedal_filter_percent(pedal_state_t *s, uint16 pct_raw)
{
	uint16 diff;
	uint32 weighted;

	if(!s->signal_valid)
	{
		s->filter_primed = 0U;
		return 0U;
	}

	if(!s->filter_primed)
	{
		s->filter_primed = 1U;
		return pct_raw;
	}

	diff = (pct_raw > s->percent) ? (uint16)(pct_raw - s->percent)
								  : (uint16)(s->percent - pct_raw);
	if(diff <= PEDAL_PERCENT_DEADBAND) return s->percent;

	weighted = (uint32)s->percent * ((1U << PEDAL_PERCENT_LPF_SHIFT) - 1U)
			 + (uint32)pct_raw
			 + (1U << (PEDAL_PERCENT_LPF_SHIFT - 1U));
	return (uint16)(weighted >> PEDAL_PERCENT_LPF_SHIFT);
}

void Pedal_Init(void)
{
	uint8 i;

	adc_init(PEDAL_ADC_CHANNEL, ADC_12BIT);

	pedal_state.raw           = 0U;
	pedal_state.min           = PEDAL_ADC_MAX_VALUE;
	pedal_state.max           = 0U;
	pedal_state.percent       = 0U;
	pedal_state.signal_valid  = 0U;
	pedal_state.filter_primed = 0U;

	for(i = 0U; i < PEDAL_WARMUP_COUNT; i++)
	{
		(void)adc_mean_filter_convert(PEDAL_ADC_CHANNEL, PEDAL_SAMPLE_COUNT);
	}
}

void Pedal_Update(void)
{
	uint16 pct_raw;

	pedal_state.raw = adc_mean_filter_convert(PEDAL_ADC_CHANNEL, PEDAL_SAMPLE_COUNT);
	pct_raw = pedal_calc_percent_raw(&pedal_state);
	pedal_state.percent = pedal_filter_percent(&pedal_state, pct_raw);
}

uint16 Pedal_GetPercent(void)
{
	return pedal_state.percent;
}

uint8 Pedal_IsSignalValid(void)
{
	return pedal_state.signal_valid;
}
