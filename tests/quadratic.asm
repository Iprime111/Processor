in
pop rax
in
pop rbx
in
pop rcx ; read coefs values

push rbx
push rbx
mul
push 4
push rax
push rcx
mul
mul
sub
pop rdx ; compute D

push rax
push 0
je Linear ; if a = 0

push rdx
push 0
je ZeroD ; one solution

push rdx
push 0
jb MinusD ; no solutions

push rdx
push 0
ja PlusD ; two solutions

ZeroD:
    push 1
    out

    push 0
    push rbx
    sub         ; -b

    push 2
    push rax
    mul         ; 2a

    div         ; -b / 2a
    out
    hlt

Linear:
    push 0
    push rbx
    je Constant   ; if a = 0 and b = 0

    push 1
    out

    push 0
    push rcx
    sub         ; -c
    push rbx
    div         ; -c / b
    out
    hlt

Constant:
    push rcx
    push 0 
    je Infinite

    push 0
    out
    hlt

Infinite:
    push inf
    out
    hlt

MinusD:
    push 0
    out
    hlt

PlusD:
    push 2
    out

    push 0
    push rbx
    sub         ; -b
    pop rbx

    push 2
    push rax
    mul         ; 2a
    pop rax

    push rdx
    sqrt        ; sqrt D
    pop rdx

    push rbx
    push rdx
    add
    push rax
    div
    out        ; (-b + D) / 2a

    push rbx
    push rdx
    sub
    push rax
    div
    out
    hlt        ; (-b - D) / 2a
