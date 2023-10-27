in
call Factorial
out
hlt

Factorial:
    pop rax
    push rax

    push rax
    push 1
    jne NotOne

    ret

    NotOne:
        push rax+-1
        call Factorial
        mul

    ret


