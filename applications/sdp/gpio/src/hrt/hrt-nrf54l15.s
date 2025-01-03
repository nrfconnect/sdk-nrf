	.file	"hrt.c"
	.option nopic
	.attribute arch, "rv32e1p9_m2p0_c2p0_zicsr2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 4
	.text
	.section	.text.hrt_set_bits,"ax",@progbits
	.align	1
	.globl	hrt_set_bits
	.type	hrt_set_bits, @function
hrt_set_bits:
 #APP
	csrr a4, 3008
 #NO_APP
	lui	a5,%hi(irq_arg)
	lhu	a5,%lo(irq_arg)(a5)
	or	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
	ret
	.size	hrt_set_bits, .-hrt_set_bits
	.section	.text.hrt_clear_bits,"ax",@progbits
	.align	1
	.globl	hrt_clear_bits
	.type	hrt_clear_bits, @function
hrt_clear_bits:
 #APP
	csrr a4, 3008
 #NO_APP
	lui	a5,%hi(irq_arg)
	lhu	a5,%lo(irq_arg)(a5)
	not	a5,a5
	and	a5,a5,a4
	slli	a5,a5,16
	srli	a5,a5,16
 #APP
	csrw 3008, a5
 #NO_APP
	ret
	.size	hrt_clear_bits, .-hrt_clear_bits
	.section	.text.hrt_toggle_bits,"ax",@progbits
	.align	1
	.globl	hrt_toggle_bits
	.type	hrt_toggle_bits, @function
hrt_toggle_bits:
	lui	a5,%hi(irq_arg)
	lhu	a5,%lo(irq_arg)(a5)
 #APP
	csrw 3024, a5
 #NO_APP
	ret
	.size	hrt_toggle_bits, .-hrt_toggle_bits
