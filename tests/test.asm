in
pop rax ; read a value
in
pop rbx ; read b value
in
pop rcx ; read c value

Begin:

push rbx
push rbx
mul     ; compute b^2

push 4
push rax
push rcx
mul
mul     ; compute 4ac

sub
out     ; compute and print D

push rax+1
out     ; compute and print a + 1

jmp Begin   ; jump test

hlt     ; terminate program


