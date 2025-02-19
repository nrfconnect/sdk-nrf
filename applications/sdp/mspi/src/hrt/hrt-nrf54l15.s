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
	addi	sp,sp,-16
	sw	s0,12(sp)
	sw	s1,8(sp)
	sw	a2,0(sp)
	beq	a5,zero,.L1
	li	a4,32
	div	a4,a4,a1
	lui	t2,%hi(xfer_shift_ctrl)
	addi	a2,t2,%lo(xfer_shift_ctrl)
	lbu	s1,2(a2)
	lbu	a5,1(a2)
	sb	a1,2(a2)
	lbu	a2,3(a2)
	li	t1,3145728
	slli	a5,a5,8
	slli	a2,a2,20
	and	a2,a2,t1
	andi	a5,a5,1792
	or	a5,a5,a2
	li	t1,126976
	slli	a2,a1,12
	and	a2,a2,t1
	or	a5,a5,a2
	addi	a4,a4,-1
	andi	a4,a4,0xff
	sb	a4,%lo(xfer_shift_ctrl)(t2)
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
	li	t1,0
	addi	t0,t2,%lo(xfer_shift_ctrl)
.L3:
	lw	a5,4(a0)
	bltu	t1,a5,.L14
.L1:
	lw	s0,12(sp)
	lw	s1,8(sp)
	addi	sp,sp,16
	jr	ra
.L14:
	lw	a5,4(a0)
	li	a4,1
	sub	a5,a5,t1
	beq	a5,a4,.L4
	li	a4,2
	beq	a5,a4,.L5
.L6:
	lw	a4,0(a0)
	li	a5,0
	beq	a4,zero,.L7
	lw	a5,0(a0)
	slli	a4,t1,2
	add	a5,a5,a4
	lw	a5,0(a5)
	j	.L7
.L4:
	lbu	a2,1(t0)
	lbu	a5,2(t0)
	li	s0,126976
	slli	a2,a2,8
	slli	a5,a5,12
	and	a5,a5,s0
	andi	a2,a2,1792
	or	a2,a2,a5
	lbu	a4,8(a0)
	lbu	a5,3(t0)
	li	s0,3145728
	addi	a4,a4,-1
	slli	a5,a5,20
	andi	a4,a4,0xff
	and	a5,a5,s0
	sb	a4,%lo(xfer_shift_ctrl)(t2)
	or	a5,a2,a5
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	lw	a5,12(a0)
.L7:
	beq	a1,s1,.L8
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
	mv	s1,a1
.L8:
	lbu	a4,16(a0)
	andi	a2,a4,0xff
	beq	a4,zero,.L10
	li	a4,1
	bne	a2,a4,.L11
 #APP
	csrw 3017, a5
 #NO_APP
.L11:
	bne	t1,zero,.L12
	lw	a5,0(sp)
	lbu	a5,0(a5)
	bne	a5,zero,.L12
	mv	a5,a3
	bne	a3,zero,.L13
	li	a5,1
.L13:
	slli	a3,a5,16
	srli	a3,a3,16
 #APP
	csrw 2005, a3
 #NO_APP
	lw	a5,0(sp)
	li	a4,1
	sb	a4,0(a5)
.L12:
	addi	t1,t1,1
	j	.L3
.L5:
	lbu	a2,1(t0)
	lbu	a5,2(t0)
	li	s0,126976
	slli	a2,a2,8
	slli	a5,a5,12
	and	a5,a5,s0
	andi	a2,a2,1792
	or	a2,a2,a5
	lbu	a4,9(a0)
	lbu	a5,3(t0)
	li	s0,3145728
	addi	a4,a4,-1
	slli	a5,a5,20
	andi	a4,a4,0xff
	and	a5,a5,s0
	sb	a4,%lo(xfer_shift_ctrl)(t2)
	or	a5,a2,a5
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
	.section	.text.hrt_tx_rx.constprop.0,"ax",@progbits
	.align	1
	.type	hrt_tx_rx.constprop.0, @function
