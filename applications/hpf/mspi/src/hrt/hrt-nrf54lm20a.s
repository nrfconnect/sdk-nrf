	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.hrt_tx.part.0,"ax",@progbits
	.align	1
	.type	hrt_tx.part.0, @function
hrt_tx.part.0:
	li	a4,32
	div	a4,a4,a1
	addi	sp,sp,-20
	sw	s0,16(sp)
	lui	s0,%hi(xfer_shift_ctrl)
	addi	t1,s0,%lo(xfer_shift_ctrl)
	lbu	a5,1(t1)
	lbu	t2,3(t1)
	li	t0,3145728
	slli	a5,a5,8
	slli	t2,t2,20
	and	t2,t2,t0
	andi	a5,a5,1792
	slli	t0,a1,12
	or	a5,a5,t2
	li	t2,126976
	and	t0,t0,t2
	or	a5,a5,t0
	sw	a2,0(sp)
	lbu	t2,2(t1)
	sw	s1,12(sp)
	sb	a1,2(t1)
	addi	a4,a4,-1
	andi	a4,a4,0xff
	andi	a2,a4,63
	sb	a4,%lo(xfer_shift_ctrl)(s0)
	or	a5,a5,a2
 #APP
	csrw 3019, a5
 #NO_APP
	lw	a5,4(a0)
	beq	a5,zero,.L1
	slli	a5,a1,16
	li	a4,2031616
	and	a5,a5,a4
	ori	a5,a5,4
	sw	a5,8(sp)
	li	t0,1
	li	a5,0
.L13:
	lw	a4,4(a0)
	sub	a4,a4,a5
	beq	a4,t0,.L3
	li	a2,2
	bne	a4,a2,.L5
	lbu	a4,3(t1)
	lbu	a2,2(t1)
	lbu	s1,1(t1)
	sw	a4,4(sp)
	slli	a2,a2,12
	li	a4,126976
	lbu	s0,9(a0)
	and	a2,a2,a4
	lw	a4,4(sp)
	slli	s1,s1,8
	andi	s1,s1,1792
	or	a2,s1,a2
	addi	s0,s0,-1
	li	s1,3145728
	slli	a4,a4,20
	andi	s0,s0,0xff
	and	a4,a4,s1
	or	a4,a2,a4
	lui	s1,%hi(xfer_shift_ctrl)
	andi	a2,s0,63
	sb	s0,%lo(xfer_shift_ctrl)(s1)
	or	a4,a4,a2
 #APP
	csrw 3019, a4
 #NO_APP
.L5:
	lw	a4,0(a0)
	li	a2,0
	beq	a4,zero,.L6
	lw	a4,0(a0)
	slli	a2,a5,2
	add	a4,a4,a2
	lw	a2,0(a4)
.L6:
	bne	a1,t2,.L8
	lbu	a4,16(a0)
	andi	s0,a4,0xff
	beq	a4,zero,.L9
.L23:
	bne	s0,t0,.L10
 #APP
	csrw 3017, a2
 #NO_APP
.L10:
	bne	a5,zero,.L11
	lw	a4,0(sp)
	lbu	a4,0(a4)
	bne	a4,zero,.L11
	mv	a4,a3
	bne	a3,zero,.L12
	li	a4,1
.L12:
	slli	a3,a4,16
	srli	a3,a3,16
 #APP
	csrw 2005, a3
 #NO_APP
	lw	a4,0(sp)
	sb	t0,0(a4)
.L11:
	lw	a4,4(a0)
	addi	a5,a5,1
	bltu	a5,a4,.L13
.L1:
	lw	s0,16(sp)
	lw	s1,12(sp)
	addi	sp,sp,20
	jr	ra
.L8:
 #APP
	csrr a4, 3022
 #NO_APP
	andi	a4,a4,0xff
	bne	a4,zero,.L8
	lw	a4,8(sp)
 #APP
	csrw 3043, a4
 #NO_APP
	lbu	a4,16(a0)
	mv	t2,a1
	andi	s0,a4,0xff
	bne	a4,zero,.L23
.L9:
 #APP
	csrw 3016, a2
 #NO_APP
	j	.L10
