	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e2p0_m2p0_c2p0_zicsr2p0_zca1p0_zcb1p0_zba1p0_zbb1p0_zbc1p0_zbs1p0"
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
	sw	a2,4(sp)
	beq	a5,zero,.L1
	li	a4,32
	div	a4,a4,a1
	lui	t1,%hi(xfer_shift_ctrl)
	addi	a2,t1,%lo(xfer_shift_ctrl)
	lbu	a5,2(a2)
	sb	a1,2(a2)
	li	t0,3145728
	sw	a5,0(sp)
	lbu	a5,1(a2)
	lbu	a2,3(a2)
	slli	a5,a5,8
	slli	a2,a2,20
	and	a2,a2,t0
	andi	a5,a5,1792
	or	a5,a5,a2
	li	t0,126976
	slli	a2,a1,12
	and	a2,a2,t0
	or	a5,a5,a2
	addi	a4,a4,-1
	andi	a4,a4,0xff
	sb	a4,%lo(xfer_shift_ctrl)(t1)
	andi	a4,a4,63
	or	a5,a5,a4
 #APP
	csrw 3019, a5
 #NO_APP
	li	a4,2031616
	slli	a5,a1,16
	and	a5,a5,a4
	ori	a5,a5,4
	li	a2,0
	li	t2,1
	sw	a5,8(sp)
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
	sh2add	a5,a2,a5
	lw	a5,0(a5)
	j	.L7
.L4:
	addi	s0,t1,%lo(xfer_shift_ctrl)
	lbu	t0,1(s0)
	lbu	a4,2(s0)
	li	s1,126976
	slli	t0,t0,8
	slli	a4,a4,12
	andi	t0,t0,1792
	and	a4,a4,s1
	or	a4,t0,a4
	lbu	a5,8(a0)
	lbu	t0,3(s0)
	li	s0,3145728
	addi	a5,a5,-1
	slli	t0,t0,20
	andi	a5,a5,0xff
	and	t0,t0,s0
	sb	a5,%lo(xfer_shift_ctrl)(t1)
	or	t0,a4,t0
	andi	a5,a5,63
	or	t0,t0,a5
 #APP
	csrw 3019, t0
 #NO_APP
	lw	a5,12(a0)
.L7:
	lw	a4,0(sp)
	beq	a1,a4,.L8
.L9:
 #APP
	csrr a4, 3022
 #NO_APP
	andi	a4,a4,0xff
	bne	a4,zero,.L9
	lw	a4,8(sp)
 #APP
	csrw 3043, a4
 #NO_APP
.L8:
	lbu	a4,16(a0)
	andi	t0,a4,0xff
	beq	a4,zero,.L10
	bne	t0,t2,.L11
 #APP
	csrw 3017, a5
 #NO_APP
.L11:
	bne	a2,zero,.L12
	lw	a5,4(sp)
	lbu	a5,0(a5)
	bne	a5,zero,.L12
	maxu	a3,a3,t2
 #APP
	csrw 2005, a3
 #NO_APP
	lw	a5,4(sp)
	sb	t2,0(a5)
.L12:
	addi	a2,a2,1
	sw	a1,0(sp)
	j	.L3
.L5:
	addi	s0,t1,%lo(xfer_shift_ctrl)
	lbu	t0,1(s0)
	lbu	a4,2(s0)
	li	s1,126976
	slli	t0,t0,8
	slli	a4,a4,12
	andi	t0,t0,1792
	and	a4,a4,s1
	or	a4,t0,a4
	lbu	a5,9(a0)
	lbu	t0,3(s0)
	li	s0,3145728
	addi	a5,a5,-1
	slli	t0,t0,20
	andi	a5,a5,0xff
	and	t0,t0,s0
	sb	a5,%lo(xfer_shift_ctrl)(t1)
	or	t0,a4,t0
	andi	a5,a5,63
	or	t0,t0,a5
 #APP
	csrw 3019, t0
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
	sw	s1,4(sp)
	mv	s0,a0
	sb	zero,3(sp)
	lhu	a5,90(a0)
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,0
	li	a2,20
	li	a3,4
.L20:
	mul	a4,a5,a2
	add	a4,s0,a4
	lw	a4,4(a4)
	bne	a4,zero,.L19
	addi	a5,a5,1
	bne	a5,a3,.L20
	li	a5,3
.L21:
	lbu	a4,83(s0)
	j	.L41
.L19:
	andi	a4,a5,0xff
	li	a3,1
	beq	a4,a3,.L22
	li	a3,3
	beq	a4,a3,.L21
	bne	a4,zero,.L35
	lbu	a4,80(s0)
.L41:
	andi	a4,a4,0xff
	mv	a3,a4
