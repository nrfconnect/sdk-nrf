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
.L21:
	mul	a4,a5,a2
	add	a4,s0,a4
	lw	a4,4(a4)
	bne	a4,zero,.L20
	addi	a5,a5,1
	bne	a5,a3,.L21
	li	a5,3
.L22:
	li	a4,1
	beq	a5,a4,.L23
	li	a4,3
	beq	a5,a4,.L24
	li	a3,0
	bne	a5,zero,.L25
	lbu	a3,80(s0)
.L43:
	andi	a3,a3,0xff
.L25:
	lui	a4,%hi(xfer_shift_ctrl+2)
	sb	a3,%lo(xfer_shift_ctrl+2)(a4)
 #APP
	csrw 2000, 2
 #NO_APP
	lhu	a4,84(s0)
	slli	a4,a4,16
	srli	a4,a4,16
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
	beq	a4,a2,.L26
	li	a2,2
	beq	a4,a2,.L27
	li	a5,32
	div	a5,a5,a3
	j	.L44
.L20:
	andi	a5,a5,0xff
	j	.L22
.L23:
	lbu	a3,81(s0)
	j	.L43
.L24:
	lbu	a3,83(s0)
	j	.L43
.L26:
	lbu	a5,8(a5)
.L44:
 #APP
	csrw 3022, a5
 #NO_APP
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L30
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
.L31:
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
	lbu	a5,94(s0)
	bne	a5,zero,.L32
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	li	a4,131072
	slli	a5,s1,1
	addi	a4,a4,-2
	and	a5,a5,a4
 #APP
	csrw 3012, a5
	csrw 2000, 0
 #NO_APP
.L33:
 #APP
	csrw 2005, 0
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L19
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L36
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
.L19:
	lw	ra,12(sp)
	lw	s0,8(sp)
	lw	s1,4(sp)
	addi	sp,sp,16
	jr	ra
.L27:
	lbu	a5,9(a5)
	j	.L44
.L30:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
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
	li	a5,4096
	addi	a5,a5,1
 #APP
	csrw 3019, a5
 #NO_APP
	lbu	a4,94(s0)
	li	a5,1
	bne	a4,a5,.L34
 #APP
	csrc 3008, 1
 #NO_APP
	j	.L33
.L34:
	lbu	a4,94(s0)
	li	a5,3
	bne	a4,a5,.L33
 #APP
	csrs 3008, 1
 #NO_APP
	j	.L33
.L36:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
	j	.L19
	.size	hrt_write, .-hrt_write
	.section	.text.hrt_read,"ax",@progbits
	.align	1
	.globl	hrt_read
	.type	hrt_read, @function
hrt_read:
	addi	sp,sp,-36
	sw	s0,28(sp)
	sw	s1,24(sp)
	sw	ra,32(sp)
	lbu	a5,83(a0)
	li	s1,32
	mv	s0,a0
	div	s1,s1,a5
	lbu	a5,83(a0)
	lbu	a3,83(a0)
	lw	a4,64(a0)
	lbu	t1,87(a0)
	lw	a1,60(a0)
	lbu	a0,94(a0)
	li	a2,3
	andi	a5,a5,0xff
	andi	a3,a3,0xff
	andi	t1,t1,0xff
	addi	s1,s1,-1
	andi	s1,s1,0xff
	bne	a0,a2,.L46
	lbu	a2,68(s0)
	addi	a2,a2,-1
	andi	a2,a2,0xff
	sb	a2,68(s0)
.L46:
	lbu	a2,88(s0)
	lbu	a0,86(s0)
	bne	a2,zero,.L47
	li	a2,1
	sll	a2,a2,a0
	slli	a2,a2,16
	srli	a2,a2,16
 #APP
	csrc 3008, a2
 #NO_APP
.L48:
 #APP
	csrr a2, 3008
 #NO_APP
	lw	a0,4(s0)
	sw	a2,0(sp)
	bne	a0,zero,.L49
	lw	a0,24(s0)
	bne	a0,zero,.L49
	lw	a0,44(s0)
	beq	a0,zero,.L50
.L49:
	sw	zero,64(s0)
	li	a0,1
	sb	a0,87(s0)
	mv	a0,s0
	sw	a1,20(sp)
	sw	t1,16(sp)
	sw	a4,12(sp)
	sw	a3,8(sp)
	sw	a5,4(sp)
	call	hrt_write
	lw	a1,20(sp)
	lw	t1,16(sp)
	lw	a4,12(sp)
	lw	a3,8(sp)
	lw	a5,4(sp)
