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
	beq	a5,zero,.L16
	addi	sp,sp,-32
	sw	s1,20(sp)
	li	a4,32
	mv	s1,a1
	div	a4,a4,s1
	lui	t1,%hi(xfer_shift_ctrl)
	sw	s0,24(sp)
	sw	ra,28(sp)
	addi	a1,t1,%lo(xfer_shift_ctrl)
	lbu	t0,2(a1)
	lbu	a5,1(a1)
	sb	s1,2(a1)
	lbu	a1,3(a1)
	mv	s0,a0
	slli	a5,a5,8
	li	a0,3145728
	slli	a1,a1,20
	and	a1,a1,a0
	andi	a5,a5,1792
	or	a5,a5,a1
	li	a0,126976
	slli	a1,s1,12
	and	a1,a1,a0
	or	a5,a5,a1
	addi	a4,a4,-1
	andi	a4,a4,0xff
	sb	a4,%lo(xfer_shift_ctrl)(t1)
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	li	a4,2031616
	slli	a5,s1,16
	and	a5,a5,a4
	ori	a5,a5,4
	sw	a5,0(sp)
	li	a1,0
.L3:
	lw	a5,4(s0)
	bltu	a1,a5,.L11
	lw	ra,28(sp)
	lw	s0,24(sp)
	lw	s1,20(sp)
	addi	sp,sp,32
	jr	ra
.L11:
	lw	a5,4(s0)
	li	a4,1
	sub	a5,a5,a1
	beq	a5,a4,.L4
	li	a4,2
	beq	a5,a4,.L5
.L6:
	lw	a5,0(s0)
	li	a0,0
	beq	a5,zero,.L7
	lw	a5,0(s0)
	slli	a4,a1,2
	add	a5,a5,a4
	lw	a0,0(a5)
	j	.L7
.L4:
	addi	t2,t1,%lo(xfer_shift_ctrl)
	lbu	a0,1(t2)
	lbu	a5,2(t2)
	li	ra,126976
	slli	a0,a0,8
	slli	a5,a5,12
	andi	a0,a0,1792
	and	a5,a5,ra
	lbu	a4,8(s0)
	or	a5,a0,a5
	lbu	a0,3(t2)
	addi	a4,a4,-1
	li	t2,3145728
	slli	a0,a0,20
	andi	a4,a4,0xff
	and	a0,a0,t2
	sb	a4,%lo(xfer_shift_ctrl)(t1)
	or	a5,a5,a0
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	lw	a0,12(s0)
.L7:
	beq	s1,t0,.L8
.L9:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L9
	lw	a5,0(sp)
 #APP
	csrw 3043, a5
 #NO_APP
	mv	t0,s1
.L8:
	lw	a5,16(s0)
	sw	a3,16(sp)
	sw	a2,12(sp)
	sw	t0,8(sp)
	sw	a1,4(sp)
	jalr	a5
	lw	a1,4(sp)
	lw	t0,8(sp)
	lw	a2,12(sp)
	lw	a3,16(sp)
	lui	t1,%hi(xfer_shift_ctrl)
	bne	a1,zero,.L10
	lbu	a5,0(a2)
	bne	a5,zero,.L10
 #APP
	csrw 2005, a3
 #NO_APP
	li	a5,1
	sb	a5,0(a2)
.L10:
	addi	a1,a1,1
	j	.L3
.L5:
	addi	t2,t1,%lo(xfer_shift_ctrl)
	lbu	a0,1(t2)
	lbu	a5,2(t2)
	li	ra,126976
	slli	a0,a0,8
	slli	a5,a5,12
	andi	a0,a0,1792
	and	a5,a5,ra
	lbu	a4,9(s0)
	or	a5,a0,a5
	lbu	a0,3(t2)
	addi	a4,a4,-1
	li	t2,3145728
	slli	a0,a0,20
	andi	a4,a4,0xff
	and	a0,a0,t2
	sb	a4,%lo(xfer_shift_ctrl)(t1)
	or	a5,a5,a0
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	j	.L6
.L16:
	ret
	.size	hrt_tx, .-hrt_tx
	.section	.text.hrt_tx_rx.constprop.0,"ax",@progbits
	.align	1
	.type	hrt_tx_rx.constprop.0, @function
hrt_tx_rx.constprop.0:
	lw	a5,0(a0)
	lw	a4,4(a0)
	beq	a4,zero,.L30
	li	a4,126976
	slli	a1,a1,12
	addi	sp,sp,-24
	and	a1,a1,a4
	li	a4,2097152
	sw	s0,16(sp)
	sw	ra,20(sp)
	sw	s1,12(sp)
	addi	a4,a4,1031
	mv	s0,a0
	lw	a5,0(a5)
	or	a1,a1,a4
 #APP
	csrw 3019, a1
 #NO_APP
	li	s1,0
