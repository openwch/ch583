ENTRY( _start )

MEMORY
{
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 4K
	RAM (xrw) : ORIGIN = 0x20002000, LENGTH = 24K
}

    PROVIDE( __global_pointer$ = 0x20002000 );

SECTIONS
{
	.init :
	{
		_sinit = .;
		. = ALIGN(4);
		KEEP(*(SORT_NONE(.init)))
		KEEP(*(.ImageFlag))
		KEEP(*(.ImageFlag.*))
		. = ALIGN(4);
		_einit = .;
	} >FLASH AT>FLASH
	
}