.L3:
	lbu	a4,3(t1)
	lbu	a2,2(t1)
	lbu	s1,1(t1)
	sw	a4,4(sp)
	slli	a2,a2,12
	li	a4,126976
	lbu	s0,8(a0)
	and	a2,a2,a4
	lw	a4,4(sp)
	slli	s1,s1,8
	andi	s1,s1,1792
	or	a2,s1,a2
	addi	s0,s0,-1
	li	s1,3145728
	slli	a4,a4,20
	andi	s0,s0,0xff
	and	a4,a4,s1
	or	a4,a2,a4
	lui	s1,%hi(xfer_shift_ctrl)
	andi	a2,s0,63
	sb	s0,%lo(xfer_shift_ctrl)(s1)
	or	a4,a4,a2
 #APP
	csrw 3019, a4
 #NO_APP
	lw	a2,12(a0)
	j	.L6
	.size	hrt_tx.part.0, .-hrt_tx.part.0
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
	li	a2,4
.L26:
	mv	a4,a5
	slli	a5,a5,2
	add	a5,a5,a4
	slli	a5,a5,2
	add	a5,s0,a5
	lw	a3,4(a5)
	addi	a5,a4,1
	bne	a3,zero,.L25
	bne	a5,a2,.L26
	li	a4,3
.L27:
	lbu	a5,83(s0)
.L63:
	andi	a5,a5,0xff
	li	a2,2031616
	slli	a3,a5,16
	and	a3,a3,a2
	ori	a3,a3,4
	mv	a2,a5
	j	.L29
.L25:
	andi	a5,a4,0xff
	li	a3,1
	beq	a5,a3,.L28
	li	a3,3
	beq	a5,a3,.L27
	beq	a5,zero,.L64
	li	a3,4
	li	a5,0
	li	a2,0
.L29:
	lui	a1,%hi(xfer_shift_ctrl+2)
	sb	a5,%lo(xfer_shift_ctrl+2)(a1)
 #APP
	csrw 2000, 2
 #NO_APP
	lhu	a5,84(s0)
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrr a1, 2003
 #NO_APP
	li	a0,-65536
	and	a1,a1,a0
	or	a5,a5,a1
 #APP
	csrw 2003, a5
	csrw 3011, 0
	csrw 3043, a3
 #NO_APP
	slli	a5,a4,2
	add	a5,a5,a4
	slli	a5,a5,2
	add	a5,s0,a5
	lw	a4,4(a5)
	li	a3,1
	beq	a4,a3,.L30
	li	a3,2
	beq	a4,a3,.L31
	li	a5,32
	div	a5,a5,a2
 #APP
	csrw 3022, a5
 #NO_APP
.L33:
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L34
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
.L35:
 #APP
	csrr s1, 3008
 #NO_APP
	lbu	a1,80(s0)
	lhu	a3,84(s0)
	lw	a5,4(s0)
	andi	a1,a1,0xff
	slli	a3,a3,16
	srli	a3,a3,16
	beq	a5,zero,.L36
	addi	a2,sp,3
	mv	a0,s0
	call	hrt_tx.part.0
.L36:
	lbu	a1,81(s0)
	lhu	a3,84(s0)
	lw	a5,24(s0)
	andi	a1,a1,0xff
	slli	a3,a3,16
	srli	a3,a3,16
	beq	a5,zero,.L37
	addi	a2,sp,3
	addi	a0,s0,20
	call	hrt_tx.part.0
.L37:
	lbu	a1,82(s0)
	lhu	a3,84(s0)
	lw	a5,44(s0)
	andi	a1,a1,0xff
	slli	a3,a3,16
	srli	a3,a3,16
	beq	a5,zero,.L38
	addi	a2,sp,3
	addi	a0,s0,40
	call	hrt_tx.part.0
.L38:
	lbu	a1,83(s0)
	lhu	a3,84(s0)
	lw	a5,64(s0)
	andi	a1,a1,0xff
	slli	a3,a3,16
	srli	a3,a3,16
	beq	a5,zero,.L39
	addi	a2,sp,3
	addi	a0,s0,60
	call	hrt_tx.part.0
.L39:
	lbu	a5,92(s0)
	bne	a5,zero,.L40
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
.L41:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L24
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L44
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
.L24:
	lw	ra,12(sp)
	lw	s0,8(sp)
	lw	s1,4(sp)
	addi	sp,sp,16
	jr	ra
