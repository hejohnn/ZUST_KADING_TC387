################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/SysTimer/SysTimer.c" 

COMPILED_SRCS += \
"code/SysTimer/SysTimer.src" 

C_DEPS += \
"./code/SysTimer/SysTimer.d" 

OBJS += \
"code/SysTimer/SysTimer.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/SysTimer/SysTimer.src":"../code/SysTimer/SysTimer.c" "code/SysTimer/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/SysTimer/SysTimer.o":"code/SysTimer/SysTimer.src" "code/SysTimer/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-SysTimer

clean-code-2f-SysTimer:
	-$(RM) ./code/SysTimer/SysTimer.d ./code/SysTimer/SysTimer.o ./code/SysTimer/SysTimer.src

.PHONY: clean-code-2f-SysTimer

