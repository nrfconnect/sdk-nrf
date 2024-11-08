	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.write_single_by_word,"ax",@progbits
	.align	1
	.globl	write_single_by_word
	.type	write_single_by_word, @function
write_single_by_word:
	li	a5,35
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,32
 #APP
	csrw 3008, a5
 #NO_APP
	li	a5,65536
	addi	a5,a5,4
 #APP
	csrw 3043, a5
	csrw 3045, 0
	csrr a5, 1996
 #NO_APP
	andi	a5,a5,17
 #APP
	csrw 1996, a5
	csrw 2000, 2
	csrr a5, 2003
 #NO_APP
	li	a4,-65536
	and	a5,a5,a4
	ori	a5,a5,4
 #APP
	csrw 2003, a5
	csrw 3023, 31
	csrw 3008, 0
	csrw 2002, 8
 #NO_APP
	li	a5,0
	addi	a3,a1,-1
.L2:
	bgt	a3,a5,.L3
 #APP
	csrw 3023, 31
 #NO_APP
	slli	a1,a1,2
	add	a0,a0,a1
	lw	a5,-4(a0)
 #APP
	csrw 3017, a5
 #NO_APP
	li	a5,65536
 #APP
	csrw 3043, a5
	csrw 3045, 0
	csrw 3012, 0
 #NO_APP
	li	a5,32
 #APP
	csrw 3008, a5
	csrw 2000, 0
 #NO_APP
	ret
.L3:
	slli	a4,a5,2
	add	a4,a0,a4
	lw	a4,0(a4)
 #APP
	csrw 3017, a4
 #NO_APP
	addi	a5,a5,1
	j	.L2
	.size	write_single_by_word, .-write_single_by_word
