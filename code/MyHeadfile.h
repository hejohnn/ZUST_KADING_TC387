#ifndef __MYHEADFILE_H__
#define __MYHEADFILE_H__

#include "zf_common_headfile.h"
#include "PID.h"
#include "isr.h"
#include "MyKEY.h"
#include "MyEncoder.h"
#include "menu.h"
#include "Beep.h"
#include "loudspeak.h"
#include "SYSTIMER.h"
#include "Status.h"
#include "Status.h"
#include "zf_device_ips200.h"
#include "zf_device_gnss.h"
#include "Turn.h"
#include "Remote.h"



typedef enum
{
    close_status,
    open_status,
}Status_Flag;



#define LIMIT_VAL(A, MIN, MAX) ((A) < (MIN) ? (MIN) : ((A) > (MAX) ? (MAX) : (A)))

#endif
