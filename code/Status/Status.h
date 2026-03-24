#ifndef STATUS_H_
#define STATUS_H_
#include "MYHEADFILE.h"

typedef struct
{
        uint16 fps;
        uint32 fps_cnt;
}FPS_Struct;
extern FPS_Struct Image_FPS_Structure;


typedef struct
{
        uint8 Island_Status1;
        uint8 Island_Status2;
        uint8 Island_Status3;
        uint8 Island_Status4;
}ISLAND_Struct;
extern ISLAND_Struct Image_ISLAND_Structure;


typedef struct
{
        uint8 Right_Down_Point_finish_flag;
        uint8 Left_Down_Point_finish_flag;
        uint8 Right_Up_Point_finish_flag;
        uint8 Left_Up_Point_finish_flag;
        uint8 Left_Strait_Found_finish_flag;
        uint8 Right_Strait_Found_finish_flag;
        uint8 Left_Circle_Point_finish_flag;
        uint8 Right_Circle_Point_finish_flag;
        uint8 L_Island_Out_Point_finish_flag;
        uint8 R_Island_Out_Point_finish_flag;
}FIGUIR_Struct;
extern FIGUIR_Struct Image_FIGUIR_Structure;











#endif
