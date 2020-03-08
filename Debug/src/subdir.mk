################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/gsm.c \
../src/main.c \
../src/packet_store.c \
../src/stm32l0xx_it.c \
../src/syscalls.c \
../src/system_stm32l0xx.c \
../src/uart_gsm.c 

OBJS += \
./src/gsm.o \
./src/main.o \
./src/packet_store.o \
./src/stm32l0xx_it.o \
./src/syscalls.o \
./src/system_stm32l0xx.o \
./src/uart_gsm.o 

C_DEPS += \
./src/gsm.d \
./src/main.d \
./src/packet_store.d \
./src/stm32l0xx_it.d \
./src/syscalls.d \
./src/system_stm32l0xx.d \
./src/uart_gsm.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -DSTM32 -DSTM32L0 -DSTM32L071CZTx -DDEBUG -DSTM32L071xx -DUSE_HAL_DRIVER -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/HAL_Driver/Inc/Legacy" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/inc" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/CMSIS/device" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/CMSIS/core" -I"C:/Users/Kuzan/workspace/GPS_Project_New_MCU/HAL_Driver/Inc" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