.L23:
	lui	a2,%hi(xfer_shift_ctrl+2)
	sb	a4,%lo(xfer_shift_ctrl+2)(a2)
 #APP
	csrw 2000, 2
 #NO_APP
	lhu	a4,84(s0)
	zext.h	a4,a4
 #APP
	csrr a2, 2003
 #NO_APP
	li	a1,-65536
	and	a2,a2,a1
	or	a4,a4,a2
 #APP
	csrw 2003, a4
	csrw 3011, 0
 #NO_APP
	li	a2,2031616
	slli	a4,a3,16
	and	a4,a4,a2
	ori	a4,a4,4
 #APP
	csrw 3043, a4
 #NO_APP
	li	a4,20
	mul	a5,a5,a4
	li	a2,1
	add	a5,s0,a5
	lw	a4,4(a5)
	beq	a4,a2,.L24
	li	a2,2
	beq	a4,a2,.L25
	li	a5,32
	div	a5,a5,a3
	j	.L42
.L22:
	lbu	a4,81(s0)
	j	.L41
.L35:
	li	a4,0
	li	a3,0
	j	.L23
.L24:
	lbu	a5,8(a5)
.L42:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a5,88(s0)
	bne	a5,zero,.L28
	lbu	a5,86(s0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrc 3008, a5
 #NO_APP
.L29:
 #APP
	csrr s1, 3008
 #NO_APP
	lbu	a1,80(s0)
	lhu	a3,84(s0)
	addi	a2,sp,3
	mv	a0,s0
	call	hrt_tx
	lbu	a1,81(s0)
	lhu	a3,84(s0)
	addi	a2,sp,3
	addi	a0,s0,20
	call	hrt_tx
	lbu	a1,82(s0)
	lhu	a3,84(s0)
	addi	a2,sp,3
	addi	a0,s0,40
	call	hrt_tx
	lbu	a1,83(s0)
	lhu	a3,84(s0)
	addi	a2,sp,3
	addi	a0,s0,60
	call	hrt_tx
	lbu	a5,92(s0)
	bne	a5,zero,.L30
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	li	a5,131072
	addi	a5,a5,-2
	slli	s1,s1,1
	and	s1,s1,a5
 #APP
	csrw 3012, s1
	csrw 2000, 0
 #NO_APP
.L31:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L18
	lbu	a5,88(s0)
	bne	a5,zero,.L34
	lbu	a5,86(s0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrs 3008, a5
 #NO_APP
.L18:
	lw	ra,12(sp)
	lw	s0,8(sp)
	lw	s1,4(sp)
	addi	sp,sp,16
	jr	ra
.L25:
	lbu	a5,9(a5)
	j	.L42
.L28:
	lbu	a5,86(s0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L29
.L30:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L30
 #APP
	csrw 2000, 0
 #NO_APP
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	lbu	a4,92(s0)
	li	a5,1
	bne	a4,a5,.L32
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L31
.L32:
	lbu	a4,92(s0)
	li	a5,3
	bne	a4,a5,.L31
 #APP
	csrs 3008, 1
 #NO_APP
	j	.L31
.L34:
	lbu	a5,86(s0)
	bset	a5,x0,a5
	zext.h	a5,a5
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
	addi	sp,sp,-24
	sw	s0,16(sp)
	sw	ra,20(sp)
	sw	s1,12(sp)
	lbu	a5,83(a0)
	lbu	a5,83(a0)
	lbu	a4,83(a0)
	lw	s1,64(a0)
	lbu	a3,87(a0)
	lw	a2,60(a0)
	lbu	a5,88(a0)
	mv	s0,a0
	andi	a4,a4,0xff
	andi	a3,a3,0xff
	bne	a5,zero,.L44
	lbu	a5,86(a0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrc 3008, a5
 #NO_APP
.L45:
 #APP
	csrr a5, 3008
 #NO_APP
	lw	a5,4(s0)
	bne	a5,zero,.L46
	lw	a5,24(s0)
	bne	a5,zero,.L46
	lw	a5,44(s0)
	beq	a5,zero,.L47
.L46:
	sw	zero,64(s0)
	li	a5,1
	sb	a5,87(s0)
	mv	a0,s0
	sw	a2,8(sp)
	sw	a3,4(sp)
	sw	a4,0(sp)
	call	hrt_write
	lw	a2,8(sp)
	lw	a3,4(sp)
	lw	a4,0(sp)
.L47:
	sw	s1,64(s0)
	sb	a3,87(s0)
 #APP
	csrw 3022, 0
	csrw 2010, 0
 #NO_APP
	lbu	a1,83(s0)
	li	a3,1
	li	a5,4
	bleu	a1,a3,.L48
	lbu	a3,83(s0)
	beq	a3,a5,.L68
	li	a5,6
.L48:
 #APP
	csrr a3, 3009
 #NO_APP
	andn	a5,a3,a5
	zext.h	a5,a5
 #APP
	csrw 3009, a5
 #NO_APP
	li	a5,-1
 #APP
	csrw 2002, a5
 #NO_APP
	li	a5,2031616
	slli	a4,a4,16
	and	a4,a4,a5
	ori	a5,a4,4
 #APP
	csrw 3043, a5
 #NO_APP
	li	a5,1
	beq	s1,a5,.L49
	li	a5,2
	beq	s1,a5,.L50
	lbu	a3,83(s0)
	li	a5,32
	div	a5,a5,a3
	j	.L79
.L44:
	lbu	a5,86(a0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L45
.L68:
	li	a5,30
	j	.L48
.L49:
	lbu	a5,68(s0)
.L79:
	addi	a5,a5,-1
	andi	a5,a5,255
 #APP
	csrw 3023, a5
	csrw 3022, 0
	csrw 3021, 0
	csrw 3011, 2
	csrw 2000, 2
	csrw 2001, 2
 #NO_APP
	lhu	a5,84(s0)
	zext.h	a5,a5
 #APP
	csrr a3, 2003
 #NO_APP
	li	a1,-65536
	and	a3,a3,a1
	or	a5,a5,a3
 #APP
	csrw 2003, a5
 #NO_APP
	lhu	a5,84(s0)
	zext.h	a5,a5
 #APP
	csrr a3, 2003
 #NO_APP
	slli	a5,a5,1
	addi	a5,a5,1
	zext.h	a3,a3
	slli	a5,a5,16
	or	a5,a5,a3
 #APP
	csrw 2003, a5
 #NO_APP
	lhu	a5,84(s0)
	bseti	a5,a5,16
 #APP
	csrw 2002, a5
	csrw 2010, 0
	csrr a5, 3018
 #NO_APP
	li	a1,1
	addi	a3,s1,-1
	li	a0,2
	li	t1,32
.L53:
	bne	a3,zero,.L58
 #APP
	csrw 3023, 0
 #NO_APP
.L59:
 #APP
	csrr a5, 3021
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L59
 #APP
	csrw 2010, 0
 #NO_APP
	lbu	a5,92(s0)
	beq	a5,zero,.L60
	lbu	a3,92(s0)
	li	a5,2
	bne	a3,a5,.L61
.L60:
 #APP
	csrw 2010, 0
 #NO_APP
.L61:
 #APP
	csrw 2000, 0
	csrw 2001, 0
	csrr a5, 3018
 #NO_APP
	li	a3,1
	bne	s1,a3,.L62
	lbu	a2,68(s0)
	li	a3,2
	bne	a2,a3,.L62
	lbu	a2,83(s0)
	li	a3,8
	bne	a2,a3,.L63
	srli	a5,a5,16
.L81:
	sw	a5,72(s0)
 #APP
	csrw 2010, 0
	csrw 3022, 0
	csrw 3043, a4
	csrw 3011, 0
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L43
	lbu	a5,88(s0)
	bne	a5,zero,.L66
	lbu	a5,86(s0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrs 3008, a5
 #NO_APP
.L43:
	lw	ra,20(sp)
	lw	s0,16(sp)
	lw	s1,12(sp)
	addi	sp,sp,24
	jr	ra
.L50:
	lbu	a5,69(s0)
	j	.L79
.L58:
	beq	a3,a1,.L54
	beq	a3,a0,.L55
	lbu	a5,83(s0)
	div	a5,t1,a5
	j	.L80
.L54:
	lbu	a5,68(s0)
.L80:
	addi	a5,a5,-1
	andi	a5,a5,255
 #APP
	csrw 3023, a5
	csrr a5, 3018
 #NO_APP
	sw	a5,0(a2)
	addi	a3,a3,-1
	addi	a2,a2,4
	j	.L53
.L55:
	lbu	a5,69(s0)
	j	.L80
.L63:
	srli	a5,a5,24
	j	.L81
.L62:
	lbu	a2,68(s0)
	lbu	a3,83(s0)
	mul	a2,a2,a3
	li	a3,32
	sub	a3,a3,a2
	srl	a5,a5,a3
	j	.L81
.L66:
	lbu	a5,86(s0)
	bset	a5,x0,a5
	zext.h	a5,a5
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L43
	.size	hrt_read, .-hrt_read
	.section	.data.xfer_shift_ctrl,"aw"
	.align	2
	.type	xfer_shift_ctrl, @object
	.size	xfer_shift_ctrl, 4
xfer_shift_ctrl:
	.byte	31
	.byte	4
	.byte	1
	.byte	0
	.section	.note.GNU-stack,"",@progbits
