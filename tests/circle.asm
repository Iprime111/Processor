in
pop rdx             ; read radius

push 0
pop rax

Begin:
    push rdx
    push rdx
    mul

    push rax
    push rdx
    sub             ; (x - r)

    push rax
    push rdx
    sub

    mul             ; (x - r) ^ 2

    sub
    sqrt
    pop rbx         ; y

    push -1
    push rbx
    mul
    push rdx
    add             ; -y + r

    push rbx
    push rdx
    add             ; y + r

    pop rbx
    call SetPixel   ; set lower pixel

    pop rbx
    call SetPixel   ; set lower pixel

    call Continue
    jmp Begin


Continue:
    push rax
    push rdx
    push 2
    mul
    jae Reset       ; check for x maximum value

    push rax+1      ; increment rax
    pop rax
    ret

    Reset:
        push 0      ; zero rax
        pop rax
        ret


SetPixel:


    push rax
    push 100
    mul
    push rbx
    add
    push 3
    mul         ; compute address

    pop rcx

    push 255
    pop [rcx]

    push 255
    pop [rcx+1]

    push 255
    pop [rcx+2] ; set 3 color channels

Return:
    ret

