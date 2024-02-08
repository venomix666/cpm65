MEMORY {
    zp : ORIGIN = 0x00, LENGTH = 0xdd
    ram (rw) : ORIGIN = 0xa000, LENGTH = 0xA00
}

SECTIONS {
	.zp : {
		*(.zp .zp.*)
		__ZP0_START__ = .;
		__ZP0_END__ = 0xdd;
	} >zp

	.text : { *(.text .text.*) } >ram
	.data : { *(.data .data.* .rodata .rodata.*) } > ram
	.noinit (NOLOAD) : {
		*(.noinit .noinit.*)
		. = ALIGN(256);
        __TPA1_START__ = 0xB000;
        __TPA1_END__ = 0xE000;

		__TPA0_START__ = 0x0500;
		__TPA0_END__   = 0x8000;

	} >ram
}

OUTPUT_FORMAT {
	SHORT(_start) TRIM(ram)
}

