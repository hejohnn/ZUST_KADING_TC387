################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/Motor/Motor.c" 

COMPILED_SRCS += \
"code/Motor/Motor.src" 

C_DEPS += \
"./code/Motor/Motor.d" 

OBJS += \
"code/Motor/Motor.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/Motor/Motor.src":"../code/Motor/Motor.c" "code/Motor/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/Motor/Motor.o":"code/Motor/Motor.src" "code/Motor/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-Motor

clean-code-2f-Motor:
	-$(RM) ./code/Motor/Motor.d ./code/Motor/Motor.o ./code/Motor/Motor.src

.PHONY: clean-code-2f-Motor

