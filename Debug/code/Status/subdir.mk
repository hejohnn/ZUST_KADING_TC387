################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/Status/Status.c" 

COMPILED_SRCS += \
"code/Status/Status.src" 

C_DEPS += \
"./code/Status/Status.d" 

OBJS += \
"code/Status/Status.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/Status/Status.src":"../code/Status/Status.c" "code/Status/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/Status/Status.o":"code/Status/Status.src" "code/Status/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-Status

clean-code-2f-Status:
	-$(RM) ./code/Status/Status.d ./code/Status/Status.o ./code/Status/Status.src

.PHONY: clean-code-2f-Status