.L50:
	sw	a4,64(s0)
	sb	t1,87(s0)
	lbu	t1,83(s0)
	li	a0,1
	bne	t1,a0,.L51
	lhu	a0,90(s0)
	slli	a0,a0,16
	srli	a0,a0,16
	andi	a0,a0,-5
	slli	a0,a0,16
	srli	a0,a0,16
	sh	a0,90(s0)
	lhu	a0,90(s0)
.L90:
 #APP
	csrw 3009, a0
	csrw 3011, 2
 #NO_APP
	li	a0,2031616
	slli	a3,a3,16
	and	a3,a3,a0
	ori	a0,a3,4
 #APP
	csrw 3043, a0
	csrw 3022, 1
	csrw 2000, 2
	csrw 2001, 2
 #NO_APP
	lhu	a0,84(s0)
	slli	a0,a0,16
	srli	a0,a0,16
 #APP
	csrr t1, 2003
 #NO_APP
	li	t0,-65536
	and	t1,t1,t0
	or	a0,a0,t1
 #APP
	csrw 2003, a0
 #NO_APP
	lhu	a0,84(s0)
	slli	a0,a0,16
	srli	a0,a0,16
 #APP
	csrr t1, 2003
 #NO_APP
	slli	a0,a0,1
	slli	t1,t1,16
	addi	a0,a0,1
	srli	t1,t1,16
	slli	a0,a0,16
	or	a0,a0,t1
 #APP
	csrw 2003, a0
 #NO_APP
	li	a0,126976
	slli	a5,a5,12
	and	a5,a5,a0
	li	t1,2097152
	andi	a0,s1,63
	or	a0,a0,a5
	addi	t1,t1,1024
	or	a0,a0,t1
 #APP
	csrw 3019, a0
	csrw 3017, 0
 #NO_APP
	li	a0,1
	bne	a4,a0,.L53
	lbu	t0,68(s0)
	li	a0,2
	bne	t0,a0,.L54
	lhu	a4,84(s0)
	li	a1,65536
	or	a4,a4,a1
 #APP
	csrw 2002, a4
 #NO_APP
.L55:
 #APP
	csrr a1, 2005
	csrr a4, 2005
 #NO_APP
	slli	a1,a1,16
	slli	a4,a4,16
	srli	a1,a1,16
	srli	a4,a4,16
	bne	a1,a4,.L55
	lw	a4,0(sp)
	slli	a2,a4,24
 #APP
	csrw 3017, a2
	csrw 3043, a3
 #NO_APP
	lbu	a3,83(s0)
	li	a4,8
	bne	a3,a4,.L56
 #APP
	csrr a4, 3018
 #NO_APP
	srli	a4,a4,16
.L93:
	sw	a4,72(s0)
 #APP
	csrw 2000, 0
	csrw 2001, 0
 #NO_APP
	andi	s1,s1,63
	or	s1,s1,a5
 #APP
	csrw 3019, s1
 #NO_APP
	lbu	a5,87(s0)
	bne	a5,zero,.L77
	lbu	a5,88(s0)
	lbu	a4,86(s0)
	bne	a5,zero,.L78
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrs 3008, a5
 #NO_APP
.L77:
	lbu	a4,83(s0)
	li	a5,1
	bne	a4,a5,.L45
	lhu	a5,90(s0)
	ori	a5,a5,4
	sh	a5,90(s0)
	lhu	a5,90(s0)
 #APP
	csrw 3009, a5
 #NO_APP
.L45:
	lw	ra,32(sp)
	lw	s0,28(sp)
	lw	s1,24(sp)
	addi	sp,sp,36
	jr	ra
.L47:
	li	a2,1
	sll	a2,a2,a0
	slli	a2,a2,16
	srli	a2,a2,16
 #APP
	csrs 3008, a2
 #NO_APP
	j	.L48
.L51:
	lhu	a0,92(s0)
	j	.L90
.L56:
 #APP
	csrr a4, 3018
 #NO_APP
	srli	a4,a4,24
	j	.L93
.L53:
	li	a0,2
	bne	a4,a0,.L87
	lbu	a0,69(s0)
	addi	a0,a0,-3
	andi	a0,a0,63
	or	a0,a0,a5
	or	a0,a0,t1
 #APP
	csrw 3019, a0
 #NO_APP
	lbu	s1,68(s0)
