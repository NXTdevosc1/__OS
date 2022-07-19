OUTPUT_FORMAT(elf64-x86-64)
ENTRY(KrnlEntry)

SECTIONS
{
	. = 0x14000000;
	_krnlbase = .;
	.text : ALIGN(0x1000)
	{	
		*(.text)
	}
	.data : ALIGN(0x1000)
	{
		*(.data)
	}
	.rodata : ALIGN(0x1000)
	{
		*(.rodata)
	}
	.INITDAT : ALIGN(0x1000)
	{
		InitData = .;
		*(.INITDAT)
	}
	.KSTACK : ALIGN(0x1000)
	{
		*(.KSTACK)
	}
	.GLBDSIG : ALIGN(0x1000)
	{
		*(.GLBDSIG)
	}
	.reloc : ALIGN(0x1000)
	{
		*(.reloc)
	}
	.rel : ALIGN(0x1000)
	{
		*(.rel)
	}
	.rela : ALIGN(0x1000)
	{
		*(.rela)
	}
	_partvrfy = .;
	.PRTVRFY : ALIGN(0x1000)
	{
		*(.PRTVRFY)
	}
	_parvrfy_end = .;
	.symtab : ALIGN(0x1000)
	{
		*(.symtab)
	}
	.strtab : ALIGN(0x1000)
	{
		*(.strtab)
	}
	.shstrtab : ALIGN(0x1000)
	{
		*(.shstrtab)
	}
	_bss_start = .;
	.bss : ALIGN(0x1000)
	{
		*(COMMON)
		*(.bss)
	}
	_bss_end = .;
	_krnlend = .;
}