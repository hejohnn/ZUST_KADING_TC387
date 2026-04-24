################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/Turn/Turn.c" 

COMPILED_SRCS += \
"code/Turn/Turn.src" 

C_DEPS += \
"./code/Turn/Turn.d" 

OBJS += \
"code/Turn/Turn.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/Turn/Turn.src":"../code/Turn/Turn.c" "code/Turn/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/Turn/Turn.o":"code/Turn/Turn.src" "code/Turn/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-Turn

clean-code-2f-Turn:
	-$(RM) ./code/Turn/Turn.d ./code/Turn/Turn.o ./code/Turn/Turn.src

.PHONY: clean-code-2f-Turn

