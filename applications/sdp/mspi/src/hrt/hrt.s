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
 #APP
	csrr a5, 3009
 #NO_APP
	ori	a5,a5,2
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3009, a5
	csrr a5, 3008
 #NO_APP
	slli	a5,a5,16
	srli	a5,a5,16
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
 #NO_APP
	lbu	a4,1(a0)
	li	a5,31
	bleu	a4,a5,.L8
.L5:
 #APP
	csrw 2000, 2
 #NO_APP
	lbu	a5,0(a0)
	andi	a5,a5,0xff
 #APP
	csrr a4, 2003
 #NO_APP
	li	a3,-65536
	and	a4,a4,a3
	or	a5,a5,a4
 #APP
	csrw 2003, a5
 #NO_APP
	lbu	a5,1(a0)
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a5,1(a0)
	addi	a5,a5,-1
	andi	a5,a5,0xff
 #APP
	csrw 3023, a5
	csrr a5, 3008
 #NO_APP
	lbu	a4,10(a0)
	andi	a5,a5,-33
	slli	a5,a5,16
	slli	a4,a4,5
	srli	a5,a5,16
	or	a5,a5,a4
 #APP
	csrw 3008, a5
 #NO_APP
	lbu	a5,0(a0)
	li	a4,3
	mul	a5,a5,a4
 #APP
	csrw 2005, a5
 #NO_APP
	li	a5,0
.L3:
	lbu	a4,8(a0)
	bgtu	a4,a5,.L6
 #APP
	csrw 3012, 0
 #NO_APP
	li	a5,65536
 #APP
	csrw 3043, a5
	csrw 3045, 0
 #NO_APP
	lbu	a5,9(a0)
	bne	a5,zero,.L7
 #APP
	csrr a5, 3008
 #NO_APP
	lbu	a4,10(a0)
	andi	a5,a5,-34
	slli	a5,a5,16
	xori	a4,a4,1
	slli	a4,a4,5
	srli	a5,a5,16
	or	a5,a5,a4
 #APP
	csrw 3008, a5
 #NO_APP
.L7:
 #APP
	csrw 2000, 0
 #NO_APP
	ret
.L4:
	lw	a4,4(a0)
	slli	t1,a5,2
	addi	a5,a5,1
	add	a4,a4,t1
	lw	a3,0(a4)
	lbu	a2,1(a0)
	lw	a4,4(a0)
	andi	a5,a5,0xff
	sub	a2,a1,a2
	add	a4,a4,t1
	sll	a3,a3,a2
	sw	a3,0(a4)
.L2:
	lbu	a4,8(a0)
	bgtu	a4,a5,.L4
	j	.L5
.L8:
	li	a5,0
	li	a1,32
	j	.L2
.L6:
	lw	a4,4(a0)
	slli	a3,a5,2
	add	a4,a4,a3
	lw	a4,0(a4)
 #APP
	csrw 3016, a4
 #NO_APP
	addi	a5,a5,1
	andi	a5,a5,0xff
	j	.L3
	.size	write_single_by_word, .-write_single_by_word
	.section	.text.write_quad_by_word,"ax",@progbits
	.align	1
	.globl	write_quad_by_word
	.type	write_quad_by_word, @function
write_quad_by_word:
 #APP
	csrr a5, 3009
 #NO_APP
	ori	a5,a5,30
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3009, a5
	csrr a5, 3008
 #NO_APP
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
	li	a5,262144
	addi	a5,a5,4
 #APP
	csrw 3043, a5
	csrw 3045, 0
	csrr a5, 1996
 #NO_APP
	andi	a5,a5,17
 #APP
	csrw 1996, a5
 #NO_APP
	lbu	a4,1(a0)
	li	a5,31
	bleu	a4,a5,.L16
.L13:
 #APP
	csrw 2000, 2
 #NO_APP
	lbu	a5,0(a0)
	andi	a5,a5,0xff
 #APP
	csrr a4, 2003
 #NO_APP
	li	a3,-65536
	and	a4,a4,a3
	or	a5,a5,a4
 #APP
	csrw 2003, a5
 #NO_APP
	lbu	a5,1(a0)
	srli	a5,a5,2
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a5,1(a0)
	srli	a5,a5,2
	addi	a5,a5,-1
	andi	a5,a5,0xff
 #APP
	csrw 3023, a5
	csrr a5, 3008
 #NO_APP
	lbu	a4,10(a0)
	andi	a5,a5,-33
	slli	a5,a5,16
	slli	a4,a4,5
	srli	a5,a5,16
	or	a5,a5,a4
 #APP
	csrw 3008, a5
 #NO_APP
	lbu	a5,0(a0)
	li	a4,3
	mul	a5,a5,a4
 #APP
	csrw 2005, a5
 #NO_APP
	li	a5,0
.L11:
	lbu	a4,8(a0)
	bgtu	a4,a5,.L14
 #APP
	csrw 3012, 0
 #NO_APP
	li	a5,262144
 #APP
	csrw 3043, a5
	csrw 3045, 0
 #NO_APP
	lbu	a5,9(a0)
	bne	a5,zero,.L15
 #APP
	csrr a5, 3008
 #NO_APP
	lbu	a4,10(a0)
	andi	a5,a5,-34
	slli	a5,a5,16
	xori	a4,a4,1
	slli	a4,a4,5
	srli	a5,a5,16
	or	a5,a5,a4
 #APP
	csrw 3008, a5
 #NO_APP
.L15:
 #APP
	csrw 2000, 0
 #NO_APP
	ret
.L12:
	lw	a4,4(a0)
	slli	t1,a5,2
	addi	a5,a5,1
	add	a4,a4,t1
	lw	a3,0(a4)
	lbu	a2,1(a0)
	lw	a4,4(a0)
	andi	a5,a5,0xff
	sub	a2,a1,a2
	add	a4,a4,t1
	sll	a3,a3,a2
	sw	a3,0(a4)
.L10:
	lbu	a4,8(a0)
	bgtu	a4,a5,.L12
	j	.L13
.L16:
	li	a5,0
	li	a1,32
	j	.L10
.L14:
	lw	a4,4(a0)
	slli	a3,a5,2
	add	a4,a4,a3
	lw	a4,0(a4)
 #APP
	csrw 3016, a4
 #NO_APP
	addi	a5,a5,1
	andi	a5,a5,0xff
	j	.L11
	.size	write_quad_by_word, .-write_quad_by_word
