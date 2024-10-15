	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.set_direction,"ax",@progbits
	.align	1
	.globl	set_direction
	.type	set_direction, @function
set_direction:
 #APP
	csrw 3009, 10
	csrw 3009, 11
	csrw 3009, 12
 #NO_APP
	ret
	.size	set_direction, .-set_direction
	.section	.text.set_output,"ax",@progbits
	.align	1
	.globl	set_output
	.type	set_output, @function
set_output:
 #APP
	csrw 3008, 10
	csrw 3009, 11
	csrw 3009, 12
 #NO_APP
	ret
	.size	set_output, .-set_output