hrt_tx_rx.constprop.0:
	lw	a4,0(a0)
	mv	a5,a1
	lw	a1,4(a0)
	beq	a1,zero,.L19
	li	a1,8
	div	a1,a1,a5
	li	t1,126976
	slli	a5,a5,12
	and	a5,a5,t1
	lw	a4,0(a4)
	addi	a1,a1,-1
	andi	a1,a1,63
	or	a1,a1,a5
	li	a5,2097152
	addi	a5,a5,1024
	or	a1,a1,a5
 #APP
	csrw 3019, a1
 #NO_APP
	li	t1,65536
	li	a5,0
	li	a1,-16777216
	li	t0,1
	add	a3,a3,t1
.L21:
	lw	t1,4(a0)
	bltu	a5,t1,.L26
.L19:
	ret
.L26:
	lbu	t1,16(a0)
	andi	t2,t1,0xff
	beq	t1,zero,.L22
	bne	t2,t0,.L23
	and	t1,a4,a1
 #APP
	csrw 3017, t1
 #NO_APP
.L23:
	slli	a4,a4,8
	bne	a5,zero,.L24
	beq	a2,zero,.L24
 #APP
	csrw 2002, a3
 #NO_APP
.L25:
	addi	a5,a5,1
	j	.L21
.L22:
	and	t1,a4,a1
 #APP
	csrw 3016, t1
 #NO_APP
	j	.L23
.L24:
 #APP
	csrr t1, 3018
 #NO_APP
	j	.L25
	.size	hrt_tx_rx.constprop.0, .-hrt_tx_rx.constprop.0
	.section	.text.hrt_write,"ax",@progbits
	.align	1
	.globl	hrt_write
	.type	hrt_write, @function
hrt_write:
	addi	sp,sp,-16
	sw	s0,8(sp)
	sw	ra,12(sp)
	sw	s1,4(sp)
	mv	s0,a0
	sb	zero,3(sp)
 #APP
	csrr s1, 3008
 #NO_APP
	lhu	a5,90(a0)
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,0
	addi	a4,a0,4
	li	a3,4
.L33:
	lw	a2,0(a4)
	bne	a2,zero,.L32
	addi	a5,a5,1
	andi	a5,a5,0xff
	addi	a4,a4,20
	bne	a5,a3,.L33
	li	a5,3
.L32:
	li	a4,1
	beq	a5,a4,.L34
	li	a4,3
	beq	a5,a4,.L35
	li	a4,0
	bne	a5,zero,.L36
	lbu	a4,80(s0)
.L36:
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
	beq	a3,a2,.L37
	li	a2,2
	beq	a3,a2,.L38
	li	a5,32
	div	a5,a5,a4
	j	.L54
.L34:
	lbu	a4,81(s0)
	j	.L36
.L35:
	lbu	a4,83(s0)
	j	.L36
.L37:
	lbu	a5,8(a5)
.L54:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L41
 #APP
	csrc 3008, a5
 #NO_APP
.L42:
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
	bne	a5,zero,.L43
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	slli	s1,s1,1
	slli	s1,s1,16
	srli	s1,s1,16
 #APP
	csrw 3012, s1
	csrw 2000, 0
 #NO_APP
.L44:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,88(s0)
	bne	a5,zero,.L31
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L47
 #APP
	csrs 3008, a5
 #NO_APP
.L31:
	lw	ra,12(sp)
	lw	s0,8(sp)
	lw	s1,4(sp)
	addi	sp,sp,16
	jr	ra
.L38:
	lbu	a5,9(a5)
	j	.L54
.L41:
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L42
.L43:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L43
 #APP
	csrw 2000, 0
 #NO_APP
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	lbu	a5,94(s0)
	li	a4,1
	bne	a5,a4,.L45
	lbu	a4,86(s0)
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L44
.L45:
	li	a3,3
	bne	a5,a3,.L44
	lbu	a5,86(s0)
	sll	a4,a4,a5
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrs 3008, a4
 #NO_APP
	j	.L44
.L47:
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L31
	.size	hrt_write, .-hrt_write
	.section	.text.hrt_read,"ax",@progbits
	.align	1
	.globl	hrt_read
	.type	hrt_read, @function
