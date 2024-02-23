/**
 * Linker Script - CM7
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1); /* end of "RAM_D1" Ram type memory */

_Min_Heap_Size  = 0x1000 ; 	/* required amount of heap  */
/* ATTENTION: if we use rand() - it needs malloc! */
_Min_Stack_Size = 0x800 ; 	/* required amount of stack */

/* Memories definition */
MEMORY
{
  FLASH   (rx)   : ORIGIN = 0x08040000, LENGTH = 2048K - 0x40000    /* with Arduino Bootloader: actual start is 0x08000000 and actual length is 2048K */
  DTCMRAM (rwx)  : ORIGIN = 0x20000000 + (((166 * 4) + 7) & 0xFFFFFFF8), LENGTH = 128K - (((166 * 4) + 7) & 0xFFFFFFF8) /* with Bootloader */
  /* FLASH   (rx)   : ORIGIN = 0x08000000, LENGTH = 2048K	/* without Bootloader */
  /* DTCMRAM (rw)   : ORIGIN = 0x20000000, LENGTH = 128K	/* without Bootloader */
  
  RAM_D1 (rw)    : ORIGIN = 0x24000000, LENGTH = 512K	/* MPU config */
  RAM_D2 (rw)    : ORIGIN = 0x30000000, LENGTH = 288K	/* MPU config */
  RAM_D3 (rw)    : ORIGIN = 0x38000000, LENGTH = 64K
  BACKUPRAM (rw) : ORIGIN = 0x38800000, LENGTH = 4K
  ITCMRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K
  SDRAM (rw)     : ORIGIN = 0xC0000000, LENGTH = 8M
}

/* Sections */
SECTIONS
{
  /* The startup code into "FLASH" Rom type memory */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data into "FLASH" Rom type memory */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data into "FLASH" Rom type memory */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { 
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH
  
  .ARM : {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >FLASH

  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH
  
  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH
  
  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections into "RAM" Ram type memory */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH

  /* Uninitialized data section into "RAM" Ram type memory */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough "RAM" Ram type memory left */
  ._user_heap_stack (NOLOAD) :
  /* ._user_heap_stack : */
  {
    . = ALIGN(8);
    PROVIDE (  end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    _estack = .;
    *(.dtcmram)
    PROVIDE (  end = . );
    PROVIDE ( _end = . );
  } >DTCMRAM

  /* use the 64K ITCM RAM for fast code execution */
  /* used by the startup to initialize data */
  _siitcm = LOADADDR(ITCM);
  ITCM (NOLOAD) : 
  {
    . = ALIGN(4);
    _sitcm = .;        		/* create a global symbol at data start */
    *(.itcmram)         	/* .itcmram sections */
	*(.text.memset)			/* place string.h functions to ITCM */
	*(.text.memmove)
	*(.text.memcpy)
	*(.text.strcpy)
	*(.text.snprintf)
	*(.text.PDM_FILTER)
	*(.text.D64_GENERIC)	/* place PDM_Filter on ITCM */
    . = ALIGN(4);
    _eitcm = .;        		/* define a global symbol at data end */
  } >ITCMRAM AT> FLASH

 .d2ram (NOLOAD) : {
    . = ALIGN(4);
    *(.ramd2)						/* 288K - (32K + 1K) free: used for SPI, data buffers, for ETH transfer */
    _ramd2 = .;
  	/* the last part for ETH */
  	/* ETH_RX_BUFFER_SIZE = 1536 */
    . = ABSOLUTE(0x30037C00);		/* see also MPU config! */
    *(.RxDecripSection) 			/* 21 descriptors * 6 * 4 byte = 0x1F8 */
    
    . = ABSOLUTE(0x30037E00);
    *(.TxDecripSection)				/* 21 descriptors * 6 * 4 byte = 0x1F8 */
    								/* total: 0x400 = 1K - Device on MPU, not cachable */
    								
    . = ABSOLUTE(0x30038000);		/* 0x8000 = 32KB /1528 = 21 MTUs */
    *(.RxArraySection)				/* ETH Rx buffers, MPU: chache-able, write-through */
    
    . = ABSOLUTE(0x30040000);		/* end 0x30048000 - 0x8000 = 32KB / 1528 = 21 MTUs - for Tx */			
    *(.LwIPHeapSection) 			/* ETH Tx buffers, MPU: chache-able, write-through */
  } >RAM_D2

/* .d3ram (NOLOAD) : { */
.d3ram (NOLOAD) : {
    . = ALIGN(4);
    *(.pdm_buffer)
    *(.pcm_buffer)
    *(.ramd3)						/* not really used */
    _ramd3 = .;
  } >RAM_D3
  
 .sdram (NOLOAD) : {
 	*(.sdram)
 } > SDRAM
 
  /* Remove information from the compiler libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
