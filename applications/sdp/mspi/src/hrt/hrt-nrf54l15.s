	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.hrt_tx,"ax",@progbits
	.align	1
	.type	hrt_tx, @function
hrt_tx:
	addi	sp,sp,-24
	sw	ra,20(sp)
	sw	s0,16(sp)
	sw	s1,12(sp)
	lw	a4,4(a0)
	sw	a2,0(sp)
	sw	a3,4(sp)
	beq	a4,zero,.L1
	slli	s0,a1,12
	li	a4,126976
	and	s0,s0,a4
	li	a4,32
	div	a4,a4,a1
	mv	a5,a0
	addi	a4,a4,-1
	andi	a4,a4,63
	or	a4,a4,s0
	ori	a4,a4,1024
 #APP
	csrw 3019, a4
 #NO_APP
	li	s1,0
.L3:
	lw	a4,4(a5)
	bltu	s1,a4,.L10
.L1:
	lw	ra,20(sp)
	lw	s0,16(sp)
	lw	s1,12(sp)
	addi	sp,sp,24
	jr	ra
.L10:
	lw	a4,4(a5)
	li	a2,1
	sub	a4,a4,s1
	beq	a4,a2,.L4
	li	a2,2
	beq	a4,a2,.L5
.L6:
	lw	a4,0(a5)
	bne	a4,zero,.L8
	lw	a4,16(a5)
	sw	a5,8(sp)
	li	a0,0
	j	.L14
.L4:
	lbu	a4,8(a5)
	addi	a4,a4,-1
	andi	a4,a4,63
	or	a4,a4,s0
	ori	a4,a4,1024
 #APP
	csrw 3019, a4
 #NO_APP
	lw	a4,16(a5)
	lw	a0,12(a5)
	sw	a5,8(sp)
.L14:
	jalr	a4
.L13:
	lw	a5,8(sp)
	bne	s1,zero,.L9
	lw	a4,0(sp)
	lbu	a4,0(a4)
	bne	a4,zero,.L9
	lw	a4,4(sp)
 #APP
	csrw 2005, a4
 #NO_APP
	lw	a3,0(sp)
	li	a4,1
	sb	a4,0(a3)
.L9:
	addi	s1,s1,1
	j	.L3
.L5:
	lbu	a4,9(a5)
	addi	a4,a4,-1
	andi	a4,a4,63
	or	a4,a4,s0
	ori	a4,a4,1024
 #APP
	csrw 3019, a4
 #NO_APP
	j	.L6
.L8:
	lw	a2,16(a5)
	lw	a4,0(a5)
	slli	a1,s1,2
	sw	a5,8(sp)
	add	a4,a4,a1
	lw	a0,0(a4)
	jalr	a2
	j	.L13
	.size	hrt_tx, .-hrt_tx
	.section	.text.hrt_write,"ax",@progbits
	.align	1
	.globl	hrt_write
	.type	hrt_write, @function
hrt_write:
	addi	sp,sp,-16
	sw	s0,8(sp)
	sw	ra,12(sp)
	mv	s0,a0
	sb	zero,3(sp)
	lhu	a5,90(a0)
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,0
	addi	a4,a0,4
	li	a3,4
.L17:
	lw	a2,0(a4)
	bne	a2,zero,.L16
	addi	a5,a5,1
	andi	a5,a5,0xff
	addi	a4,a4,20
	bne	a5,a3,.L17
	li	a5,3
.L16:
	li	a4,1
	beq	a5,a4,.L18
	li	a4,3
	beq	a5,a4,.L19
	li	a4,0
	bne	a5,zero,.L20
	lbu	a4,80(s0)
.L20:
 #APP
	csrw 2000, 2
 #NO_APP
	lhu	a3,84(s0)
 #APP
	csrr a2, 2003
 #NO_APP
	li	a1,-65536
	and	a2,a2,a1
	or	a3,a3,a2
 #APP
	csrw 2003, a3
	csrw 3011, 0
 #NO_APP
	li	a2,2031616
	slli	a3,a4,16
	and	a3,a3,a2
	ori	a3,a3,4
 #APP
	csrw 3043, a3
 #NO_APP
	li	a3,20
	mul	a5,a5,a3
	li	a2,1
	add	a5,s0,a5
	lw	a3,4(a5)
	beq	a3,a2,.L21
	li	a2,2
	beq	a3,a2,.L22
	li	a5,32
	div	a5,a5,a4
	j	.L39
.L18:
	lbu	a4,81(s0)
	j	.L20
.L19:
	lbu	a4,83(s0)
	j	.L20
.L21:
	lbu	a5,8(a5)
.L39:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a4,86(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,88(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L25
 #APP
	csrc 3008, a5
 #NO_APP
.L26:
	lhu	a3,84(s0)
	lbu	a1,80(s0)
	addi	a2,sp,3
	mv	a0,s0
	call	hrt_tx
	lhu	a3,84(s0)
	lbu	a1,81(s0)
	addi	a2,sp,3
	addi	a0,s0,20
	call	hrt_tx
	lhu	a3,84(s0)
	lbu	a1,82(s0)
	addi	a2,sp,3
	addi	a0,s0,40
	call	hrt_tx
	lhu	a3,84(s0)
	lbu	a1,83(s0)
	addi	a2,sp,3
	addi	a0,s0,60
	call	hrt_tx
	lbu	a5,89(s0)
	beq	a5,zero,.L27
.L28:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L28
 #APP
	csrw 2010, 0
 #NO_APP
.L27:
	li	a5,16384
	addi	a5,a5,1
 #APP
	csrw 3019, a5
	csrw 3017, 0
	csrw 2000, 0
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L15
	lbu	a4,86(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,88(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L30
 #APP
	csrs 3008, a5
 #NO_APP
.L15:
	lw	ra,12(sp)
	lw	s0,8(sp)
	addi	sp,sp,16
	jr	ra
.L22:
	lbu	a5,9(a5)
	j	.L39
.L25:
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L26
.L30:
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L15
	.size	hrt_write, .-hrt_write
