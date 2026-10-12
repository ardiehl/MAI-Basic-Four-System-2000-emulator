| BOSS/IX start for the Green Hills C library: entry at 0. _START does what
| crt0.c does: Init_IO, args, exit(main(argc, argv, env)).
| _bx(call, d0, d1, d2) runs trap #2 with the call number in a0 and retries
| while the kernel answers -0x8000.
	.text
	.globl	_start
_start:
	movel	%sp,__sp0
	jsr	_START
	.short	0xa000
	.globl	_START
_START:
	jsr	_Init_IO
	subql	#8,%sp
	pea	%sp@(4)
	pea	%sp@(4)
	jsr	_args
	addql	#8,%sp
	movel	%sp@(4),%sp@-
	movel	%d0,%sp@-
	movel	%sp@(8),%sp@-
	jsr	_main
	lea	%sp@(20),%sp
	movel	%d0,%sp@-
	jsr	_exit
	.short	0xa000
	.globl	__bx
__bx:
	moveml	%d2-%d3,%sp@-
1:	movel	%sp@(12),%a0
	moveml	%sp@(16),%d0-%d2
	clrl	%d3
	trap	#2
	cmpl	#-0x8000,%d0
	beq	1b
	moveml	%sp@+,%d2-%d3
	rts
| _bx4(call, d0, d1, d2, d3) is _bx with a fourth argument in d3.
	.globl	__bx4
__bx4:
	moveml	%d2-%d3,%sp@-
1:	movel	%sp@(12),%a0
	moveml	%sp@(16),%d0-%d3
	trap	#2
	cmpl	#-0x8000,%d0
	beq	1b
	moveml	%sp@+,%d2-%d3
	rts
| _bxcreate(type, path, a2, a3, a4, a5, a6, a7) is the BOSS/IX file create,
| trap #0 call 0x89 with its arguments on the stack as the caller left them.
	.globl	__bxcreate
__bxcreate:
	trap	#0
	.short	0x0089
	cmpl	#-0x8000,%d0
	beq	__bxcreate
	rts
	.data
	.globl	__sp0
__sp0:	.long	0
