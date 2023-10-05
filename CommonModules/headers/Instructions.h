//INSTRUCTION(NAME, NUMBER, ARUMENTS_COUNT, SCANF_SPECIFIERS, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK)

INSTRUCTION (hlt,   0, 0, {}, {
    RETURN PROCESSOR_HALT;
})

INSTRUCTION (out,  -1, 0, {}, {
    elem_t value {};
    PopValue(&value);

    PrintData (CONSOLE_DEFAULT, CONSOLE_NORMAL, stdout, value);
    fputs ("\n", stdout);
})

INSTRUCTION (in,   -2, 0, {}, {
    elem_t value {};

    printf_color (CONSOLE_WHITE, CONSOLE_BOLD, "Enter value: ");
    scanf ("%llu", &value);

    PushValue (value);

})

INSTRUCTION (push, -3, 1, {"%lld"}, {
    CheckBuffer (Bytecode);

    elem_t value = *(elem_t *) (Bytecode->buffer + CurrentChar);
    CurrentChar += sizeof (elem_t);

    PushValue (value);
})

INSTRUCTION (add,   1, 0, {}, {
    elem_t value1 {};
    elem_t value2 {};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value1 + value2);
})

INSTRUCTION (sub,   2, 0, {}, {
    elem_t value1 {};
    elem_t value2 {};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value2 - value1);
})

INSTRUCTION (mul,   3, 0, {}, {
    elem_t value1 {};
    elem_t value2 {};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value1 * value2);
})

INSTRUCTION (div,   4, 0, {}, {
    elem_t value1 {};
    elem_t value2 {};

    PopValue (&value1);
    PopValue (&value2);

    PushValue (value2 / value1);
})

INSTRUCTION (sin,   5, 0, {}, {
    elem_t value {};

    PopValue (&value);

    PushValue (sin (value));
})

INSTRUCTION (cos,   6, 0, {}, {
    elem_t value {};

    PopValue (&value);

    PushValue (cos (value));
})

INSTRUCTION (sqrt,  7, 0, {}, {
    elem_t value {};

    PopValue (&value);

    PushValue (sqrt (value));
})
