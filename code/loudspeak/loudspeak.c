#include "loudspeak.h"

static inline void LoudSpeak_SetTone(uint32 freq_hz)
{
    pwm_init(LOUDSPEAK_PWM_CHANNEL, freq_hz, 0U);
}

static inline void LoudSpeak_On(void)
{
    pwm_set_duty(LOUDSPEAK_PWM_CHANNEL, LOUDSPEAK_PWM_DUTY_ON);
}

static inline void LoudSpeak_Off(void)
{
    pwm_set_duty(LOUDSPEAK_PWM_CHANNEL, 0U);
}

static void LoudSpeak_Horn_Seconds(uint16 ms)
{
    LoudSpeak_On();
    system_delay_ms(ms);
    LoudSpeak_Off();
}

static void LoudSpeak_Horn_Times_1s(uint8 times)
{
    for(uint8 i = 0; i < times; i++)
    {
        LoudSpeak_On();
        system_delay_ms(LOUDSPEAK_MULTI_BEEP_MS);
        LoudSpeak_Off();

        if(i + 1U < times)
        {
            system_delay_ms(LOUDSPEAK_MULTI_GAP_MS);
        }
    }
}

void LoudSpeak_Init(void)
{
    LoudSpeak_SetTone(LOUDSPEAK_PWM_FREQ_HZ);
    LoudSpeak_Off();
}

void LoudSpeak_Horn_1s(void)
{
    LoudSpeak_Horn_Seconds(1000U);
}

void LoudSpeak_Horn_2s(void)
{
    LoudSpeak_Horn_Seconds(2000U);
}

void LoudSpeak_Horn_3s(void)
{
    LoudSpeak_Horn_Seconds(3000U);
}

void LoudSpeak_Horn_2Times(void)
{
    LoudSpeak_Horn_Times_1s(2U);
}

void LoudSpeak_Horn_3Times(void)
{
    LoudSpeak_Horn_Times_1s(3U);
}

void LoudSpeak_Horn_4Times(void)
{
    LoudSpeak_Horn_Times_1s(4U);
}

void LoudSpeak_Horn_LongShort(void)
{
    LoudSpeak_Horn_1s();
    system_delay_ms(1000U);
    LoudSpeak_Horn_3s();
}

void LoudSpeak_Horn_Rapid(void)
{
    for(uint8 i = 0; i < LOUDSPEAK_RAPID_REPEAT; i++)
    {
        LoudSpeak_On();
        system_delay_ms(LOUDSPEAK_RAPID_BEEP_MS);
        LoudSpeak_Off();

        if(i + 1U < LOUDSPEAK_RAPID_REPEAT)
        {
            system_delay_ms(LOUDSPEAK_RAPID_GAP_MS);
        }
    }
}

void LoudSpeak_Horn_Alarm(void)
{
    for(uint8 i = 0; i < LOUDSPEAK_ALARM_ALT_COUNT; i++)
    {
        uint32 freq = (i & 0x01U) ? LOUDSPEAK_ALARM_FREQ_2_HZ : LOUDSPEAK_ALARM_FREQ_1_HZ;
        LoudSpeak_SetTone(freq);
        LoudSpeak_On();
        system_delay_ms(LOUDSPEAK_ALARM_TONE_MS);
        LoudSpeak_Off();
    }

    // 警报结束后恢复默认音调
    LoudSpeak_SetTone(LOUDSPEAK_PWM_FREQ_HZ);
}
