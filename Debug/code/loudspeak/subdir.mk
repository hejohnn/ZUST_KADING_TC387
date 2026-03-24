################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/loudspeak/loudspeak.c" 

COMPILED_SRCS += \
"code/loudspeak/loudspeak.src" 

C_DEPS += \
"./code/loudspeak/loudspeak.d" 

OBJS += \
"code/loudspeak/loudspeak.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/loudspeak/loudspeak.src":"../code/loudspeak/loudspeak.c" "code/loudspeak/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/loudspeak/loudspeak.o":"code/loudspeak/loudspeak.src" "code/loudspeak/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-loudspeak

clean-code-2f-loudspeak:
	-$(RM) ./code/loudspeak/loudspeak.d ./code/loudspeak/loudspeak.o ./code/loudspeak/loudspeak.src

.PHONY: clean-code-2f-loudspeak