.L40:
 #APP
	csrr a5, 3022
 #NO_APP
	andi	a5,a5,0xff
	bne	a5,zero,.L40
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
	beq	a4,a5,.L65
	lbu	a4,92(s0)
	li	a5,3
	bne	a4,a5,.L41
 #APP
	csrs 3008, 1
 #NO_APP
	j	.L41
.L34:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
	j	.L35
.L44:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	lw	ra,12(sp)
	lw	s0,8(sp)
	lw	s1,4(sp)
	addi	sp,sp,16
	jr	ra
.L28:
	lbu	a5,81(s0)
	j	.L63
.L31:
	lbu	a5,9(a5)
 #APP
	csrw 3022, a5
 #NO_APP
	j	.L33
.L30:
	lbu	a5,8(a5)
 #APP
	csrw 3022, a5
 #NO_APP
	j	.L33
.L64:
	lbu	a5,80(s0)
	j	.L63
.L65:
 #APP
	csrc 3008, 1
 #NO_APP
	j	.L41
	.size	hrt_write, .-hrt_write
	.section	.text.hrt_read,"ax",@progbits
	.align	1
	.globl	hrt_read
	.type	hrt_read, @function
hrt_read:
	addi	sp,sp,-36
	sw	s0,28(sp)
	sw	ra,32(sp)
	sw	s1,24(sp)
	lbu	a4,83(a0)
	li	a5,32
	lbu	a2,83(a0)
	div	a5,a5,a4
	lbu	a1,83(a0)
	lw	a4,64(a0)
	lbu	a3,87(a0)
	lw	s1,60(a0)
	lbu	t1,92(a0)
	mv	s0,a0
	li	a0,3
	andi	a2,a2,0xff
	andi	a1,a1,0xff
	andi	a3,a3,0xff
	addi	a5,a5,-1
	andi	a5,a5,0xff
	bne	t1,a0,.L67
	lbu	a0,68(s0)
	addi	a0,a0,-1
	andi	a0,a0,0xff
	sb	a0,68(s0)
.L67:
	lbu	a0,88(s0)
	lbu	t1,86(s0)
	bne	a0,zero,.L68
	li	a0,1
	sll	a0,a0,t1
	slli	a0,a0,16
	srli	a0,a0,16
 #APP
	csrc 3008, a0
 #NO_APP
.L69:
 #APP
	csrr a0, 3008
 #NO_APP
	sw	a0,0(sp)
	lw	a0,4(s0)
	bne	a0,zero,.L70
	lw	a0,24(s0)
	beq	a0,zero,.L116
.L70:
	sw	zero,64(s0)
	li	a0,1
	sb	a0,87(s0)
	mv	a0,s0
	sw	a3,20(sp)
	sw	a4,16(sp)
	sw	a1,12(sp)
	sw	a2,8(sp)
	sw	a5,4(sp)
	call	hrt_write
	lw	a3,20(sp)
	lw	a4,16(sp)
	lw	a1,12(sp)
	lw	a2,8(sp)
	lw	a5,4(sp)
.L71:
	sw	a4,64(s0)
	sb	a3,87(s0)
	lbu	t1,83(s0)
	li	a0,1
	li	a3,-5
	bleu	t1,a0,.L72
	lbu	t1,83(s0)
	li	a0,4
	li	a3,-31
	beq	t1,a0,.L72
	li	a3,-7
