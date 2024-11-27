	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.hrt_write,"ax",@progbits
	.align	1
	.globl	hrt_write
	.type	hrt_write, @function
hrt_write:
 #APP
	csrr a3, 3008
 #NO_APP
	lbu	a5,23(a0)
	bne	a5,zero,.L2
	lbu	a5,21(a0)
	lbu	a4,21(a0)
	li	a5,1
	sll	a5,a5,a4
	not	a5,a5
	and	a5,a5,a3
.L26:
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
	li	a4,0
	li	a5,0
	li	a2,1
	li	a1,2
.L4:
	lw	a3,8(a0)
	bgtu	a3,a5,.L17
	lbu	a5,24(a0)
	beq	a5,zero,.L18
.L19:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L19
 #APP
	csrw 2010, 0
 #NO_APP
.L20:
	li	a5,65536
 #APP
	csrw 3043, a5
	csrw 3045, 0
 #NO_APP
	lbu	a5,22(a0)
	bne	a5,zero,.L21
 #APP
	csrr a3, 3008
 #NO_APP
	lbu	a5,23(a0)
	bne	a5,zero,.L22
	lbu	a5,21(a0)
	li	a4,1
	lbu	a2,21(a0)
	sll	a5,a4,a5
	not	a5,a5
	and	a5,a5,a3
	sll	a4,a4,a2
	or	a5,a5,a4
.L30:
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
.L21:
 #APP
	csrw 2000, 0
 #NO_APP
	ret
.L2:
	lbu	a5,21(a0)
	li	a4,1
	lbu	a2,21(a0)
	sll	a5,a4,a5
	not	a5,a5
	and	a5,a5,a3
	sll	a4,a4,a2
	or	a5,a5,a4
	j	.L26
.L17:
	lw	a3,8(a0)
	sub	a3,a3,a5
	beq	a3,a2,.L5
	beq	a3,a1,.L6
.L7:
	lbu	t1,20(a0)
	andi	a3,t1,0xff
	beq	t1,a2,.L12
	beq	a3,a1,.L13
	bne	a3,zero,.L11
	lw	a3,4(a0)
	add	a3,a3,a4
	lw	a3,0(a3)
	j	.L27
.L5:
	lbu	a3,12(a0)
	addi	a3,a3,-1
	andi	a3,a3,0xff
 #APP
	csrw 3023, a3
 #NO_APP
	lbu	t1,20(a0)
	andi	a3,t1,0xff
	beq	t1,a2,.L8
	beq	a3,a1,.L9
	bne	a3,zero,.L11
	lw	a3,16(a0)
.L27:
 #APP
	csrw 3012, a3
 #NO_APP
.L11:
	bne	a5,zero,.L15
	lhu	a3,0(a0)
 #APP
	csrw 2005, a3
	csrr t1, 3022
 #NO_APP
	andi	t1,t1,0xff
.L16:
 #APP
	csrr a3, 3022
 #NO_APP
	andi	a3,a3,0xff
	beq	t1,a3,.L16
.L15:
	addi	a5,a5,1
	addi	a4,a4,4
	j	.L4
.L8:
	lw	a3,16(a0)
.L29:
 #APP
	csrw 3016, a3
 #NO_APP
	j	.L11
.L9:
	lw	a3,16(a0)
.L28:
 #APP
	csrw 3017, a3
 #NO_APP
	j	.L11
.L6:
	lbu	a3,13(a0)
	addi	a3,a3,-1
	andi	a3,a3,0xff
 #APP
	csrw 3023, a3
 #NO_APP
	j	.L7
.L12:
	lw	a3,4(a0)
	add	a3,a3,a4
	lw	a3,0(a3)
	j	.L29
.L13:
	lw	a3,4(a0)
	add	a3,a3,a4
	lw	a3,0(a3)
	j	.L28
.L18:
 #APP
	csrw 3012, 0
 #NO_APP
	j	.L20
.L22:
	lbu	a5,21(a0)
	lbu	a4,21(a0)
	li	a5,1
	sll	a5,a5,a4
	not	a5,a5
	and	a5,a5,a3
	j	.L30
	.size	hrt_write, .-hrt_write
