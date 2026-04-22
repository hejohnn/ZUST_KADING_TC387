#ifndef CODE_LOUDSPEAK_LOUDSPEAK_H_
#define CODE_LOUDSPEAK_LOUDSPEAK_H_

#include "zf_common_headfile.h"

#define LOUDSPEAK_PWM_CHANNEL      ATOM2_CH0_P33_4
#define LOUDSPEAK_PWM_FREQ_HZ      2000U
#define LOUDSPEAK_PWM_DUTY_ON      5000U

#define LOUDSPEAK_MULTI_BEEP_MS    1000U
#define LOUDSPEAK_MULTI_GAP_MS     1000U
#define LOUDSPEAK_RAPID_BEEP_MS    500U
#define LOUDSPEAK_RAPID_GAP_MS     500U
#define LOUDSPEAK_RAPID_REPEAT     6U

#define LOUDSPEAK_ALARM_FREQ_1_HZ  500U
#define LOUDSPEAK_ALARM_FREQ_2_HZ  1000U
#define LOUDSPEAK_ALARM_TONE_MS    1000U
#define LOUDSPEAK_ALARM_ALT_COUNT  6U

void LoudSpeak_Init(void);

void LoudSpeak_Horn_1s(void);
void LoudSpeak_Horn_2s(void);
void LoudSpeak_Horn_3s(void);

void LoudSpeak_Horn_2Times(void);
void LoudSpeak_Horn_3Times(void);
void LoudSpeak_Horn_4Times(void);

void LoudSpeak_Horn_LongShort(void);
void LoudSpeak_Horn_Rapid(void);
void LoudSpeak_Horn_Alarm(void);

#endif /* CODE_LOUDSPEAK_LOUDSPEAK_H_ */
