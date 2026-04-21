/*********************************************************************************************************************
* TC387 Opensourec Library 即（TC387 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 TC387 开源库的一部分
*
* TC387 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          cpu0_main
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          ADS v1.10.2
* 适用平台          TC387QP
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者                备注
* 2022-11-04       pudding            first version
********************************************************************************************************************/
#include "zf_common_headfile.h"
#include "MYHEADFILE.h"
#pragma section all "cpu0_dsram"

// 将本语句与#pragma section all restore语句之间的全局变量都放在CPU0的RAM中
// *************************** 例程硬件连接说明 ***************************
// 接入总钻风灰度数字摄像头 对应主板摄像头接口 请注意线序
//      模块管脚            单片机管脚
//      TXD                 查看 zf_device_mt9v03x.h 中 MT9V03X_COF_UART_TX 宏定义
//      RXD                 查看 zf_device_mt9v03x.h 中 MT9V03X_COF_UART_RX 宏定义
//      PCLK                查看 zf_device_mt9v03x.h 中 MT9V03X_PCLK_PIN 宏定义
//      VSY                 查看 zf_device_mt9v03x.h 中 MT9V03X_VSYNC_PIN 宏定义
//      D0-D7               查看 zf_device_mt9v03x.h 中 MT9V03X_DATA_PIN 宏定义 从该定义开始的连续八个引脚
//      GND                 核心板电源地 GND
//      3V3                 核心板 3V3 电源
// 接入2寸IPS模块
// *************************** 例程硬件连接说明 ***************************
//      模块管脚            单片机管脚
//      BL                  查看 zf_device_ips200_parallel8.h 中 IPS200_BL_PIN 宏定义  默认 P15_3
//      CS                  查看 zf_device_ips200_parallel8.h 中 IPS200_CS_PIN 宏定义  默认 P15_5
//      RST                 查看 zf_device_ips200_parallel8.h 中 IPS200_RST_PIN 宏定义 默认 P15_1
//      RS                  查看 zf_device_ips200_parallel8.h 中 IPS200_RS_PIN 宏定义  默认 P15_0
//      WR                  查看 zf_device_ips200_parallel8.h 中 IPS200_WR_PIN 宏定义  默认 P15_2
//      RD                  查看 zf_device_ips200_parallel8.h 中 IPS200_RD_PIN 宏定义  默认 P15_4
//      D0-D7               查看 zf_device_ips200_parallel8.h 中 IPS200_Dx_PIN 宏定义  默认 P11_9/P11_10/P11_11/P11_12/P13_0/P13_1/P13_2/P13_3
//      GND                 核心板电源地 GND
//      3V3                 核心板 3V3 电源


//                              此处我硬件实际使用的是p2类的引脚


// *************************** 例程测试说明 ***************************
// 1.核心板烧录完成本例程 将核心板插在主板上 插到底
// 2.摄像头接在主板的摄像头接口 注意线序2寸IPS模块插入主板屏幕接口
// 3.主板上电 或者核心板链接完毕后上电 核心板按下复位按键
// 4.屏幕会显示初始化信息然后显示摄像头图像
// 如果发现现象与说明严重不符 请参照本文件最下方 例程常见问题说明 进行排查
// **************************** 代码区域 ****************************

#define IPS200_TYPE     (IPS200_TYPE_SPI)                                       // 双排排针 并口两寸屏 这里宏定义填写 IPS200_TYPE_PARALLEL8

                                                                          // 单排排针 SPI 两寸屏 这里宏定义填写 IPS200_TYPE_SPI
//uint8_t Init_End_Flag = 0;                  //初始化结束标志符
//
//uint8_t wifi_Init_End_Flag = 0;                  //初始化结束标志符



// **************************** 代码区域 ****************************
int core0_main(void)
{
    clock_init();                   // 获取时钟频率<务必保留>
    debug_init();                   // 初始化默认调试串口
    Turn_Init();                    //转向电机闭环初始化(优先接管P14_5/P14_6防误驱动)
    Remote_Init();                  //遥控模块初始化(SBUS接收)
    // 此处编写用户代码 例如外设初始化代码等
    Beep_Init();                    //蜂鸣器初始化
    LoudSpeak_Init();               //PWM喇叭初始化(P33.5)
    Pedal_Init();                   //踏板ADC初始化
    Motor_Init();                   //驱动电机初始化(DRV8701E: 左 P33.13/P33.12, 右 P33.11/P33.5)
    ips200_init(IPS200_TYPE);       //屏幕初始化
    Menu_Init();                    //菜单初始化
    encoder_app_init();             //后轮编码器初始化(左:TIM5 P21_7/P21_6, 右:TIM2 P33_7/P33_6) —— 必须在 MyEncoder_Init 之前, 否则 T5 init 会破坏同块 T6 配置
    MyEncoder_Init();               //编码器初始化(旋转编码器 TIM6 P20_3/P20_0)
    my_key_init(20);                //按键初始化


    pit_ms_init(CCU60_CH0, 20);     //按键扫描中断初始化:菜单的
    pit_ms_init(CCU60_CH1, 2);      //旋转编码器采样中断初始化(提升响应速度)
    pit_ms_init(CCU61_CH0, ENCODER_PIT_PERIOD_MS); //后轮编码器+踏板/电机周期任务(50ms, 方案二)

    ips200_set_color(RGB565_WHITE, RGB565_BLACK);
    ips200_clear();

    Beep_MediumRing(); //上电蜂鸣


    // 此处编写用户代码 例如外设初始化代码等
    cpu_wait_event_ready();         // 等待所有核心初始化完毕
    while (TRUE)
    {
        // 此处编写需要循环执行的代码
        MENU_RUN(); 

        // 此处编写需要循环执行的代码
    }
}
#pragma section all restore
// **************************** 代码区域 ****************************
