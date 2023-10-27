Begin:
    push rax
    push 30000

    jb Continue

    hlt

Continue:
    call IncrementColor
    call IncrementCell
    jmp Begin

IncrementCell:
    push rax+1
    pop rax
    ret

IncrementColor:
    push [rax]
    push 255
    add
    pop [rax]
    ret