.L21:
	lw	a4,4(s0)
	bltu	s1,a4,.L24
	lw	ra,20(sp)
	lw	s0,16(sp)
	lw	s1,12(sp)
	addi	sp,sp,24
	jr	ra
.L24:
	lw	a4,16(s0)
	li	a0,-16777216
	and	a0,a5,a0
	sw	a3,8(sp)
	sw	a2,4(sp)
	sw	a5,0(sp)
	jalr	a4
	lw	a5,0(sp)
	lw	a2,4(sp)
	lw	a3,8(sp)
	slli	a5,a5,8
	bne	s1,zero,.L22
	beq	a2,zero,.L22
	li	a4,65536
	add	a4,a3,a4
 #APP
	csrw 2002, a4
 #NO_APP
.L23:
	addi	s1,s1,1
	j	.L21
.L22:
 #APP
	csrr a4, 3018
 #NO_APP
	j	.L23
.L30:
	ret
	.size	hrt_tx_rx.constprop.0, .-hrt_tx_rx.constprop.0
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
.L35:
	lw	a2,0(a4)
	bne	a2,zero,.L34
	addi	a5,a5,1
	andi	a5,a5,0xff
	addi	a4,a4,20
	bne	a5,a3,.L35
	li	a5,3
.L34:
	li	a4,1
	beq	a5,a4,.L36
	li	a4,3
	beq	a5,a4,.L37
	li	a4,0
	bne	a5,zero,.L38
	lbu	a4,80(s0)
.L38:
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
	beq	a3,a2,.L39
	li	a2,2
	beq	a3,a2,.L40
	li	a5,32
	div	a5,a5,a4
	j	.L56
.L36:
	lbu	a4,81(s0)
	j	.L38
.L37:
	lbu	a4,83(s0)
	j	.L38
.L39:
	lbu	a5,8(a5)
.L56:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L43
 #APP
	csrc 3008, a5
 #NO_APP
.L44:
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
	bne	a5,zero,.L45
	li	a5,16384
	addi	a5,a5,1
 #APP
	csrw 3019, a5
	csrw 3017, 0
	csrw 2000, 0
 #NO_APP
.L46:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,88(s0)
	bne	a5,zero,.L33
	lbu	a4,87(s0)
	li	a5,1
	sll	a5,a5,a4
	lbu	a4,89(s0)
	slli	a5,a5,16
	srli	a5,a5,16
	bne	a4,zero,.L49
 #APP
	csrs 3008, a5
 #NO_APP
.L33:
	lw	ra,12(sp)
	lw	s0,8(sp)
	addi	sp,sp,16
	jr	ra
.L40:
	lbu	a5,9(a5)
	j	.L56
.L43:
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L44
.L45:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L45
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
	bne	a5,a4,.L47
	lbu	a4,86(s0)
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L46
.L47:
	li	a3,3
	bne	a5,a3,.L46
	lbu	a5,86(s0)
	sll	a4,a4,a5
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrs 3008, a4
 #NO_APP
	j	.L46
.L49:
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L33
	.size	hrt_write, .-hrt_write
	.section	.text.hrt_read,"ax",@progbits
	.align	1
	.globl	hrt_read
	.type	hrt_read, @function
hrt_read:
	addi	sp,sp,-12
	sw	s0,4(sp)
	sw	ra,8(sp)
	lbu	a5,89(a0)
	lbu	a4,87(a0)
	mv	s0,a0
	bne	a5,zero,.L58
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
.L59:
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
	li	a5,0
.L60:
	lw	a4,64(s0)
	bltu	a5,a4,.L61
 #APP
	csrw 2000, 0
	csrw 2001, 0
	csrw 3019, 0
 #NO_APP
	lbu	a5,88(s0)
	bne	a5,zero,.L62
	lbu	a5,89(s0)
	lbu	a4,87(s0)
	bne	a5,zero,.L63
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
.L62:
	lhu	a5,90(s0)
	ori	a5,a5,4
	sh	a5,90(s0)
	lhu	a5,90(s0)
 #APP
	csrw 3009, a5
 #NO_APP
	lw	ra,8(sp)
	lw	s0,4(sp)
	addi	sp,sp,12
	jr	ra
.L58:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L59
.L61:
 #APP
	csrr a3, 3018
 #NO_APP
	lw	a4,60(s0)
	srli	a3,a3,24
	add	a4,a4,a5
	addi	a5,a5,1
	sb	a3,0(a4)
	andi	a5,a5,0xff
	j	.L60
.L63:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L62
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
