################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/Remote/Remote.c" 

COMPILED_SRCS += \
"code/Remote/Remote.src" 

C_DEPS += \
"./code/Remote/Remote.d" 

OBJS += \
"code/Remote/Remote.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/Remote/Remote.src":"../code/Remote/Remote.c" "code/Remote/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/Remote/Remote.o":"code/Remote/Remote.src" "code/Remote/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-Remote

clean-code-2f-Remote:
	-$(RM) ./code/Remote/Remote.d ./code/Remote/Remote.o ./code/Remote/Remote.src

.PHONY: clean-code-2f-Remote

