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
	lw	a5,4(a0)
	addi	sp,sp,-20
	sw	s0,16(sp)
	sw	s1,12(sp)
	sw	a2,0(sp)
	sw	a3,8(sp)
	beq	a5,zero,.L1
	li	a4,32
	div	a4,a4,a1
	lui	t0,%hi(xfer_shift_ctrl)
	addi	a3,t0,%lo(xfer_shift_ctrl)
	lbu	s0,2(a3)
	lbu	a5,1(a3)
	sb	a1,2(a3)
	lbu	a3,3(a3)
	li	a2,3145728
	slli	a5,a5,8
	slli	a3,a3,20
	and	a3,a3,a2
	andi	a5,a5,1792
	or	a5,a5,a3
	li	a2,126976
	slli	a3,a1,12
	and	a3,a3,a2
	or	a5,a5,a3
	addi	a4,a4,-1
	andi	a4,a4,0xff
	sb	a4,%lo(xfer_shift_ctrl)(t0)
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	li	a4,2031616
	slli	a5,a1,16
	and	a5,a5,a4
	ori	a5,a5,4
	sw	a5,4(sp)
	li	a2,0
	li	t2,1
	addi	t1,t0,%lo(xfer_shift_ctrl)
.L3:
	lw	a5,4(a0)
	bltu	a2,a5,.L13
.L1:
	lw	s0,16(sp)
	lw	s1,12(sp)
	addi	sp,sp,20
	jr	ra
.L13:
	lw	a5,4(a0)
	sub	a5,a5,a2
	beq	a5,t2,.L4
	li	a4,2
	beq	a5,a4,.L5
.L6:
	lw	a4,0(a0)
	li	a5,0
	beq	a4,zero,.L7
	lw	a5,0(a0)
	slli	a4,a2,2
	add	a5,a5,a4
	lw	a5,0(a5)
	j	.L7
.L4:
	lbu	a3,1(t1)
	lbu	a5,2(t1)
	li	s1,126976
	slli	a3,a3,8
	slli	a5,a5,12
	and	a5,a5,s1
	andi	a3,a3,1792
	or	a3,a3,a5
	lbu	a4,8(a0)
	lbu	a5,3(t1)
	li	s1,3145728
	addi	a4,a4,-1
	slli	a5,a5,20
	andi	a4,a4,0xff
	and	a5,a5,s1
	sb	a4,%lo(xfer_shift_ctrl)(t0)
	or	a5,a3,a5
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	lw	a5,12(a0)
.L7:
	beq	a1,s0,.L8
.L9:
 #APP
	csrr a4, 3022
 #NO_APP
	andi	a4,a4,0xff
	bne	a4,zero,.L9
	lw	a4,4(sp)
 #APP
	csrw 3043, a4
 #NO_APP
	mv	s0,a1
.L8:
	lbu	a4,16(a0)
	andi	a3,a4,0xff
	beq	a4,zero,.L10
	bne	a3,t2,.L11
 #APP
	csrw 3017, a5
 #NO_APP
.L11:
	bne	a2,zero,.L12
	lw	a5,0(sp)
	lbu	a5,0(a5)
	bne	a5,zero,.L12
	lw	a5,8(sp)
 #APP
	csrw 2005, a5
 #NO_APP
	lw	a5,0(sp)
	sb	t2,0(a5)
.L12:
	addi	a2,a2,1
	j	.L3
.L5:
	lbu	a3,1(t1)
	lbu	a5,2(t1)
	li	s1,126976
	slli	a3,a3,8
	slli	a5,a5,12
	and	a5,a5,s1
	andi	a3,a3,1792
	or	a3,a3,a5
	lbu	a4,9(a0)
	lbu	a5,3(t1)
	li	s1,3145728
	addi	a4,a4,-1
	slli	a5,a5,20
	andi	a4,a4,0xff
	and	a5,a5,s1
	sb	a4,%lo(xfer_shift_ctrl)(t0)
	or	a5,a3,a5
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	j	.L6
.L10:
 #APP
	csrw 3016, a5
 #NO_APP
	j	.L11
	.size	hrt_tx, .-hrt_tx
	.section	.text.hrt_write,"ax",@progbits
	.align	1
	.globl	hrt_write
	.type	hrt_write, @function
