################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/led_driver/led_test_ctrl.c" \
"../code/led_driver/zf_device_dot_matrix_screen.c" \
"../code/led_driver/zf_device_tld7002.c" 

COMPILED_SRCS += \
"code/led_driver/led_test_ctrl.src" \
"code/led_driver/zf_device_dot_matrix_screen.src" \
"code/led_driver/zf_device_tld7002.src" 

C_DEPS += \
"./code/led_driver/led_test_ctrl.d" \
"./code/led_driver/zf_device_dot_matrix_screen.d" \
"./code/led_driver/zf_device_tld7002.d" 

OBJS += \
"code/led_driver/led_test_ctrl.o" \
"code/led_driver/zf_device_dot_matrix_screen.o" \
"code/led_driver/zf_device_tld7002.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/led_driver/led_test_ctrl.src":"../code/led_driver/led_test_ctrl.c" "code/led_driver/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/led_driver/led_test_ctrl.o":"code/led_driver/led_test_ctrl.src" "code/led_driver/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/led_driver/zf_device_dot_matrix_screen.src":"../code/led_driver/zf_device_dot_matrix_screen.c" "code/led_driver/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/led_driver/zf_device_dot_matrix_screen.o":"code/led_driver/zf_device_dot_matrix_screen.src" "code/led_driver/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/led_driver/zf_device_tld7002.src":"../code/led_driver/zf_device_tld7002.c" "code/led_driver/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/led_driver/zf_device_tld7002.o":"code/led_driver/zf_device_tld7002.src" "code/led_driver/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-led_driver

clean-code-2f-led_driver:
	-$(RM) ./code/led_driver/led_test_ctrl.d ./code/led_driver/led_test_ctrl.o ./code/led_driver/led_test_ctrl.src ./code/led_driver/zf_device_dot_matrix_screen.d ./code/led_driver/zf_device_dot_matrix_screen.o ./code/led_driver/zf_device_dot_matrix_screen.src ./code/led_driver/zf_device_tld7002.d ./code/led_driver/zf_device_tld7002.o ./code/led_driver/zf_device_tld7002.src

.PHONY: clean-code-2f-led_driver

