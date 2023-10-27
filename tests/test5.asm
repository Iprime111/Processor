Begin:
    in
    pop rax       ; read address

    call SetPixel ; set pixel by address

    push rax
    push 0
    jae Begin     ; end program if address is less than zero

    hlt

SetPixel:
    push rax
    push 0
    jb RET      ; check if address is bigger or equal than 0

    push rax
    push 10000
    jae RET    ; check if address is less than 300

    push 255
    pop [rax]  ; set pixel

    RET:
        ret    ; return
