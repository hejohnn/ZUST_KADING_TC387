#ifndef PID_H
#define PID_H

#include "MyheadFile.h"
#include "MyEncoder.h"

#define motor_speed_Left_Kp  9.0
#define motor_speed_Left_Ki  0.185
#define motor_speed_Left_Kd  0.8

#define motor_speed_Right_Kp  9.0
#define motor_speed_Right_Ki  0.185
#define motor_speed_Right_Kd  0.85

#define turn_speed_Kp   1.61
#define turn_speed_Ki   0.00
#define turn_speed_Kd   0.57

#define Turn_Kp 2.5
#define Turn_Ki 0
#define Turn_Kd 0.57



#define straight_speed_Kp   0.2
#define straight_speed_Ki   0.00
#define straight_speed_Kd   11

typedef struct {
    float target_val;             //Ŀ��ֵ���൱��SetValue
    float Kp;                       //����ϵ��Proportional
    float Ki;                       //����ϵ��Integral
    float Kd;                       //΢��ϵ��Derivative
    float Ek;                       //��ǰ���
    float last_Ek;                      //ǰһ����� e(k-1)
    float Ek_sum;                      //������
    float OUT;                      //PID���
    float OUT1;
    float P_Out;
    float I_Out;
    float D_Out;
    float I_Max;
    unsigned short OLS_Order;               //最小二乘法求微分样本数
} PID_IncTypeDef;

// 添加全局变量声明
extern float motor_speed_l;
extern float motor_speed_r;

extern PID_IncTypeDef Motor_Speed_PID_Left;
extern PID_IncTypeDef Motor_Speed_PID_Right;
extern PID_IncTypeDef Turn_Speed_PID;
extern PID_IncTypeDef Straight_Speed_PID;
extern PID_IncTypeDef Speed_PID;
extern PID_IncTypeDef Angle_PID;
extern PID_IncTypeDef Angle_AccPID;

extern PID_IncTypeDef Turnout_PID;
extern PID_IncTypeDef Angleroll_PID;
extern PID_IncTypeDef Z_Angle_speed_PID;
extern PID_IncTypeDef Yaw_Angle_PID;

extern PID_IncTypeDef UB_Angle_PID;
extern PID_IncTypeDef UB_Angle_AccPID;
extern PID_IncTypeDef UB_Speed_PID;
extern PID_IncTypeDef Auto_Cornering_PID;

extern PID_IncTypeDef Temp_Speed_PID;
extern PID_IncTypeDef Temp_Angle_PID;
extern PID_IncTypeDef Temp_Angle_AccPID;
extern PID_IncTypeDef Temp_Turnout_PID;
extern PID_IncTypeDef Temp_Angleroll_PID;
extern PID_IncTypeDef Temp_Z_Angle_speed_PID;
extern PID_IncTypeDef Temp_Yaw_Angle_PID;

/*****************************************************函数部分**************************************************************/
void PID_Inc_Init(PID_IncTypeDef *sptr, float kp, float ki, float kd);
float Positional_PID(PID_IncTypeDef *PID, float SetValue, float ActualValue, float Max_I);
void PID_clear(PID_IncTypeDef *sptr);
void PID_param_init(void);
void set_pid_target(PID_IncTypeDef *pid, float temp_val);
float get_pid_target(PID_IncTypeDef *pid);
void set_p_i_d(PID_IncTypeDef *pid, float p, float i, float d);
float Incremental_PID(PID_IncTypeDef *PID, float SetValue, float ActualValue);
float Changing_Integration_Rate_Positional_PID(PID_IncTypeDef *PID, float SetValue, float ActualValue, float Max_I, float Coef_A, float Coef_B);
float Speed_Changing_Integration_Rate_Positional_PID(PID_IncTypeDef *PID, float SetValue, float ActualValue, float Max_I, float Coef_A, float Coef_B);

#endif
