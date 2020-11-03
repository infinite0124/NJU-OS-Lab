.code16

.global start
start:
	cli
	inb $0x92, %al
	orb $0x02, %al
	outb %al, $0x92
	data32 addr32 lgdt gdtDesc
	movl %cr0, %eax
	orb $0x01, %al
	movl %eax, %cr0
	data32 ljmp $0x08, $start32

.code32
start32:
	movw $(2<<3), %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss

	movw $(3<<3), %ax
	movw %ax, %gs

	movl $0, %ebp
	movl $(128<<20), %esp

	jmp bootMain

gdt:
	.word 0,0
	.byte 0,0,0,0		

	.word 0xffff,0
	.byte 0,0x9a,0xcf,0	#code

	.word 0xffff,0
	.byte 0,0x92,0xcf,0	#data

	.word 0xffff,0x8000
	.byte 0x0b,0x92,0xcf,0

gdtDesc:
	.word (gdtDesc-gdt-1)
	.long gdt

