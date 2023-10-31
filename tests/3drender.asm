push 0 ; rax - x0
pop rax

push 0 ; rbx - y0
pop rbx

push 1.5
pop [30003] ; fov (rad)

push 1.57
push [30003]
div
push 2
mul
pop [30004] ; x scale

push [30003] ; rcx - camera vector angle
push 2
div
push -1
mul
pop rcx

push [30003]
push 100
div
pop rdx    ; rdx - angle increment

push 2
pop [30002] ; wall distance

push 0
pop rhx     ; line number

RenderLoop:

    call ComputeCameraVector
    pop rfx ; camera vector y
    pop rex ; camera vector x

    push 1
    push rex
    push rex
    mul
    sub
    sqrt
    pop [30005]

    push rhx

    push rex
    push rfx
    call ComputeDistance

    push 100
    push [30001] ; distance
    push [30001]
    mul
    div
    push [30005]
    div
    floor        ; compute brightness

    call DrawLine

    call IncrementCameraAngle
    jmp RenderLoop

DrawLine:           ; needs x coordinate and line height in stack
    pop rex         ; line height
    pop rfx         ; line x

    push rex

    DrawLineLoop:
        pop rex

        push rfx
        push 50
        push rex
        add
        call SetPixel

        push rfx
        push 50
        push rex
        push -1
        mul
        add
        call SetPixel

        push rex+-1
        push rex+-1
        push 0
        jae DrawLineLoop

    ret

SetPixel:           ; needs x and y coordinate in stack
    pop rgx

    push 100
    mul
    push rgx
    add
    push 3
    mul
    pop rgx         ; Compute ram address

    push 255
    push [30001]
    div
    pop [rgx]       ; set red channel

    push 255
    push [30001]    ; distance
    div
    pop [rgx+1]     ; set green channel

    push 255
    push [30001]
    div
    pop [rgx+2]     ; set blue channel

    ret

IncrementCameraAngle:
    push rcx
    push rdx
    add
    pop rcx

    push rhx+1
    floor
    pop rhx

    push rhx
    push 100
    jae IsMax
    ret

    IsMax:
        push [30003] ; fov
        push 2
        div
        push -1
        mul
        pop rcx

        push 0
        pop rhx

        in
        pop rgx     ; read player's action

        push rgx
        push 1
        je IncrementDistance

        push rgx
        push 2
        je DecrementDistance

    ReturnIncrementCamera:
        call ClearScreen
        ret

IncrementDistance:
    push [30002]  ; wall distance
    push 0.1
    add
    pop  [30002]
    jmp ReturnIncrementCamera

DecrementDistance:
    push [30002]  ; wall distance
    push 0.1
    sub
    pop  [30002]
    jmp ReturnIncrementCamera

ClearScreen:        ; no arguments required
    push 0
    pop rgx

    ClearScreenLoop:
        push 0
        pop [rgx]

        push rgx+1
        pop rgx

        push rgx
        push 30000
        jb ClearScreenLoop
    ret


ComputeCameraVector: ; needs angle in rcx
    push rcx
    sin

    push rcx
    cos
    ret

ComputeDistance:    ; needs camera vector coordinates in stack
    pop rfx         ; camera vector y
    pop rex         ; camera vector x

    push rex
    push [30002]    ; wall distance
    mul

    push rfx

    div

    pop rex         ; x distance
    push [30002]    ; wall distance
    pop rfx         ; y distance

    push rex
    push rex
    mul
    push rfx
    push rfx
    mul
    add
    sqrt            ; distance
    pop  [30001]
    ret

Stop:
    hlt


