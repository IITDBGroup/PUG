################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../src/mem_manager/mem_mgr.o 

C_SRCS += \
../src/mem_manager/mem_mgr.c 

OBJS += \
./src/mem_manager/mem_mgr.o 

C_DEPS += \
./src/mem_manager/mem_mgr.d 


# Each subdirectory must supply rules for building sources it contributes
src/mem_manager/mem_mgr.o: /Users/xun/Documents/database_project/p1/provenance-rewriter-prototype/src/mem_manager/mem_mgr.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

