in

pop rax
push rax
push 1
jb Stop

push rax

call Factorial
out
hlt

Factorial:
    pop rax
    push rax

    push rax
    push 1
    ja NotOne

    ret

    NotOne:
        push rax+-1
        call Factorial
        mul

    ret

Stop:
    hlt


