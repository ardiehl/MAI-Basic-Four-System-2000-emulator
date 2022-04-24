#NO_APP
	.file	"test.c"
	.text
	.align	2
	.globl	main
	.type	main, @function
main:
	link.w %fp,#-4
	clr.l -4(%fp)
	jra .L2
.L3:
	addq.l #1,-4(%fp)
.L2:
	moveq #9,%d0
	cmp.l -4(%fp),%d0
	jge .L3
	unlk %fp
	rts
	.size	main, .-main
	.ident	"GCC: (GNU) 4.9.1 20140717 (Red Hat Cross 4.9.1-1)"
	.section	.note.GNU-stack,"",@progbits
