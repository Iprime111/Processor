Begin:
    push rax
    push 1200

    jb Continue

    push 0
    pop rax

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
    push 1
    add
    pop [rax]
    ret

