################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
CR95HF_Communication.obj: ../CR95HF_Communication.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv6/ccs_base/msp430/include" --include_path="/Users/jorgezaratemen/Documents/Octanis_Nestboxes_Project/CSS_Workspace/CR95HF_control_by_MSP430FR5969_SPI/CR95HF_Control_by_MSP430FR5969_SPI_EchoTest_3/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/include" --advice:power="none" --advice:hw_config=all -g --define=__MSP430FR5969__ --define=DEPRECATED --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="CR95HF_Communication.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv6/ccs_base/msp430/include" --include_path="/Users/jorgezaratemen/Documents/Octanis_Nestboxes_Project/CSS_Workspace/CR95HF_control_by_MSP430FR5969_SPI/CR95HF_Control_by_MSP430FR5969_SPI_EchoTest_3/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/include" --advice:power="none" --advice:hw_config=all -g --define=__MSP430FR5969__ --define=DEPRECATED --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

myClocks.obj: ../myClocks.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv6/ccs_base/msp430/include" --include_path="/Users/jorgezaratemen/Documents/Octanis_Nestboxes_Project/CSS_Workspace/CR95HF_control_by_MSP430FR5969_SPI/CR95HF_Control_by_MSP430FR5969_SPI_EchoTest_3/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/include" --advice:power="none" --advice:hw_config=all -g --define=__MSP430FR5969__ --define=DEPRECATED --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="myClocks.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

myGpio.obj: ../myGpio.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv6/ccs_base/msp430/include" --include_path="/Users/jorgezaratemen/Documents/Octanis_Nestboxes_Project/CSS_Workspace/CR95HF_control_by_MSP430FR5969_SPI/CR95HF_Control_by_MSP430FR5969_SPI_EchoTest_3/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/include" --advice:power="none" --advice:hw_config=all -g --define=__MSP430FR5969__ --define=DEPRECATED --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="myGpio.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

mySPI.obj: ../mySPI.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv6/ccs_base/msp430/include" --include_path="/Users/jorgezaratemen/Documents/Octanis_Nestboxes_Project/CSS_Workspace/CR95HF_control_by_MSP430FR5969_SPI/CR95HF_Control_by_MSP430FR5969_SPI_EchoTest_3/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/include" --advice:power="none" --advice:hw_config=all -g --define=__MSP430FR5969__ --define=DEPRECATED --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="mySPI.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

unused_interrupts.obj: ../unused_interrupts.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/bin/cl430" -vmspx --data_model=restricted --use_hw_mpy=F5 --include_path="/Applications/ti/ccsv6/ccs_base/msp430/include" --include_path="/Users/jorgezaratemen/Documents/Octanis_Nestboxes_Project/CSS_Workspace/CR95HF_control_by_MSP430FR5969_SPI/CR95HF_Control_by_MSP430FR5969_SPI_EchoTest_3/driverlib/MSP430FR5xx_6xx" --include_path="/Applications/ti/ccsv6/tools/compiler/ti-cgt-msp430_15.12.1.LTS/include" --advice:power="none" --advice:hw_config=all -g --define=__MSP430FR5969__ --define=DEPRECATED --define=_MPU_ENABLE --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="unused_interrupts.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