hrt_write:
	addi	sp,sp,-16
	sw	s0,8(sp)
	sw	ra,12(sp)
	lbu	a4,94(a0)
	sb	zero,3(sp)
	li	a5,2
	mv	s0,a0
	bne	a4,a5,.L19
	lw	a5,64(a0)
	bne	a5,zero,.L37
	lw	a5,44(a0)
	bne	a5,zero,.L38
	lw	a5,24(a0)
	bne	a5,zero,.L39
	lw	a5,4(a0)
	beq	a5,zero,.L19
	li	a5,0
.L20:
	li	a4,20
	mul	a5,a5,a4
	add	a5,s0,a5
	lbu	a4,8(a5)
	addi	a4,a4,1
	sb	a4,8(a5)
.L19:
	lhu	a5,90(s0)
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,0
	addi	a4,s0,4
	li	a3,4
.L22:
	lw	a2,0(a4)
	bne	a2,zero,.L21
	addi	a5,a5,1
	andi	a5,a5,0xff
	addi	a4,a4,20
	bne	a5,a3,.L22
	li	a5,3
.L21:
	li	a4,1
	beq	a5,a4,.L23
	li	a4,3
	beq	a5,a4,.L24
	li	a4,0
	bne	a5,zero,.L25
	lbu	a4,80(s0)
.L25:
	lui	a3,%hi(xfer_shift_ctrl+2)
	sb	a4,%lo(xfer_shift_ctrl+2)(a3)
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
	beq	a3,a2,.L26
	li	a2,2
	beq	a3,a2,.L27
	li	a5,32
	div	a5,a5,a4
	j	.L49
.L37:
	li	a5,3
	j	.L20
.L38:
	li	a5,2
	j	.L20
.L39:
	li	a5,1
	j	.L20
.L23:
	lbu	a4,81(s0)
	j	.L25
.L24:
	lbu	a4,83(s0)
	j	.L25
.L26:
	lbu	a5,8(a5)
.L49:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L30
 #APP
	csrc 3008, a5
 #NO_APP
.L31:
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
	lbu	a5,94(s0)
	bne	a5,zero,.L32
	li	a5,16384
	addi	a5,a5,1
 #APP
	csrw 3019, a5
	csrw 3017, 0
	csrw 2000, 0
 #NO_APP
.L33:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,88(s0)
	bne	a5,zero,.L18
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L36
 #APP
	csrs 3008, a5
 #NO_APP
.L18:
	lw	ra,12(sp)
	lw	s0,8(sp)
	addi	sp,sp,16
	jr	ra
.L27:
	lbu	a5,9(a5)
	j	.L49
.L30:
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L31
.L32:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L32
 #APP
	csrw 2000, 0
 #NO_APP
	li	a5,16384
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	lbu	a5,94(s0)
	li	a4,1
	bne	a5,a4,.L34
	lbu	a4,86(s0)
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L33
.L34:
	li	a3,3
	bne	a5,a3,.L33
	lbu	a5,86(s0)
	sll	a4,a4,a5
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrs 3008, a4
 #NO_APP
	j	.L33
.L36:
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L18
	.size	hrt_write, .-hrt_write
	.section	.text.hrt_read,"ax",@progbits
	.align	1
	.globl	hrt_read
	.type	hrt_read, @function
