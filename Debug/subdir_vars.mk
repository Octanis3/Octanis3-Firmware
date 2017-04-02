################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../nestbox_rtos.cfg 

CMD_SRCS += \
../nestbox_memory_map.cmd 

C_SRCS += \
../main.c \
../nestbox_init.c 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_MISC_DIRS += \
./configPkg/ 

C_DEPS += \
./main.d \
./nestbox_init.d 

GEN_OPTS += \
./configPkg/compiler.opt 

OBJS += \
./main.obj \
./nestbox_init.obj 

GEN_MISC_DIRS__QUOTED += \
"configPkg/" 

OBJS__QUOTED += \
"main.obj" \
"nestbox_init.obj" 

C_DEPS__QUOTED += \
"main.d" \
"nestbox_init.d" 

GEN_FILES__QUOTED += \
"configPkg/linker.cmd" \
"configPkg/compiler.opt" 

C_SRCS__QUOTED += \
"../main.c" \
"../nestbox_init.c" 


