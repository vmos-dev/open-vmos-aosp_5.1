OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
	. = 0x1101000;		/* bootstub entry in DRAM */
	.text.head : {
		_head = . ;
		*(.text.head)
		_ehead = . ;
	}
	.text :	{
		_text = .; 	/* Text */
		*(.text)
		*(.text.*)
		_etext = . ;
	}
	.rodata : {
		_rodata = . ;
		*(.rodata)	 /* read-only data */
		*(.rodata.*)
		_erodata = . ;
	}
	.data :	{
		_data = . ;
		*(.data)
		*(.data.*)
		_edata = . ;
	}
	.bss : {
		_bss = . ;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		_end = . ;
	}
}