hrt_read:
	addi	sp,sp,-20
	sw	s0,12(sp)
	sw	s1,8(sp)
	sw	ra,16(sp)
	lw	a4,64(a0)
	lbu	a5,88(a0)
	li	s1,1
	sw	zero,64(a0)
	sb	s1,88(a0)
	mv	s0,a0
	sw	a4,4(sp)
	sw	a5,0(sp)
	call	hrt_write
	lw	a5,0(sp)
	lw	a4,4(sp)
	lbu	a1,83(s0)
	sb	a5,88(s0)
	lhu	a5,90(s0)
	sw	a4,64(s0)
	andi	a5,a5,-5
	slli	a5,a5,16
	srli	a5,a5,16
	sh	a5,90(s0)
 #APP
	csrw 3009, a5
	csrw 3011, 2
 #NO_APP
	li	a4,2031616
	slli	a5,a1,16
	and	a5,a5,a4
	ori	a5,a5,4
 #APP
	csrw 3043, a5
	csrw 3022, 1
	csrw 2000, 2
	csrw 2001, 2
 #NO_APP
	lhu	a5,84(s0)
 #APP
	csrr a4, 2003
 #NO_APP
	li	a3,-65536
	and	a4,a4,a3
	or	a5,a5,a4
 #APP
	csrw 2003, a5
 #NO_APP
	lhu	a5,84(s0)
	slli	a5,a5,1
	addi	a5,a5,1
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrr a4, 2003
 #NO_APP
	slli	a4,a4,16
	srli	a4,a4,16
	slli	a5,a5,16
	or	a5,a5,a4
 #APP
	csrw 2003, a5
 #NO_APP
	li	a5,126976
	slli	a1,a1,12
	and	a1,a1,a5
	li	a5,2097152
	addi	a5,a5,1055
	or	a5,a1,a5
 #APP
	csrw 3019, a5
	csrw 3017, 0
 #NO_APP
	lw	a5,64(s0)
	lw	a0,60(s0)
	beq	a5,s1,.L51
.L53:
	li	a2,2097152
	addi	t1,a0,-4
	li	a4,31
	li	a5,0
	li	t0,1
	li	t2,2
	addi	a2,a2,1024
	li	s1,65536
.L52:
	lw	a3,64(s0)
	bgtu	a3,a5,.L61
.L62:
 #APP
	csrr a3, 3021
 #NO_APP
	andi	a3,a3,0xff
	bne	a3,zero,.L62
 #APP
	csrr a3, 2003
 #NO_APP
	li	a2,-65536
	and	a3,a3,a2
	ori	a3,a3,2000
 #APP
	csrw 2003, a3
 #NO_APP
	li	a3,131072000
	addi	a3,a3,2000
 #APP
	csrw 2002, a3
	csrr a2, 3018
 #NO_APP
	li	t0,16711680
	addi	t0,t0,255
	srli	a3,a2,8
	and	a3,a3,t0
	sw	t0,0(sp)
	li	t0,-16711680
	addi	t0,t0,-256
	slli	a2,a2,8
	and	a2,a2,t0
	or	a2,a3,a2
	slli	a3,a2,16
	srli	a2,a2,16
	or	a3,a3,a2
	lbu	a2,68(s0)
	li	s1,32
	sw	t0,4(sp)
	sub	a2,s1,a2
	lw	t0,0(sp)
	sll	a2,a3,a2
	srli	a3,a2,8
	and	a3,a3,t0
	lw	t0,4(sp)
	slli	a2,a2,8
	lbu	t1,69(s0)
	and	a2,a2,t0
	or	a3,a3,a2
	slli	a2,a3,16
	srli	a3,a3,16
	sub	t2,s1,t1
	or	a3,a3,a2
	beq	s1,t1,.L63
	li	a2,1
	bleu	a5,a2,.L63
	slli	a2,a5,2
	addi	a2,a2,-8
	add	a2,a0,a2
	lw	t0,0(a2)
	sll	t1,a3,t1
	srl	a3,a3,t2
	srl	t0,t0,t2
	or	t1,t1,t0
	sw	t1,0(a2)
