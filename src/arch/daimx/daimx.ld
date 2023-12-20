MEMORY {
    zp : ORIGIN = 0, LENGTH = 0xd0
    ram (rw) : ORIGIN = 0xa000, LENGTH = 0xA00
}

SECTIONS {
	.zp : {
		*(.zp .zp.*)
		__USERZEROPAGE_START__ = .;
		__USERZEROPAGE_END__ = 0xd0;
	} >zp

	.text : { *(.text .text.*) } >ram
	.data : { *(.data .data.* .rodata .rodata.*) } > ram
	.noinit (NOLOAD) : {
		*(.noinit .noinit.*)
		.=ALIGN(256);
		__TPA0_START__ = 0x0700;
		__TPA0_END__   = 0x7F00;

	} >ram
}

OUTPUT_FORMAT {
	SHORT (_start) TRIM(ram)
}

