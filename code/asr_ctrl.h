#ifndef CODE_ASR_CTRL_H_
#define CODE_ASR_CTRL_H_

// 连接到本地代理服务器（运行在您的电脑上）
#define ASR_TARGET_IP           "10.53.66.70"  // 改为您电脑的 IP 地址
#define ASR_TARGET_PORT         "8888"         // 代理服务器端口
#define ASR_LOCAL_PORT          "0"

#define ASR_AUDIO_ADC           ADC0_CH0_A0        //采样所用使用的ADC

#define ASR_BUTTON              P20_9
#define ASR_BUTTON_ACTIVE_HIGH   0
#define ASR_PIT                 CCU61_CH1

#define ASR_WIFI_SSID           "hejohn"                         // wifi名称 wifi需要是2.4G频率
#define ASR_WIFI_PASSWORD       "qwe12345"                            // wifi密码

#define ASR_APIID               "0af3349a"                              // 讯飞的id

#define RANDOM_NUM_ADC          ADC0_CH5_A5                             // 随机数 ADC 通道，作用是给 WebSocket 客户端发包时提供随机掩码来源。

#endif /* CODE_ASR_CTRL_H_ */