.L63:
	lbu	a2,68(s0)
	li	t1,3
	addi	a2,a2,-1
	sub	a2,a2,t2
	srli	a2,a2,3
	addi	a2,a2,1
	beq	a2,t1,.L64
	bgtu	a2,t1,.L65
	li	t1,1
	bne	a2,t1,.L67
	slli	a5,a5,2
	add	a0,a0,a5
	sb	a3,-4(a0)
	j	.L55
.L51:
	lbu	a4,68(s0)
	li	a5,2
	bne	a4,a5,.L53
	lhu	a5,84(s0)
	li	a4,65536
	add	a5,a5,a4
 #APP
	csrw 2002, a5
 #NO_APP
.L54:
 #APP
	csrr a5, 3021
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L54
 #APP
	csrr a5, 2003
 #NO_APP
	li	a4,-65536
	and	a5,a5,a4
	ori	a5,a5,2000
 #APP
	csrw 2003, a5
 #NO_APP
	li	a5,131072000
	addi	a5,a5,2000
 #APP
	csrw 2002, a5
	csrr a5, 3018
 #NO_APP
	lbu	a4,68(s0)
	lbu	a2,83(s0)
	lw	a3,60(s0)
	mul	a4,a4,a2
	addi	a4,a4,2
	srl	a5,a5,a4
	andi	a5,a5,0xff
	sb	a5,0(a3)
	li	a4,31
.L55:
 #APP
	csrw 2000, 0
	csrw 2001, 0
 #NO_APP
	andi	a4,a4,63
	or	a4,a4,a1
 #APP
	csrw 3019, a4
 #NO_APP
	lbu	a5,88(s0)
	bne	a5,zero,.L69
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L70
 #APP
	csrs 3008, a5
 #NO_APP
.L69:
	lhu	a5,90(s0)
	ori	a5,a5,4
	sh	a5,90(s0)
 #APP
	csrw 3009, a5
 #NO_APP
	lw	ra,16(sp)
	lw	s0,12(sp)
	lw	s1,8(sp)
	addi	sp,sp,20
	jr	ra
.L61:
	sub	a3,a3,a5
	beq	a3,t0,.L56
	li	a4,31
	bne	a3,t2,.L58
	lbu	a4,69(s0)
	j	.L77
.L56:
	lbu	a4,68(s0)
.L77:
	addi	a4,a4,-1
	andi	a4,a4,0xff
.L58:
	bne	a5,zero,.L59
	addi	a3,a4,-2
	andi	a4,a3,0xff
	andi	a3,a3,63
	or	a3,a3,a1
	or	a3,a3,a2
 #APP
	csrw 3019, a3
 #NO_APP
	lhu	a3,84(s0)
	add	a3,a3,s1
 #APP
	csrw 2002, a3
	csrr a3, 3018
 #NO_APP
.L60:
	addi	a5,a5,1
	addi	t1,t1,4
	j	.L52
.L59:
	andi	a3,a4,63
	or	a3,a3,a1
	or	a3,a3,a2
 #APP
	csrw 3019, a3
	csrr a3, 3018
 #NO_APP
	sw	a3,0(t1)
	j	.L60
.L65:
	li	t1,4
	bne	a2,t1,.L55
	slli	a5,a5,2
	add	a5,a0,a5
	sw	a3,-4(a5)
	j	.L55
.L64:
	slli	a2,a5,2
	add	a2,a0,a2
	srli	t1,a3,16
	sb	t1,-2(a2)
.L67:
	slli	a5,a5,2
	add	a5,a0,a5
	sh	a3,-4(a5)
	j	.L55
.L70:
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L69
	.size	hrt_read, .-hrt_read
	.section	.sdata.xfer_shift_ctrl,"aw"
	.align	2
	.type	xfer_shift_ctrl, @object
	.size	xfer_shift_ctrl, 4
xfer_shift_ctrl:
	.byte	31
	.byte	4
	.byte	1
	.byte	0
