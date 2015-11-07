1. After a new cross-compiler build and/or moving to a different linux box, if the OS crashes at boot then check the generated MOE.aout ELF file 
   using elfhacker tool. Ensure that FPU init and MOS Main assembly code is put in the first .text section. .init, .fini sections come after that
	 If they aren't in that order then possibly the kernel.ld linker script needs to change or the order in which these object files are specified
	 as parameter to g++ needs to change
