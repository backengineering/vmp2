.data
	__mbase dq 0h
	public __mbase

.code
__lconstbzx proc
	mov al, [rsi]
	lea rsi, [rsi+1]
	xor al, bl
	dec al
	ror al, 1
	neg al
	xor bl, al

	pushfq			; save flags...
	cmp ax, 01Ch
	je swap_val

					; the constant is not 0x1C
	popfq			; restore flags...	 
	sub rbp, 2
	mov [rbp], ax
	mov rax, __mbase
	add rax, 059FEh	; calc jmp rva is 0x59FE...
	jmp rax

swap_val:			; the constant is 0x1C
	popfq			; restore flags...
	mov ax, 5		; bit 5 is VMX in ECX after CPUID...
	sub rbp, 2
	mov [rbp], ax
	mov rax, __mbase
	add rax, 059FEh	; calc jmp rva is 0x59FE...
	jmp rax
__lconstbzx endp
end