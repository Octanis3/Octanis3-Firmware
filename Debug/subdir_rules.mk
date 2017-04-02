################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware" --include_path="/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

nestbox_init.obj: ../nestbox_init.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware" --include_path="/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="nestbox_init.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

build-2104654033:
	@$(MAKE) -Onone -f subdir_rules.mk build-2104654033-inproc

build-2104654033-inproc: ../nestbox_rtos.cfg
	@echo 'Building file: $<'
	@echo 'Invoking: XDCtools'
	"/Applications/ti/xdctools_3_32_00_06_core/xs" --xdcpath="/Applications/ti/tirtos_msp43x_2_20_00_06/packages;/Applications/ti/tirtos_msp43x_2_20_00_06/products/bios_6_46_00_23/packages;/Applications/ti/tirtos_msp43x_2_20_00_06/products/tidrivers_msp43x_2_20_00_08/packages;/Applications/ti/tirtos_msp43x_2_20_00_06/products/uia_2_00_06_52/packages;/Applications/ti/ccsv7/ccs_base;" xdc.tools.configuro -o configPkg -t ti.targets.msp430.elf.MSP430X -p ti.platforms.msp430:MSP430FR5969 -r release -c "/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS" --compileOptions "-vmspx --data_model=restricted --use_hw_mpy=F5 --include_path=\"/Applications/ti/ccsv7/ccs_base/msp430/include\" --include_path=\"/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware\" --include_path=\"/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx\" --include_path=\"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include\" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40  " "$<"
	@echo 'Finished building: $<'
	@echo ' '

configPkg/linker.cmd: build-2104654033 ../nestbox_rtos.cfg
configPkg/compiler.opt: build-2104654033
configPkg/: build-2104654033


