################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../code/xf_asr/asr_audio.c" \
"../code/xf_asr/base64.c" \
"../code/xf_asr/websocket_client.c" 

COMPILED_SRCS += \
"code/xf_asr/asr_audio.src" \
"code/xf_asr/base64.src" \
"code/xf_asr/websocket_client.src" 

C_DEPS += \
"./code/xf_asr/asr_audio.d" \
"./code/xf_asr/base64.d" \
"./code/xf_asr/websocket_client.d" 

OBJS += \
"code/xf_asr/asr_audio.o" \
"code/xf_asr/base64.o" \
"code/xf_asr/websocket_client.o" 


# Each subdirectory must supply rules for building sources it contributes
"code/xf_asr/asr_audio.src":"../code/xf_asr/asr_audio.c" "code/xf_asr/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/xf_asr/asr_audio.o":"code/xf_asr/asr_audio.src" "code/xf_asr/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/xf_asr/base64.src":"../code/xf_asr/base64.c" "code/xf_asr/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/xf_asr/base64.o":"code/xf_asr/base64.src" "code/xf_asr/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"code/xf_asr/websocket_client.src":"../code/xf_asr/websocket_client.c" "code/xf_asr/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 "-fD:/work/ZUST_KADING_TC387/Debug/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Ctc38x -Y0 -N0 -Z0 -o "$@" "$<"
"code/xf_asr/websocket_client.o":"code/xf_asr/websocket_client.src" "code/xf_asr/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-code-2f-xf_asr

clean-code-2f-xf_asr:
	-$(RM) ./code/xf_asr/asr_audio.d ./code/xf_asr/asr_audio.o ./code/xf_asr/asr_audio.src ./code/xf_asr/base64.d ./code/xf_asr/base64.o ./code/xf_asr/base64.src ./code/xf_asr/websocket_client.d ./code/xf_asr/websocket_client.o ./code/xf_asr/websocket_client.src

.PHONY: clean-code-2f-xf_asr

