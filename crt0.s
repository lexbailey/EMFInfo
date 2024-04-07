	.module crt0
	.globl	_main

	.area	_HEADER (ABS)
	.org 	0x8000
	ld	sp,#0x0000
	call	_main
	ret
