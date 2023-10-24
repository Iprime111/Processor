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

hlt     ; terminate program


