#NO_APP
	.file	"eagle-sys.c"
	.text
	.align	2
	.globl	writechar
	.type	writechar, @function
writechar:
	link.w %fp,#-8
	move.l #6291462,-4(%fp)
	move.l #6291460,-8(%fp)
	nop
.L2:
	move.l -8(%fp),%d0
	move.l %d0,%a0
	move.b (%a0),%d0
	ext.w %d0
	ext.l %d0
	moveq #4,%d1
	and.l %d1,%d0
	tst.l %d0
	jeq .L2
	move.l 8(%fp),%d0
	move.b %d0,%d1
	move.l -4(%fp),%d0
	move.l %d0,%a0
	move.b %d1,(%a0)
	nop
	unlk %fp
	rts
	.size	writechar, .-writechar
	.align	2
	.type	readchar, @function
readchar:
	link.w %fp,#0
	moveq #0,%d0
	unlk %fp
	rts
	.size	readchar, .-readchar
	.align	2
	.globl	read
	.type	read, @function
read:
	link.w %fp,#-4
	move.l 16(%fp),-4(%fp)
	jra .L7
.L8:
	jsr readchar
	move.b %d0,%d1
	move.l 12(%fp),%d0
	move.l %d0,%a0
	move.b %d1,(%a0)
	addq.l #1,12(%fp)
	subq.l #1,-4(%fp)
.L7:
	tst.l -4(%fp)
	jne .L8
	move.l 16(%fp),%d0
	unlk %fp
	rts
	.size	read, .-read
	.align	2
	.globl	lseek
	.type	lseek, @function
lseek:
	link.w %fp,#0
	moveq #0,%d0
	unlk %fp
	rts
	.size	lseek, .-lseek
	.align	2
	.globl	write
	.type	write, @function
write:
	link.w %fp,#-4
	move.l 16(%fp),-4(%fp)
	jra .L13
.L14:
	move.l 12(%fp),%d0
	move.l %d0,%a0
	move.b (%a0),%d0
	ext.w %d0
	ext.l %d0
	addq.l #1,12(%fp)
	move.l %d0,-(%sp)
	jsr writechar
	addq.l #4,%sp
	subq.l #1,-4(%fp)
.L13:
	tst.l -4(%fp)
	jne .L14
	move.l 16(%fp),%d0
	unlk %fp
	rts
	.size	write, .-write
	.align	2
	.globl	close
	.type	close, @function
close:
	link.w %fp,#0
	moveq #-1,%d0
	unlk %fp
	rts
	.size	close, .-close
	.section	.rodata
.LC0:
	.string	"Heap and stack collision\n"
	.text
	.align	2
	.globl	sbrk
	.type	sbrk, @function
sbrk:
	link.w %fp,#-4
	move.l heap_end.1705,%d0
	tst.l %d0
	jne .L19
	move.l #_end,heap_end.1705
.L19:
	move.l heap_end.1705,-4(%fp)
	move.l heap_end.1705,%d1
	move.l 8(%fp),%d0
	add.l %d1,%d0
	move.l %sp,%d1
	cmp.l %d0,%d1
	jcc .L20
	pea 25.w
	pea .LC0
	pea 1.w
	jsr write
	lea (12,%sp),%sp
.L21:
	jra .L21
.L20:
	move.l heap_end.1705,%d1
	move.l 8(%fp),%d0
	add.l %d1,%d0
	move.l %d0,heap_end.1705
	move.l -4(%fp),%d0
	unlk %fp
	rts
	.size	sbrk, .-sbrk
	.align	2
	.globl	isatty
	.type	isatty, @function
isatty:
	link.w %fp,#0
	moveq #1,%d0
	unlk %fp
	rts
	.size	isatty, .-isatty
	.align	2
	.globl	fstat
	.type	fstat, @function
fstat:
	link.w %fp,#0
	move.l 12(%fp),%d0
	move.l %d0,%a0
	move.l #8192,4(%a0)
	moveq #0,%d0
	unlk %fp
	rts
	.size	fstat, .-fstat
	.align	2
	.globl	open
	.type	open, @function
open:
	link.w %fp,#0
	moveq #0,%d0
	unlk %fp
	rts
	.size	open, .-open
	.align	2
	.globl	_exit
	.type	_exit, @function
_exit:
	link.w %fp,#0
#APP
| 105 "eagle-sys.c" 1
	trap #0x0f
| 0 "" 2
#NO_APP
	unlk %fp
	rts
	.size	_exit, .-_exit
	.align	2
	.globl	execve
	.type	execve, @function
execve:
	link.w %fp,#0
	jsr __errno
	moveq #12,%d1
	move.l %d0,%a0
	move.l %d1,(%a0)
	moveq #-1,%d0
	unlk %fp
	rts
	.size	execve, .-execve
	.align	2
	.globl	fork
	.type	fork, @function
fork:
	link.w %fp,#0
	jsr __errno
	moveq #11,%d1
	move.l %d0,%a0
	move.l %d1,(%a0)
	moveq #-1,%d0
	unlk %fp
	rts
	.size	fork, .-fork
	.align	2
	.globl	getpid
	.type	getpid, @function
getpid:
	link.w %fp,#0
	moveq #1,%d0
	unlk %fp
	rts
	.size	getpid, .-getpid
	.align	2
	.globl	kill
	.type	kill, @function
kill:
	link.w %fp,#0
	jsr __errno
	moveq #22,%d1
	move.l %d0,%a0
	move.l %d1,(%a0)
	moveq #-1,%d0
	unlk %fp
	rts
	.size	kill, .-kill
	.align	2
	.globl	link
	.type	link, @function
link:
	link.w %fp,#0
	jsr __errno
	moveq #31,%d1
	move.l %d0,%a0
	move.l %d1,(%a0)
	moveq #-1,%d0
	unlk %fp
	rts
	.size	link, .-link
	.align	2
	.globl	times
	.type	times, @function
times:
	link.w %fp,#0
	moveq #-1,%d0
	unlk %fp
	rts
	.size	times, .-times
	.align	2
	.globl	unlink
	.type	unlink, @function
unlink:
	link.w %fp,#0
	jsr __errno
	moveq #2,%d1
	move.l %d0,%a0
	move.l %d1,(%a0)
	moveq #-1,%d0
	unlk %fp
	rts
	.size	unlink, .-unlink
	.align	2
	.globl	wait
	.type	wait, @function
wait:
	link.w %fp,#0
	jsr __errno
	moveq #10,%d1
	move.l %d0,%a0
	move.l %d1,(%a0)
	moveq #-1,%d0
	unlk %fp
	rts
	.size	wait, .-wait
	.local	heap_end.1705
	.comm	heap_end.1705,4,2
	.ident	"GCC: (GNU) 4.7.3"