.L72:
 #APP
	csrr a0, 3009
 #NO_APP
	and	a3,a3,a0
	slli	a3,a3,16
	srli	a3,a3,16
 #APP
	csrw 3009, a3
	csrw 3011, 2
 #NO_APP
	li	a3,2031616
	slli	a1,a1,16
	and	a1,a1,a3
	ori	a3,a1,4
 #APP
	csrw 3043, a3
	csrw 3022, 1
	csrw 2000, 2
	csrw 2001, 2
 #NO_APP
	lhu	a3,84(s0)
	slli	a3,a3,16
	srli	a3,a3,16
 #APP
	csrr a0, 2003
 #NO_APP
	li	t1,-65536
	and	a0,a0,t1
	or	a3,a3,a0
 #APP
	csrw 2003, a3
 #NO_APP
	lhu	a0,84(s0)
	slli	a0,a0,16
	srli	a0,a0,16
 #APP
	csrr a3, 2003
 #NO_APP
	slli	a0,a0,1
	addi	a0,a0,1
	slli	a3,a3,16
	srli	a3,a3,16
	slli	a0,a0,16
	or	a0,a0,a3
 #APP
	csrw 2003, a0
 #NO_APP
	li	a3,126976
	slli	a2,a2,12
	and	a2,a2,a3
	andi	a5,a5,63
	li	a3,2097152
	or	a5,a5,a2
	addi	a3,a3,1024
	or	a0,a5,a3
 #APP
	csrw 3019, a0
	csrw 3017, 0
 #NO_APP
	li	a0,1
	beq	a4,a0,.L117
	li	a5,2
	beq	a4,a5,.L118
	lbu	a0,83(s0)
	li	a5,32
	div	a5,a5,a0
	addi	a5,a5,-3
	andi	a5,a5,63
	or	a5,a5,a2
	or	a5,a5,a3
 #APP
	csrw 3019, a5
 #NO_APP
	li	a5,2
	addi	t1,a4,-1
	bne	t1,a5,.L83
.L121:
	lbu	a5,69(s0)
	addi	a5,a5,-1
	j	.L82
.L116:
	lw	a0,44(s0)
	beq	a0,zero,.L71
	j	.L70
.L118:
	lbu	a5,69(s0)
	addi	a5,a5,-3
	andi	a5,a5,63
	or	a5,a5,a2
	or	a5,a5,a3
 #APP
	csrw 3019, a5
 #NO_APP
	lbu	a5,68(s0)
	li	t1,1
	addi	a5,a5,-1
.L82:
	lhu	a3,84(s0)
	li	a0,65536
	or	a3,a3,a0
 #APP
	csrw 2002, a3
	csrr a3, 3018
 #NO_APP
	li	a0,2097152
	andi	a5,a5,63
	or	a5,a5,a2
	addi	a0,a0,1024
	or	a3,a5,a0
 #APP
	csrw 3019, a3
 #NO_APP
	beq	t1,zero,.L90
	mv	a3,s1
	addi	a4,a4,-2
	li	t2,1
	li	t0,2
	li	s1,32
	li	t1,-1
	sw	a1,4(sp)
.L89:
	beq	a4,t2,.L85
	beq	a4,t0,.L86
	lbu	a5,83(s0)
	div	a5,s1,a5
	addi	a5,a5,-1
.L88:
 #APP
	csrr a1, 3018
 #NO_APP
	andi	a5,a5,63
	or	a5,a5,a2
	sw	a1,0(a3)
	or	a1,a5,a0
 #APP
	csrw 3019, a1
 #NO_APP
	addi	a4,a4,-1
	addi	a3,a3,4
	bne	a4,t1,.L89
	lw	a1,4(sp)
.L90:
 #APP
	csrr a3, 2005
	csrr a4, 2005
 #NO_APP
	slli	a3,a3,16
	slli	a4,a4,16
	srli	a3,a3,16
	srli	a4,a4,16
	bne	a3,a4,.L90
	lw	a4,0(sp)
	slli	a4,a4,24
 #APP
	csrw 3017, a4
	csrw 3043, a1
 #NO_APP
	lbu	a3,92(s0)
	li	a4,3
	beq	a3,a4,.L119
 #APP
	csrr a4, 3018
 #NO_APP
	lbu	a2,68(s0)
	lbu	a3,83(s0)
	mul	a2,a2,a3
	li	a3,32
	sub	a3,a3,a2
	srl	a4,a4,a3
	sw	a4,72(s0)
.L77:
 #APP
	csrw 2000, 0
	csrw 2001, 0
	csrw 3019, a5
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L66
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L98
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
.L66:
	lw	ra,32(sp)
	lw	s0,28(sp)
	lw	s1,24(sp)
	addi	sp,sp,36
	jr	ra
.L86:
	lbu	a5,69(s0)
	addi	a5,a5,-1
	j	.L88
.L85:
	lbu	a5,68(s0)
	addi	a5,a5,-1
	j	.L88
.L68:
	li	a0,1
	sll	a0,a0,t1
	slli	a0,a0,16
	srli	a0,a0,16
 #APP
	csrs 3008, a0
 #NO_APP
	j	.L69
