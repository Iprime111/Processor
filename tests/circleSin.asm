in
pop rdx             ; read r

push 0
pop rax             ; rax - t parameter

Begin:
    push rax
    cos
    push 1
    add
    push rdx
    mul
    floor           ; Compute and convert to integer r * (cos t + 1)

    push rax
    sin
    push 1
    add
    push rdx
    mul
    floor           ; Compute and convert to integer r * (sin t + 1)

    call SetPixel
    call IncrementParameter

    push 1
    je End          ; end program if max t == 2 * pi

    jmp Begin

End:
    in
    hlt

IncrementParameter:
    push rax+0.001  ; increment t
    pop rax

    push rax
    push 6.28
    jb MaxValue     ; jump if t >= 2 * pi

    push 0
    pop rax

    push 1
    ret

    MaxValue:
        push 0
        ret

SetPixel:
    push 100
    mul
    add
    push 3
    mul
    pop rbx         ; Compute ram address

    push 255
    pop [rbx]       ; set red channel

    push 255
    pop [rbx+1]     ; set green channel

    push 255
    pop [rbx+2]     ; set blue channel

    ret
