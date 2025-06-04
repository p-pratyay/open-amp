__DDR_START__ = 0x9B800000;
__IRQ_STACK_SIZE      = 0x400;
__FIQ_STACK_SIZE      = 0x400;
__SVC_STACK_SIZE      = 0x1000;
__ABORT_STACK_SIZE    = 0x400;
__UNDEFINED_STACK_SIZE = 0x400;


MEMORY
{
	R5F_VECS(rwx)  : ORIGIN = 0x00000000 , LENGTH = 0x00000040
	R5F_TCMA(rwx)  : ORIGIN = 0x00000040 , LENGTH = 0x00007FC0

	DDR_1 (rwx) : ORIGIN = 0x79100000, LENGTH = 0x80000

	DDR_IPC_RESOURCE_TABLE_LINUX (rwx) : ORIGIN =  0x9B900000, LENGTH = 0x400
	TRACE (rwx) : ORIGIN = 0x9B900400, LENGTH = 0xFFC00

	DDR_IPC_VRING_RTOS  (rwx)             : ORIGIN = 0xA0400000, LENGTH = 0x300000
	
	
}

SECTIONS
{

	.vectors : 
	{
		. = ALIGN(8); KEEP(*(.isr_vector))
	} > R5F_VECS

	.boot :
	{
		
		. = ALIGN(8); *(.boot*)
		. = ALIGN(8); *(.text.mpu*)
		. = ALIGN(8); *(.text.cache*)
		. = ALIGN(8); *(.text.boot*)
		. = ALIGN(8); *(.text.hwi*)
	} > R5F_TCMA

	.text : ALIGN(8) 
	{ 
		*(.text*) 
	} > DDR_1

	.rodata : ALIGN(8) { 
		*(.rodata*) 
	} > DDR_1

	



	.textcplusplus :
	{
		

		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)


		KEEP(*(.eh_frame*))
	} > DDR_1

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > DDR_1

	__exidx_start = .;

	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > DDR_1

	__exidx_end = .;

											/* To copy multiple ROM to RAM sections,                 ----------- this is commented 
											* uncomment .copy.table section and,
											* define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */


											/*
											.copy.table :
											{
												. = ALIGN(4);                         ----------- this is commented 
												__copy_table_start__ = .;
												LONG (__etext)
												LONG (__data_start__)
												LONG (__data_end__ - __data_start__)
												LONG (__etext2)
												LONG (__data2_start__)
												LONG (__data2_end__ - __data2_start__)
												__copy_table_end__ = .;
											} > FLASH
											*/

											/* To clear multiple BSS sections,
											* uncomment .zero.table section and,
											* define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
											/*
											.zero.table :
											{
												. = ALIGN(4);                                     ----- this is commented
												__zero_table_start__ = .;
												LONG (__bss_start__)
												LONG (__bss_end__ - __bss_start__)
												LONG (__bss2_start__)
												LONG (__bss2_end__ - __bss2_start__)
												__zero_table_end__ = .;
											} > FLASH
											*/

											/* Location counter can end up 2byte aligned with narrow Thumb code but
											__etext is assumed by startup code to be the LMA of a section in RAM     ----------- this is commented 
											which must be 4byte aligned */
	

	__etext = ALIGN (4);

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);


		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > DDR_1

	
	.resource_table : ALIGN(1024)
	{
		*(.resource_table*)
	} > DDR_IPC_RESOURCE_TABLE_LINUX

	.log_shared_mem : ALIGN(128)
	{
		*(.log_shared_mem*)
	} > TRACE
	.bss :
	{
		. = ALIGN(8);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
				. = ALIGN(8);    /* THIS WAS NOT THERE IN THE .CMD FILE*/
		__bss_end__ = .;
	} > DDR_1
	
	.heap (NOLOAD): ALIGN(8)
	{
		__end__ = .;
		PROVIDE(end = .);
		*(.heap*) 
		. = . + 0x10000;                        /* increased the static heap size  */ 
		
		__HeapLimit = .;
	} > DDR_1

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (COPY): ALIGN(8)
	{
		*(.stack*)
		/*. = . + 0x10000;*/
	} > DDR_1
	
	    /* ARM mode-specific stacks */
    .irqstack () : ALIGN(8)
	{
        . = . + __IRQ_STACK_SIZE;
        PROVIDE(__IRQ_STACK_START = . - __IRQ_STACK_SIZE);
        PROVIDE(__IRQ_STACK_END = .);
    } > DDR_1

    .fiqstack () : ALIGN(8)
	{
        . = . + __FIQ_STACK_SIZE;
        PROVIDE(__FIQ_STACK_START = . - __FIQ_STACK_SIZE);
        PROVIDE(__FIQ_STACK_END = .);
    } > DDR_1

    .svcstack () : ALIGN(8)
	{
        . = . + __SVC_STACK_SIZE;
        PROVIDE(__SVC_STACK_START = . - __SVC_STACK_SIZE);
        PROVIDE(__SVC_STACK_END = .);
    } > DDR_1

    .abortstack () : ALIGN(8) {
        . = . + __ABORT_STACK_SIZE;
        PROVIDE(__ABORT_STACK_START = . - __ABORT_STACK_SIZE);
        PROVIDE(__ABORT_STACK_END = .);
    } > DDR_1

    .undefinedstack () : ALIGN(8) {    /* NOLOAD  in the bracket () */
        . = . + __UNDEFINED_STACK_SIZE;
        PROVIDE(__UNDEFINED_STACK_START = . - __UNDEFINED_STACK_SIZE);
        PROVIDE(__UNDEFINED_STACK_END = .);
    } > DDR_1

	

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(DDR_1) + LENGTH(DDR_1);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region TCMA overflowed with stack")
	PROVIDE(__BSS_START = __bss_start__);
    PROVIDE(__BSS_END = __bss_end__);               /*----------- this is commented*/ 
    PROVIDE(__STACK_END = __stack);
}