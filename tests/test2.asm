    in
    pop rax

Begin:
    push rax
    out
    push rax
    push 1
    sub

    pop rax
    push rax
    push 0

    jae Begin

    hlt
