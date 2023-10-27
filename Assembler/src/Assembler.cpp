#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>

#include "Assembler.h"
#include "Buffer.h"
#include "CommonModules.h"
#include "Registers.h"
#include "AssemblyHeader.h"
#include "CustomAssert.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SPU.h"
#include "TextTypes.h"
#include "Stack/Stack.h"
#include "DSLFunctions.h"
#include "Label.h"
#include "StringProcessing.h"
#include "FileFunctions.h"

const size_t MAX_LABELS_COUNT    = 128;

static ProcessorErrorCode DoCompilationPass (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *text, size_t *commandsCount);
static ProcessorErrorCode CompileLine       (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextLine   *line, int     lineNumber);

static ProcessorErrorCode CompileInstructionOpcode (TextLine *line, AssemblerInstruction *instruction, ArgumentsType *permittedArguments, int lineNumber);
static ProcessorErrorCode CompileInstructionArgumentsData (AssemblerInstruction *instruction, TextLine *line, InstructionArguments *arguments, ArgumentsType permittedArguments, Buffer <Label> *labelsBuffer, int lineNumber);
static ProcessorErrorCode ReadRamBrackets (AssemblerInstruction *instruction, TextLine *line, ssize_t *offset, ArgumentsType permittedArguments, int lineNumber);

static ProcessorErrorCode SaveLabel (Buffer <char> *binaryBuffer, Buffer <Label> *labelsBuffer, TextLine *sourceLine, char *labelName);

static ProcessorErrorCode EmitLabelListing       (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer,                                                                     TextLine *sourceLine, int lineNumber);
static ProcessorErrorCode EmitInstructionListing (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine, int lineNumber);
static ProcessorErrorCode EmitInstructionBinary  (Buffer <char> *binaryBuffer,                               AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine, int lineNumber);

static ProcessorErrorCode CreateAssemblyBuffers  (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, FileBuffer *sourceFile, TextBuffer *sourceText);
static ProcessorErrorCode DestroyAssemblyBuffers (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer);