.L91:
	lhu	a0,84(s0)
	li	t1,65536
	addi	s1,s1,-1
	andi	s1,s1,0xff
	or	a0,a0,t1
 #APP
	csrw 2002, a0
	csrr a0, 3018
 #NO_APP
	li	t1,2097152
	andi	a0,s1,63
	or	a0,a0,a5
	addi	t1,t1,1024
	or	a0,a0,t1
 #APP
	csrw 3019, a0
 #NO_APP
	li	a2,-1
	addi	a4,a4,-2
	li	t0,2
	li	t2,32
.L64:
	bne	a4,a2,.L69
.L70:
 #APP
	csrr a1, 2005
	csrr a4, 2005
 #NO_APP
	slli	a1,a1,16
	slli	a4,a4,16
	srli	a1,a1,16
	srli	a4,a4,16
	bne	a1,a4,.L70
	lw	a4,0(sp)
	slli	a2,a4,24
 #APP
	csrw 3017, a2
	csrw 3043, a3
 #NO_APP
	lbu	a3,94(s0)
	li	a4,3
	bne	a3,a4,.L71
 #APP
	csrr a3, 3018
	csrw 3011, 0
 #NO_APP
	lhu	a2,84(s0)
	li	a4,35
	bleu	a2,a4,.L72
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
.L72:
	li	a4,65536
	addi	a4,a4,-1
.L73:
 #APP
	csrr a2, 3010
 #NO_APP
	slli	a2,a2,16
	srli	a2,a2,16
	beq	a2,a4,.L73
	lhu	a2,84(s0)
	li	a4,35
	bleu	a2,a4,.L74
 #APP
	csrw 2010, 0
 #NO_APP
.L74:
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
	beq	a1,a4,.L80
	li	t1,2
	li	t0,4
.L75:
	slli	a1,a3,24
	srli	a4,a3,24
	or	a4,a4,a1
	li	a1,65536
	srli	a0,a3,8
	addi	a1,a1,-256
	and	a0,a0,a1
	lbu	t2,83(s0)
	or	a4,a4,a0
	slli	a3,a3,8
	li	a0,16711680
	and	a3,a3,a0
	or	a4,a4,a3
	and	a2,a2,t0
	lbu	a3,68(s0)
	sra	a2,a2,t1
	sll	a4,a4,t2
	or	a4,a4,a2
	lbu	a2,83(s0)
	addi	a3,a3,1
	mul	a3,a3,a2
	li	a2,32
	sub	a3,a2,a3
	sll	a4,a4,a3
	slli	a2,a4,24
	srli	a3,a4,24
	or	a3,a3,a2
	srli	a2,a4,8
	and	a2,a2,a1
	slli	a4,a4,8
	or	a3,a3,a2
	and	a4,a4,a0
	or	a4,a3,a4
	j	.L93
.L54:
	lbu	a0,68(s0)
	addi	a0,a0,-3
	andi	a0,a0,63
	or	a0,a0,a5
	or	a0,a0,t1
 #APP
	csrw 3019, a0
 #NO_APP
	li	a0,0
.L61:
	li	t1,2
	bne	a0,t1,.L63
	lbu	s1,69(s0)
	j	.L91
.L87:
	lbu	t0,83(s0)
	li	a0,32
	div	a0,a0,t0
	addi	a0,a0,-3
	andi	a0,a0,63
	or	a0,a0,a5
	or	a0,a0,t1
 #APP
	csrw 3019, a0
 #NO_APP
	addi	a0,a4,-1
	j	.L61
.L63:
	lbu	a0,83(s0)
	li	s1,32
	div	s1,s1,a0
	j	.L91
.L69:
	li	a0,1
	beq	a4,a0,.L65
	beq	a4,t0,.L66
	lbu	s1,83(s0)
	div	s1,t2,s1
	j	.L92
.L65:
	lbu	s1,68(s0)
.L92:
	addi	s1,s1,-1
	andi	s1,s1,0xff
 #APP
	csrr a0, 3018
 #NO_APP
	sw	a0,0(a1)
	andi	a0,s1,63
	or	a0,a0,a5
	or	a0,a0,t1
 #APP
	csrw 3019, a0
 #NO_APP
	addi	a1,a1,4
	addi	a4,a4,-1
	j	.L64
.L66:
	lbu	s1,69(s0)
	j	.L92
.L80:
	li	t1,1
	li	t0,30
	j	.L75
.L71:
 #APP
	csrr a4, 3018
 #NO_APP
	lbu	a2,68(s0)
	lbu	a3,83(s0)
	mul	a2,a2,a3
	li	a3,32
	sub	a3,a3,a2
	srl	a4,a4,a3
	j	.L93
.L78:
	li	a5,1
	sll	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrc 3008, a5
 #NO_APP
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
