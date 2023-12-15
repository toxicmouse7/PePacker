.386
.model flat, stdcall

.code

_start:
    call main

main:
    pop esi
    sub esi, 5

    mov edi, esi
    and esi, 0FFFF0000h

    add esi, dword ptr [section_start + edi]
    mov ebx, esi
    xor ecx, ecx
    .while ecx != [decode_length + edi]
        mov al, byte ptr [ebx + ecx]
        xor al, 228
        mov [ebx + ecx], al
        inc ecx
    .endw
    
    mov eax, [original_entry + edi]
    and esi, 0FFFF0000h
    add eax, esi
    jmp eax

section_start dd 0
decode_length dd 0
original_entry dd 0

end