hrt_read:
	addi	sp,sp,-12
	sw	s0,4(sp)
	sw	ra,8(sp)
	sw	s1,0(sp)
	lbu	s1,83(a0)
	lbu	a5,89(a0)
	lbu	a4,87(a0)
	mv	s0,a0
	andi	s1,s1,0xff
	bne	a5,zero,.L56
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
.L57:
	lbu	a4,83(s0)
	li	a5,1
	bne	a4,a5,.L58
	lhu	a5,90(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	andi	a5,a5,-5
	slli	a5,a5,16
	srli	a5,a5,16
	sh	a5,90(s0)
	lhu	a5,90(s0)
 #APP
	csrw 3009, a5
 #NO_APP
.L58:
 #APP
	csrw 3011, 2
 #NO_APP
	li	a5,65536
	addi	a5,a5,4
 #APP
	csrw 3043, a5
	csrw 3022, 8
	csrw 2000, 2
	csrw 2001, 2
 #NO_APP
	lhu	a5,84(s0)
	slli	a5,a5,16
	srli	a5,a5,16
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
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrr a4, 2003
 #NO_APP
	slli	a5,a5,1
	slli	a4,a4,16
	addi	a5,a5,1
	srli	a4,a4,16
	slli	a5,a5,16
	or	a5,a5,a4
 #APP
	csrw 2003, a5
 #NO_APP
	lbu	a1,80(s0)
	lhu	a3,84(s0)
	li	a2,1
	mv	a0,s0
	call	hrt_tx_rx.constprop.0
	lbu	a1,81(s0)
	lhu	a3,84(s0)
	li	a2,0
	addi	a0,s0,20
	call	hrt_tx_rx.constprop.0
	lbu	a4,83(s0)
	li	a5,1
	beq	a4,a5,.L59
	lhu	a5,92(s0)
 #APP
	csrw 3009, a5
 #NO_APP
.L59:
	lbu	a4,81(s0)
	lbu	a5,83(s0)
	beq	a4,a5,.L67
	li	a5,2031616
	slli	s1,s1,16
	and	s1,s1,a5
	ori	s1,s1,4
 #APP
	csrw 3043, s1
 #NO_APP
	lbu	a4,83(s0)
	li	a5,8
	div	a5,a5,a4
	addi	a5,a5,-1
	andi	a5,a5,0xff
 #APP
	csrw 3023, a5
	csrr a5, 3018
 #NO_APP
	li	a4,4
.L61:
	lw	a3,64(s0)
	bgtu	a3,a4,.L62
 #APP
	csrw 2000, 0
	csrw 2001, 0
	csrw 3019, 0
 #NO_APP
	lbu	a4,88(s0)
	bne	a4,zero,.L63
	lbu	a4,89(s0)
	lbu	a3,87(s0)
	bne	a4,zero,.L64
	li	a4,1
	sll	a4,a4,a3
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrs 3008, a4
 #NO_APP
.L63:
	lbu	a3,81(s0)
	lbu	a4,83(s0)
	beq	a3,a4,.L65
	lw	a4,60(s0)
	andi	a3,a5,0xff
	sb	a3,0(a4)
	lw	a3,60(s0)
	srli	a4,a5,8
	andi	a4,a4,0xff
	sb	a4,1(a3)
	lw	a3,60(s0)
	srli	a4,a5,16
	andi	a4,a4,0xff
	sb	a4,2(a3)
	lw	a4,60(s0)
	srli	a5,a5,24
	sb	a5,3(a4)
.L65:
	lbu	a4,83(s0)
	li	a5,1
	bne	a4,a5,.L55
	lhu	a5,90(s0)
	ori	a5,a5,4
	sh	a5,90(s0)
	lhu	a5,90(s0)
 #APP
	csrw 3009, a5
 #NO_APP
.L55:
	lw	ra,8(sp)
	lw	s0,4(sp)
	lw	s1,0(sp)
	addi	sp,sp,12
	jr	ra
.L56:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L57
.L62:
 #APP
	csrr a2, 3018
 #NO_APP
	lw	a3,60(s0)
	srli	a2,a2,24
	add	a3,a3,a4
	sb	a2,0(a3)
	addi	a4,a4,1
	j	.L61
.L67:
	li	a4,0
	li	a5,0
	j	.L61
.L64:
	li	a4,1
	sll	a4,a4,a3
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrc 3008, a4
 #NO_APP
	j	.L63
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
