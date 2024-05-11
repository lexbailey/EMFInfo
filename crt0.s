	.module crt0
	.globl _main

	.area _HEADER (ABS)
	.org 24200
	ld sp,#0x0000
    call _do_gs_init
	call _main
	ret

    .area _BEFORE_GS_INIT
_do_gs_init:
    .area _AFTER_GS_INIT
    ret
    .area _END_OF_PROGRAM
_end_of_program:
    .db 1
