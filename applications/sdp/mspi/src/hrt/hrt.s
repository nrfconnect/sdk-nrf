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
	lw	a5,4(a0)
	sw	a2,0(sp)
	sw	a3,4(sp)
	beq	a5,zero,.L1
	li	a5,32
	div	a5,a5,a1
	andi	a1,a1,31
	slli	a1,a1,12
	mv	s0,a0
	addi	a5,a5,-1
	andi	a5,a5,63
	ori	a5,a5,1024
	or	a1,a5,a1
	sw	a1,8(sp)
 #APP
	csrw 3019, a1
 #NO_APP
	li	s1,0
.L3:
	lw	a5,4(s0)
	bgtu	a5,s1,.L9
.L1:
	lw	ra,20(sp)
	lw	s0,16(sp)
	lw	s1,12(sp)
	addi	sp,sp,24
	jr	ra
.L9:
	sub	a5,a5,s1
	li	a2,1
	beq	a5,a2,.L4
	li	a2,2
	beq	a5,a2,.L5
.L6:
	lw	a5,0(s0)
	slli	a2,s1,2
	add	a5,a5,a2
	lw	a0,0(a5)
	lw	a2,16(s0)
	jalr	a2
	j	.L7
.L4:
	lbu	a2,8(s0)
	lw	a5,8(sp)
	addi	a2,a2,-1
	andi	a2,a2,63
	andi	a5,a5,-64
	or	a5,a5,a2
	sw	a5,8(sp)
 #APP
	csrw 3019, a5
 #NO_APP
	lw	a5,16(s0)
	lw	a0,12(s0)
	jalr	a5
.L7:
	bne	s1,zero,.L8
	lw	a5,0(sp)
	lbu	a5,0(a5)
	bne	a5,zero,.L8
	lw	a5,4(sp)
 #APP
	csrw 2005, a5
 #NO_APP
	lw	a4,0(sp)
	li	a5,1
	sb	a5,0(a4)
.L8:
	addi	s1,s1,1
	j	.L3
.L5:
	lbu	a2,9(s0)
	lw	a5,8(sp)
	addi	a2,a2,-1
	andi	a2,a2,63
	andi	a5,a5,-64
	or	a5,a5,a2
	sw	a5,8(sp)
 #APP
	csrw 3019, a5
 #NO_APP
	j	.L6
	.size	hrt_tx, .-hrt_tx
	.section	.text.hrt_write,"ax",@progbits
	.align	1
	.globl	hrt_write
	.type	hrt_write, @function
hrt_write:
	addi	sp,sp,-20
	li	a5,16384
	sw	s0,12(sp)
	sw	ra,16(sp)
	addi	a5,a5,1
	sw	a5,4(sp)
	mv	s0,a0
	lhu	a5,70(a0)
	sb	zero,3(sp)
 #APP
	csrw 3009, a5
 #NO_APP
	lw	a5,4(a0)
	beq	a5,zero,.L15
	lbu	a3,60(a0)
 #APP
	csrw 2000, 2
 #NO_APP
	lhu	a5,64(a0)
 #APP
	csrr a4, 2003
 #NO_APP
	li	a2,-65536
	and	a4,a4,a2
	or	a5,a5,a4
 #APP
	csrw 2003, a5
	csrw 3011, 0
 #NO_APP
	li	a4,2031616
	slli	a5,a3,16
	and	a5,a5,a4
	ori	a5,a5,4
 #APP
	csrw 3043, a5
 #NO_APP
	lw	a5,4(a0)
	li	a4,1
	beq	a5,a4,.L16
	li	a4,2
	beq	a5,a4,.L17
	li	a5,32
	div	a5,a5,a3
	j	.L35
.L16:
	lbu	a5,8(a0)
.L35:
 #APP
	csrw 3022, a5
 #NO_APP
.L15:
 #APP
	csrr a4, 3008
 #NO_APP
	lbu	a5,68(s0)
	lbu	a3,66(s0)
	bne	a5,zero,.L19
	li	a5,1
	sll	a5,a5,a3
	not	a5,a5
	and	a5,a5,a4
.L36:
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
	lhu	a3,64(s0)
	lbu	a1,60(s0)
	addi	a2,sp,3
	mv	a0,s0
	call	hrt_tx
	lhu	a3,64(s0)
	lbu	a1,61(s0)
	addi	a2,sp,3
	addi	a0,s0,20
	call	hrt_tx
	lhu	a3,64(s0)
	lbu	a1,62(s0)
	addi	a2,sp,3
	addi	a0,s0,40
	call	hrt_tx
	lbu	a5,69(s0)
	beq	a5,zero,.L21
.L22:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L22
 #APP
	csrw 2010, 0
 #NO_APP
.L21:
	lw	a5,4(sp)
 #APP
	csrw 3019, a5
	csrw 3017, 0
	csrw 2000, 0
 #NO_APP
	lbu	a5,67(s0)
	bne	a5,zero,.L14
 #APP
	csrr a4, 3008
 #NO_APP
	lbu	a5,68(s0)
	lbu	a3,66(s0)
	bne	a5,zero,.L24
	li	a5,1
	sll	a5,a5,a3
	or	a5,a5,a4
.L37:
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
.L14:
	lw	ra,16(sp)
	lw	s0,12(sp)
	addi	sp,sp,20
	jr	ra
.L17:
	lbu	a5,9(a0)
	j	.L35
.L19:
	li	a5,1
	sll	a5,a5,a3
	or	a5,a5,a4
	j	.L36
.L24:
	li	a5,1
	sll	a5,a5,a3
	not	a5,a5
	and	a5,a5,a4
	j	.L37
	.size	hrt_write, .-hrt_write
