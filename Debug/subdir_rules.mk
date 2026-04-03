################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/ccs20.5/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/Debug" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/icode" -gdwarf-3 -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-1792117860: ../shaoxing_D_question.syscfg
	@echo 'SysConfig - building file: "$<"'
	"D:/ccs20.5/sysconfig_1.26.2/sysconfig_cli.bat" -s "D:/ccs20.5/mspm0_sdk_2_10_00_04/.metadata/product.json" --script "C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/shaoxing_D_question.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-1792117860 ../shaoxing_D_question.syscfg
device.opt: build-1792117860
device.cmd.genlibs: build-1792117860
ti_msp_dl_config.c: build-1792117860
ti_msp_dl_config.h: build-1792117860
Event.dot: build-1792117860

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/ccs20.5/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/Debug" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/icode" -gdwarf-3 -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: D:/ccs20.5/mspm0_sdk_2_10_00_04/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/ccs20.5/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/Debug" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/icode" -gdwarf-3 -Wall -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


