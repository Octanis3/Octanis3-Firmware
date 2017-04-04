################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
fw/ST95HF.obj: ../fw/ST95HF.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware" --include_path="/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="fw/ST95HF.d" --obj_directory="fw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

fw/lightbarrier.obj: ../fw/lightbarrier.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware" --include_path="/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="fw/lightbarrier.d" --obj_directory="fw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

fw/rfid_reader.obj: ../fw/rfid_reader.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware" --include_path="/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="fw/rfid_reader.d" --obj_directory="fw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

fw/user_button.obj: ../fw/user_button.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv7/ccs_base/msp430/include" --include_path="/Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware" --include_path="/Applications/ti/tirtos_msp43x_2_20_00_06/products/msp430_driverlib_2_70_01_01a/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.0.LTS/include" --advice:power=all --advice:power_severity=suppress --advice:hw_config=all --define=__MSP430FR5969__ --define=_MPU_ENABLE --define=ccs --define=MSP430WARE -g --printf_support=minimal --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --abi=eabi --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="fw/user_button.d" --obj_directory="fw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