.L117:
	lbu	t1,68(s0)
	li	a0,2
	beq	t1,a0,.L120
	lbu	a5,68(s0)
	addi	a5,a5,-3
	andi	a5,a5,63
	or	a5,a5,a2
	or	a5,a5,a3
 #APP
	csrw 3019, a5
 #NO_APP
	li	t1,0
	li	a5,2
	beq	t1,a5,.L121
.L83:
	lbu	a3,83(s0)
	li	a5,32
	div	a5,a5,a3
	addi	a5,a5,-1
	j	.L82
.L119:
 #APP
	fence iorw,iorw
	csrr a3, 3018
	csrw 3011, 0
 #NO_APP
	lhu	a2,84(s0)
	li	a4,35
	bgtu	a2,a4,.L122
.L92:
	li	a2,65536
	addi	a2,a2,-1
.L93:
 #APP
	csrr a4, 3010
 #NO_APP
	slli	a4,a4,16
	srli	a4,a4,16
	beq	a4,a2,.L93
	lhu	a2,84(s0)
	li	a4,35
	bleu	a2,a4,.L94
 #APP
	csrw 2010, 0
 #NO_APP
.L94:
 #APP
	csrr a2, 3010
 #NO_APP
	slli	a2,a2,16
	srli	a2,a2,16
 #APP
	csrs 3008, 1
 #NO_APP
	lbu	a1,83(s0)
	li	a4,4
	beq	a1,a4,.L101
	li	a4,2
	sw	a4,4(sp)
	li	a4,4
	sw	a4,0(sp)
.L95:
	lbu	t2,83(s0)
	lbu	a1,68(s0)
	lbu	a4,83(s0)
	li	a0,65536
	addi	a1,a1,1
	mul	a1,a1,a4
	addi	a0,a0,-256
	srli	s1,a3,24
	slli	t1,a3,24
	srli	a4,a3,8
	li	t0,16711680
	or	t1,s1,t1
	and	a4,a4,a0
	slli	a3,a3,8
	and	a3,a3,t0
	or	a4,t1,a4
	or	a4,a4,a3
	lw	a3,0(sp)
	sll	a4,a4,t2
	and	a2,a2,a3
	lw	a3,4(sp)
	sra	a2,a2,a3
	li	a3,32
	sub	a3,a3,a1
	or	a4,a4,a2
	sll	a4,a4,a3
	srli	a3,a4,24
	slli	a1,a4,24
	srli	a2,a4,8
	or	a3,a3,a1
	and	a2,a2,a0
	slli	a4,a4,8
	or	a3,a3,a2
	and	a4,a4,t0
	or	a4,a3,a4
	sw	a4,72(s0)
	j	.L77
.L98:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	lw	ra,32(sp)
	lw	s0,28(sp)
	lw	s1,24(sp)
	addi	sp,sp,36
	jr	ra
.L122:
 #APP
	csrw 2000, 0
 #NO_APP
	lhu	a4,84(s0)
	addi	a4,a4,-35
	slli	a4,a4,16
	srli	a4,a4,16
 #APP
	csrw 2005, a4
 #NO_APP
	j	.L92
.L101:
	li	a4,1
	sw	a4,4(sp)
	li	a4,30
	sw	a4,0(sp)
	j	.L95
.L120:
	lhu	a4,84(s0)
	li	a3,65536
	or	a4,a4,a3
 #APP
	csrw 2002, a4
 #NO_APP
.L75:
 #APP
	csrr a3, 2005
	csrr a4, 2005
 #NO_APP
	slli	a3,a3,16
	slli	a4,a4,16
	srli	a3,a3,16
	srli	a4,a4,16
	bne	a3,a4,.L75
	lw	a4,0(sp)
	slli	a4,a4,24
 #APP
	csrw 3017, a4
	csrw 3043, a1
 #NO_APP
	lbu	a3,83(s0)
	li	a4,8
	beq	a3,a4,.L123
 #APP
	csrr a4, 3018
 #NO_APP
	srli	a4,a4,24
	sw	a4,72(s0)
	j	.L77
.L123:
 #APP
	csrr a4, 3018
 #NO_APP
	srli	a4,a4,16
	sw	a4,72(s0)
	j	.L77
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
