# invoke SourceDir generated makefile for nestbox_rtos.pe430X
nestbox_rtos.pe430X: .libraries,nestbox_rtos.pe430X
.libraries,nestbox_rtos.pe430X: package/cfg/nestbox_rtos_pe430X.xdl
	$(MAKE) -f /Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware/src/makefile.libs

clean::
	$(MAKE) -f /Users/raffael/Desktop/Octanis/Octanis3/ccs_workspace3/Octanis3-Firmware/src/makefile.libs clean

