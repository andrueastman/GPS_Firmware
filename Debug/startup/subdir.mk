################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32.s 

OBJS += \
./startup/startup_stm32.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/HAL_Driver/Inc/Legacy" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/inc" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/CMSIS/device" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/CMSIS/core" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/HAL_Driver/Inc" -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


