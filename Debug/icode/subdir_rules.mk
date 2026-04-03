################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
icode/%.o: ../icode/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/ccs20.5/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/Debug" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/ccs20.5/mspm0_sdk_2_10_00_04/source" -I"C:/Users/wcx12/workspace_ccstheia/2025_shaoxing_D_question/icode" -gdwarf-3 -Wall -MMD -MP -MF"icode/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


