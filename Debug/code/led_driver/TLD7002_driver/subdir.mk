################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/led_driver/TLD7002_driver/TLD7002FuncLayer.c" \
"../code/led_driver/TLD7002_driver/TLD7002_ControlLayer.c" \
"../code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.c" 

COMPILED_SRCS += \
"code/led_driver/TLD7002_driver/TLD7002FuncLayer.src" \
"code/led_driver/TLD7002_driver/TLD7002_ControlLayer.src" \
"code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.src" 

C_DEPS += \
"./code/led_driver/TLD7002_driver/TLD7002FuncLayer.d" \
"./code/led_driver/TLD7002_driver/TLD7002_ControlLayer.d" \
"./code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.d" 

OBJS += \
"code/led_driver/TLD7002_driver/TLD7002FuncLayer.o" \
"code/led_driver/TLD7002_driver/TLD7002_ControlLayer.o" \
"code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/led_driver/TLD7002_driver/TLD7002FuncLayer.src":"../code/led_driver/TLD7002_driver/TLD7002FuncLayer.c" "code/led_driver/TLD7002_driver/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/led_driver/TLD7002_driver/TLD7002FuncLayer.o":"code/led_driver/TLD7002_driver/TLD7002FuncLayer.src" "code/led_driver/TLD7002_driver/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/led_driver/TLD7002_driver/TLD7002_ControlLayer.src":"../code/led_driver/TLD7002_driver/TLD7002_ControlLayer.c" "code/led_driver/TLD7002_driver/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/led_driver/TLD7002_driver/TLD7002_ControlLayer.o":"code/led_driver/TLD7002_driver/TLD7002_ControlLayer.src" "code/led_driver/TLD7002_driver/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.src":"../code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.c" "code/led_driver/TLD7002_driver/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.o":"code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.src" "code/led_driver/TLD7002_driver/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-led_driver-2f-TLD7002_driver

clean-code-2f-led_driver-2f-TLD7002_driver:
	-$(RM) ./code/led_driver/TLD7002_driver/TLD7002FuncLayer.d ./code/led_driver/TLD7002_driver/TLD7002FuncLayer.o ./code/led_driver/TLD7002_driver/TLD7002FuncLayer.src ./code/led_driver/TLD7002_driver/TLD7002_ControlLayer.d ./code/led_driver/TLD7002_driver/TLD7002_ControlLayer.o ./code/led_driver/TLD7002_driver/TLD7002_ControlLayer.src ./code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.d ./code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.o ./code/led_driver/TLD7002_driver/TLD7002_ServiceLayer.src

.PHONY: clean-code-2f-led_driver-2f-TLD7002_driver

