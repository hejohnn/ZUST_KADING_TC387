################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/Uart_rs232/Uart_rs232.c" 

COMPILED_SRCS += \
"code/Uart_rs232/Uart_rs232.src" 

C_DEPS += \
"./code/Uart_rs232/Uart_rs232.d" 

OBJS += \
"code/Uart_rs232/Uart_rs232.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/Uart_rs232/Uart_rs232.src":"../code/Uart_rs232/Uart_rs232.c" "code/Uart_rs232/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/Uart_rs232/Uart_rs232.o":"code/Uart_rs232/Uart_rs232.src" "code/Uart_rs232/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-Uart_rs232

clean-code-2f-Uart_rs232:
	-$(RM) ./code/Uart_rs232/Uart_rs232.d ./code/Uart_rs232/Uart_rs232.o ./code/Uart_rs232/Uart_rs232.src

.PHONY: clean-code-2f-Uart_rs232