ProcessorErrorCode AssembleFile (TextBuffer *text, FileBuffer *file, int binaryDescriptor, int listingDescriptor) {
    PushLog (1);

    custom_assert (file,        pointer_is_null, NO_BUFFER);
    custom_assert (text,        pointer_is_null, NO_BUFFER);
    custom_assert (text->lines, pointer_is_null, NO_BUFFER);

    Buffer <char>            binaryBuffer    = {0, 0, NULL};
    Buffer <char>            listingBuffer   = {0, 0, NULL};
    Buffer <Label>           labelsBuffer    = {0, 0, NULL};
    Buffer <DebugInfoChunk> debugInfoBuffer = {0, 0, NULL};

    DeleteExcessWhitespaces (text);

    PrintSuccessMessage ("Starting assembly...", NULL);

    ProgramErrorCheck (CreateAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer, file, text), "Error occuried while creating file buffers");

    size_t commandsCount = 0;


    #define TerminateIfErrorsWereFound(message, ...)                                                                                    \
        do {                                                                                                                            \
            ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;                                                                         \
            if ((errorCode = (__VA_ARGS__)) != NO_PROCESSOR_ERRORS) {                                                                   \
                errorCode = (ProcessorErrorCode) (errorCode | DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer));   \
                errorCode = (ProcessorErrorCode) (errorCode | DestroyBuffer (&debugInfoBuffer));                                        \
                ProgramErrorCheck (errorCode, message);                                                                                 \
            }                                                                                                                           \
        } while (0)

    TerminateIfErrorsWereFound("Compilation error", DoCompilationPass (&binaryBuffer, &listingBuffer, &labelsBuffer, NULL, text, &commandsCount));

    InitBuffer (&debugInfoBuffer, commandsCount);

    TerminateIfErrorsWereFound ("Compilation error", DoCompilationPass( &binaryBuffer, &listingBuffer, &labelsBuffer, &debugInfoBuffer, text, NULL));
    TerminateIfErrorsWereFound ("Error occuried while writing data to output file", WriteDataToFiles (&binaryBuffer, &listingBuffer, &debugInfoBuffer, binaryDescriptor, listingDescriptor));

    #undef TerminateIfErrorsWereFound

    ProgramErrorCheck (DestroyAssemblyBuffers (&binaryBuffer, &listingBuffer, &labelsBuffer), "Error occuried while destroying assembly buffers");
    DestroyBuffer (&debugInfoBuffer);

    PrintSuccessMessage ("Assembly finished successfully!", NULL);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DoCompilationPass (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextBuffer *text, size_t *commandsCount) {
    PushLog (1);

    custom_assert (binaryBuffer,  pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (labelsBuffer,  pointer_is_null, NO_BUFFER);
    custom_assert (text,          pointer_is_null, NO_BUFFER);
    custom_assert (text->lines,   pointer_is_null, NO_BUFFER);

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    binaryBuffer->currentIndex  = 0;
    listingBuffer->currentIndex = 0;

    for (size_t lineIndex = 0; lineIndex < text->line_count; lineIndex++) {
        errorCode = CompileLine (binaryBuffer, listingBuffer, labelsBuffer, debugInfoBuffer, text->lines + lineIndex, (int) lineIndex + 1);

        if (errorCode != BLANK_LINE && commandsCount) {
            (*commandsCount)++;

        } else if (errorCode != BLANK_LINE) {
            ProgramErrorCheck (errorCode, "Something has gone wrong while compiling line");
        }
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CompileLine (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, Buffer <DebugInfoChunk> *debugInfoBuffer, TextLine *line, int lineNumber) {
    PushLog (2);

    custom_assert (line,          pointer_is_null, NO_BUFFER);
    custom_assert (line->pointer, pointer_is_null, NO_BUFFER);
    custom_assert (binaryBuffer,  pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (labelsBuffer,  pointer_is_null, NO_BUFFER);

    if (FindActualStringBegin (line) < 0) {
        RETURN BLANK_LINE;
    }

    char labelName [LABEL_NAME_LENGTH] = "";

    if (IsLabelLine (line, labelName)) {
        ProgramErrorCheck (SaveLabel (binaryBuffer, labelsBuffer, line, labelName),
                            "Error occuried while saving label");
        ProgramErrorCheck (EmitLabelListing (binaryBuffer, listingBuffer, line, lineNumber),
                            "Error occuried while writing label to the listing");

        RETURN BLANK_LINE;
    }

    ProcessorErrorCode errorCode = NO_PROCESSOR_ERRORS;

    InstructionArguments arguments {NAN, REGISTER_COUNT};
    ArgumentsType permittedArguments = NO_ARGUMENTS;
    AssemblerInstruction outputInstruction {"", {0, 0}, NULL};

    if ((errorCode = CompileInstructionOpcode (line, &outputInstruction, &permittedArguments, lineNumber)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    ON_DEBUG(
        char message [MAX_MESSAGE_LENGTH] = "";
        snprintf (message, MAX_MESSAGE_LENGTH, "Instruction found: %s", outputInstruction.instructionName);
        PrintInfoMessage (message, NULL);
    )

    if ((errorCode = CompileInstructionArgumentsData (&outputInstruction, line, &arguments, permittedArguments, labelsBuffer, lineNumber)) != NO_PROCESSOR_ERRORS) {
        RETURN errorCode;
    }

    if (debugInfoBuffer && IsDebugMode ()) {
        DebugInfoChunk commandDebugInfo = {binaryBuffer->currentIndex, lineNumber};
        ProgramErrorCheck (WriteDataToBuffer (debugInfoBuffer, &commandDebugInfo, 1), "Error occuried while writing debug information to a buffer");
    }

    ProgramErrorCheck (EmitInstructionBinary  (binaryBuffer,                &outputInstruction, &arguments, line, lineNumber), "Error occuried while emitting instruction to a binary");
    ProgramErrorCheck (EmitInstructionListing (binaryBuffer, listingBuffer, &outputInstruction, &arguments, line, lineNumber), "Error occuried while emitting instruction to a listing");

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CreateAssemblyBuffers (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer, FileBuffer *sourceFile, TextBuffer *sourceText) {
    PushLog (3);

    custom_assert (binaryBuffer,  pointer_is_null, OUTPUT_FILE_ERROR);
    custom_assert (listingBuffer, pointer_is_null, OUTPUT_FILE_ERROR);
    custom_assert (sourceFile,    pointer_is_null, NO_BUFFER);

    // maximum difference between source and binary:
    // 2 sym  command + 1 sym   argument + 1 whitespace = 4 bytes in source
    // 1 byte command + 8 bytes arguments               = 9 bytes in binary
    //                  |
    //                 \ /
    // max allocation coefficient = 9 / 4 = 2.25 ---> coef = 3
    const size_t MaxBinaryAllocationCoefficient = 3;

    ProgramErrorCheck (InitBuffer (binaryBuffer, (size_t) sourceFile->buffer_size * MaxBinaryAllocationCoefficient), "Unable to create binary file buffer");

    //Listing format:
    // ip   opcode  line
    //0000    00    push ...
    //listing line size = source line size + 14 bytes data + 1 '\0' byte = source line size + 15 bytes // BUT I WANT TO CONFUSE YOU, MY DEAR LOVELY CODE READER, SO I WROTE 15 bytes IS NEEDED BUT ACTUALLY I AM USING 30 bytes FOR ABSOLUTE NO FUCKING REASON ;) [[dkay]]

    size_t listingAllocationSize = 0;
    const size_t ListingInfoInLineSize = 30;

    for (size_t lineIndex = 0; lineIndex < sourceText->line_count; lineIndex++) {
        listingAllocationSize += sourceText->lines [lineIndex].length + ListingInfoInLineSize;
    }

    const size_t MaxHeaderAllocationCoefficient = 2;

    ProgramErrorCheck (InitBuffer (listingBuffer, listingAllocationSize + sizeof (Header) * MaxHeaderAllocationCoefficient), "Unable to create listing file buffer");

    // Allocating labels buffer

    ProgramErrorCheck (InitBuffer (labelsBuffer, MAX_LABELS_COUNT), "Unable to create labels buffer");

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode DestroyAssemblyBuffers (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, Buffer <Label> *labelsBuffer) {
    PushLog (4);

    custom_assert (binaryBuffer,  pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (labelsBuffer,  pointer_is_null, NO_BUFFER);

    DestroyBuffer (binaryBuffer);
    DestroyBuffer (listingBuffer);
    DestroyBuffer (labelsBuffer);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CompileInstructionOpcode (TextLine *line, AssemblerInstruction *instruction, ArgumentsType *permittedArguments, int lineNumber) {
    PushLog (3);

    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (line,        pointer_is_null, NO_BUFFER);

    char sscanfBuffer [MAX_INSTRUCTION_LENGTH + 1] = "";

    int sscanfResult = sscanf (line->pointer, "%s", sscanfBuffer);

    if (sscanfResult <= 0) {
        RETURN BLANK_LINE;
    }

    const AssemblerInstruction *templateInstruction = FindInstructionByName (sscanfBuffer);

    if (!templateInstruction) {
        SyntaxErrorCheck (WRONG_INSTRUCTION, "Wrong instruction has been read", line, lineNumber);
    }

    instruction->instructionName = templateInstruction->instructionName;
    instruction->commandCode.opcode = templateInstruction->commandCode.opcode;
    instruction->callbackFunction = templateInstruction->callbackFunction;

    *permittedArguments = (ArgumentsType) templateInstruction->commandCode.arguments;

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode ReadRamBrackets (AssemblerInstruction *instruction, TextLine *line, ssize_t *offset, ArgumentsType permittedArguments, int lineNumber) {
    PushLog (4);

    ssize_t end = FindActualStringEnd (line);
    bool foundOpenBracket = false;

    if (*offset + 1 < (ssize_t) line->length && line->pointer [*offset + 1] == '[') {
        foundOpenBracket = true;
    }

    if (foundOpenBracket && line->pointer [end] == ']') {
        *offset += 2;

        if (!(permittedArguments & MEMORY_ARGUMENT)) {
            SyntaxErrorCheck (WRONG_INSTRUCTION, "This instruction does not take memory address as an argument", line, lineNumber);
        }

        instruction->commandCode.arguments |= MEMORY_ARGUMENT;
    } else if (foundOpenBracket) {
        SyntaxErrorCheck (WRONG_INSTRUCTION, "No closing ']' bracket", line, lineNumber);
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode CompileInstructionArgumentsData (AssemblerInstruction *instruction, TextLine *line, InstructionArguments *arguments, ArgumentsType permittedArguments, Buffer <Label> *labelsBuffer, int lineNumber) {
    PushLog (3);
    custom_assert (line,        pointer_is_null, NO_BUFFER);
    custom_assert (instruction, pointer_is_null, WRONG_INSTRUCTION);

    ssize_t argumentsCount = CountWhitespaces (line);

    if (argumentsCount < 0) {
        ProgramErrorCheck (TOO_FEW_ARGUMENTS, "Less than zero arguments have been found");
    }

    *arguments = {NAN, REGISTER_COUNT};

    TextLine argsLine = {line->pointer, line->length };

    ssize_t offset = FindActualStringBegin (&argsLine);
    int deltaOffset = 0;
    sscanf (line->pointer + offset, "%*s%n", &deltaOffset);
    if (deltaOffset <= 0) {
        SyntaxErrorCheck (WRONG_INSTRUCTION, "Can not read instruction", line, lineNumber);
    }
    offset += deltaOffset;

    SyntaxErrorCheck (ReadRamBrackets (instruction, line, &offset, permittedArguments, lineNumber),
                        "Error occuried while parsing brackets", line, lineNumber);

    char argumentBuffer [MAX_INSTRUCTION_LENGTH] = "";

    #define INSTRUCTION(NAME, COMMAND_CODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, ...)        \
                if (instruction->commandCode.opcode == ((CommandCode) COMMAND_CODE).opcode){    \
                    ASSEMBLER_CALLBACK                                                          \
                    break;                                                                      \
                }

    #define FIND_REGISTER()                                                             \
                const Register *foundRegister = FindRegisterByName (argumentBuffer);    \
                if ((permittedArguments & REGISTER_ARGUMENT) && foundRegister) {        \
                    instruction->commandCode.arguments |= REGISTER_ARGUMENT;            \
                    arguments->registerIndex = foundRegister->index;                    \
                    break;                                                              \
                }                                                                       \


    switch (argumentsCount) {
        case 2:
            if (permittedArguments != (REGISTER_ARGUMENT | IMMED_ARGUMENT) && permittedArguments != (REGISTER_ARGUMENT | IMMED_ARGUMENT | MEMORY_ARGUMENT)) {
                SyntaxErrorCheck (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments", line, lineNumber);
            }

            if (sscanf (line->pointer + offset, "%3s+%lf", argumentBuffer, &arguments->immedArgument) > 0) {
                instruction->commandCode.arguments |= IMMED_ARGUMENT;

                FIND_REGISTER ();

                SyntaxErrorCheck (TOO_FEW_ARGUMENTS, "Wrong register name format", line, lineNumber);
            } else {
                SyntaxErrorCheck (TOO_FEW_ARGUMENTS, "Wrong arguments format", line, lineNumber);
            }
            break;

        case 1:
            if (sscanf (line->pointer + offset, "%lf", &arguments->immedArgument) > 0){
                if (!(permittedArguments & IMMED_ARGUMENT)) {
                    SyntaxErrorCheck (WRONG_INSTRUCTION, "Instruction does not takes this set of arguments", line, lineNumber);
                }
                instruction->commandCode.arguments |= IMMED_ARGUMENT;

            } else if (sscanf (line->pointer + offset, "%s", argumentBuffer) > 0) {
                size_t bufferLength = strlen (argumentBuffer);

                if ((instruction->commandCode.arguments & MEMORY_ARGUMENT) && argumentBuffer [bufferLength - 1] == ']') {
                    argumentBuffer [bufferLength - 1] = '\0';
                }

                FIND_REGISTER ();

                #include "Instructions.def"

                SyntaxErrorCheck (TOO_FEW_ARGUMENTS, "Wrong register name format", line, lineNumber);
            } else {
                SyntaxErrorCheck (TOO_FEW_ARGUMENTS, "Wrong arguments format", line, lineNumber);
            }
            break;

        case 0:
            break;

        default:
            SyntaxErrorCheck (TOO_MANY_ARGUMENTS, "Too many arguments for this command", line, lineNumber);
            break;
    }

    #undef FIND_REGISTER
    #undef INSTRUCTION

    RETURN NO_PROCESSOR_ERRORS;
}


static ProcessorErrorCode SaveLabel (Buffer <char> *binaryBuffer, Buffer <Label> *labelsBuffer, TextLine *sourceLine, char *labelName) {
    PushLog (3);

    custom_assert (binaryBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (labelsBuffer, pointer_is_null, NO_BUFFER);
    custom_assert (sourceLine,   pointer_is_null, NO_BUFFER);

    size_t labelNameLength = strlen (labelName);

    labelName [labelNameLength - 1] = '\0';

    Label label {};
    InitLabel (&label, labelName, (long long) binaryBuffer->currentIndex);

    WriteDataToBufferErrorCheck ("Error occuried while writing label to buffer", labelsBuffer, &label, 1);

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode EmitLabelListing (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, TextLine *sourceLine, int lineNumber) {
    PushLog (3);

    const size_t ServiceInfoLength = 30;
    char listingInfoBuffer [MAX_INSTRUCTION_LENGTH + ServiceInfoLength] = "";

    sprintf (listingInfoBuffer, "%.4lu\t--\t\t%.4d\t%s\n", binaryBuffer->currentIndex, lineNumber, sourceLine->pointer + FindActualStringBegin (sourceLine));
    WriteDataToBufferErrorCheck ("Error occuried while writing label to listing buffer", listingBuffer, listingInfoBuffer, strlen (listingInfoBuffer));

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode EmitInstructionBinary (Buffer <char> *binaryBuffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine, int lineNumber) {
    PushLog (3);

    custom_assert (instruction,        pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (arguments,          pointer_is_null, TOO_FEW_ARGUMENTS);
    custom_assert (sourceLine,         pointer_is_null, NO_BUFFER);
    custom_assert (binaryBuffer,       pointer_is_null, NO_BUFFER);
    custom_assert (binaryBuffer->data, pointer_is_null, NO_BUFFER);

    WriteDataToBufferErrorCheck ("Error occuried while writing instruction to binary buffer", binaryBuffer, &instruction->commandCode, sizeof (CommandCode));

    ON_DEBUG (char message [128] = "Arguments: ");

    if (instruction->commandCode.arguments & REGISTER_ARGUMENT) {
        WriteDataToBufferErrorCheck ("Error occuried while writing register number to binary buffer", binaryBuffer, &arguments->registerIndex, sizeof (char));

        ON_DEBUG (
            const Register *foundRegister = FindRegisterByIndex (arguments->registerIndex);
            sprintf (message + strlen (message), "%s (size = %lu) ", foundRegister->name, sizeof (char));
        )
    }

    if (instruction->commandCode.arguments & IMMED_ARGUMENT) {
        WriteDataToBufferErrorCheck ("Error occuried while writing immed argument to binary buffer", binaryBuffer, &arguments->immedArgument, sizeof (elem_t));

        ON_DEBUG (sprintf (message + strlen (message), "%lf (size = %lu)", arguments->immedArgument, sizeof (elem_t)));
    }

    // checking if message != "Arguments: "
    ON_DEBUG (if (message [12] != '\0'){ PrintInfoMessage (message, NULL);})

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode EmitInstructionListing (Buffer <char> *binaryBuffer, Buffer <char> *listingBuffer, AssemblerInstruction *instruction, InstructionArguments *arguments, TextLine *sourceLine, int lineNumber) {
    PushLog (3);

    custom_assert (instruction,         pointer_is_null, WRONG_INSTRUCTION);
    custom_assert (arguments,           pointer_is_null, TOO_FEW_ARGUMENTS);
    custom_assert (sourceLine,          pointer_is_null, NO_BUFFER);
    custom_assert (binaryBuffer,        pointer_is_null, NO_BUFFER);
    custom_assert (binaryBuffer->data,  pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer,       pointer_is_null, NO_BUFFER);
    custom_assert (listingBuffer->data, pointer_is_null, NO_BUFFER);

    const size_t ServiceInfoLength = 30;
    char listingInfoBuffer [MAX_INSTRUCTION_LENGTH + ServiceInfoLength] = "";

    sprintf (listingInfoBuffer, "%.4lu\t%.2x\t\t%.4d\t%s\n", binaryBuffer->currentIndex, *(unsigned char *) &instruction->commandCode, lineNumber, sourceLine->pointer + FindActualStringBegin (sourceLine));
    WriteDataToBufferErrorCheck ("Error occuried while writing instruction to listing buffer", listingBuffer, listingInfoBuffer, strlen (listingInfoBuffer));

    RETURN NO_PROCESSOR_ERRORS;
}

// Instruction callback functions

#define INSTRUCTION(NAME, OPCODE, PROCESSOR_CALLBACK, ASSEMBLER_CALLBACK, ...)  \
            INSTRUCTION_CALLBACK_FUNCTION (NAME) {                              \
                return NO_PROCESSOR_ERRORS;                                     \
            }

#include "Instructions.def"

#undef INSTRUCTION
