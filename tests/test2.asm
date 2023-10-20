    in
    pop rax     ; Read value

Begin:
    push rax
    out         ; print current index

    push rax
    push 1
    sub         ; decrement index

    pop rax
    push rax    ; save new index value

    push 0
    jae Begin   ; jump if positive

    jmp Stop

    out         ; Should not be triggered


Stop:           ; end program

    hlt
