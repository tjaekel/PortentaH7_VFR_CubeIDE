/*
 * lex.c
 *
 * Created: 11/27/2011 5:39:25 PM
 *  Author: tjaekel
 */ 

#include "picoc.h"

#ifdef NO_CTYPE
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalnum(c) (isalpha(c) || isdigit(c))
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#endif
//extended for UVM:
#define isCidstart(c) (isalpha(c) || (c)=='_' || (c)=='#' || (c)=='`' || (c)=='$')
#define isCident(c) (isalnum(c) || (c)=='_')

#define IS_HEX_ALPHA_DIGIT(c) (((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define IS_BASE_DIGIT(c,b) (((c) >= '0' && (c) < '0' + (((b)<10)?(b):10)) || (((b) > 10) ? IS_HEX_ALPHA_DIGIT(c) : FALSE))
#define GET_BASE_DIGIT(c) (((c) <= '9') ? ((c) - '0') : (((c) <= 'F') ? ((c) - 'A' + 10) : ((c) - 'a' + 10)))

#define NEXTIS(c,x,y) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else GotToken = (y); }
#define NEXTIS3(c,x,d,y,z) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else NEXTIS(d,y,z) }
#define NEXTIS4(c,x,d,y,e,z,a) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else NEXTIS3(d,y,e,z,a) }
#define NEXTIS3PLUS(c,x,d,y,e,z,a) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else if (NextChar == (d)) { if (Lexer->Pos[1] == (e)) { LEXER_INCN(Lexer, 2); GotToken = (z); } else { LEXER_INC(Lexer); GotToken = (y); } } else GotToken = (a); }
#define NEXTISEXACTLY3(c,d,y,z) { if (NextChar == (c) && Lexer->Pos[1] == (d)) { LEXER_INCN(Lexer, 2); GotToken = (y); } else GotToken = (z); }

#ifdef FANCY_ERROR_REPORTING
#define LEXER_INC(l) ( (l)->Pos++, (l)->CharacterPos++ )
#define LEXER_INCN(l, n) ( (l)->Pos+=(n), (l)->CharacterPos+=(n) )
#define TOKEN_DATA_OFFSET 2
#else
#define LEXER_INC(l) (l)->Pos++
#define LEXER_INCN(l, n) (l)->Pos+=(n)
#define TOKEN_DATA_OFFSET 1
#endif

#define MAX_CHAR_VALUE 255      /* maximum value which can be represented by a "char" data type */

static union AnyValue LexAnyValue;
static struct Value LexValue = { (TValueType *)TypeVoid, &LexAnyValue, (struct Value *)NULL, FALSE, FALSE, FALSE };

/* global */ unsigned long GLine;

struct ReservedWord
{
    const char *Word;
    const enum LexToken Token;
    char *SharedWord; 			/* word stored in shared string space */
};

static struct ReservedWord ReservedWords[] =
{
    { "#define", TokenHashDefine, NULL },
    { "`define", TokenHashDefine, NULL },
    { "#undef", TokenHashUndef, NULL },
    { "`undef", TokenHashUndef, NULL },
    { "#include", TokenHashInclude, NULL },
    { "`include", TokenHashInclude, NULL },
    { "#ifdef", TokenIfDefine, NULL },
    { "`ifdef", TokenIfDefine, NULL },
    { "#ifndef", TokenIfNDefine, NULL },
    { "`ifndef", TokenIfNDefine, NULL },
    { "#if", TokenIfDefineVal, NULL },
    { "`if", TokenIfDefineVal, NULL },
    { "#else", TokenElseDefine, NULL },
    { "`else", TokenElseDefine, NULL },
    { "#endif", TokenEndDefine, NULL },
    { "`endif", TokenEndDefine, NULL },
    { "break", TokenBreak, NULL },
    { "case", TokenCase, NULL },
    { "char", TokenCharType, NULL },
    { "continue", TokenContinue, NULL },
    { "default", TokenDefault, NULL },
    { "delete", TokenDelete, NULL },      /* special: delete is used to undefine a variable/function! */
    { "do", TokenDo, NULL },
#ifndef NO_FP
    { "double", TokenDoubleType, NULL },
#endif
    { "else", TokenElse, NULL },
    { "enum", TokenEnumType, NULL },
#ifndef NO_FP
    { "float", TokenFloatType, NULL },
#endif
    { "for", TokenFor, NULL },
    { "if", TokenIf, NULL },
    { "int", TokenIntType, NULL },
    { "long", TokenLongType, NULL },
    /* { "new", TokenNew, NULL }, */      /* new is not really defined, nothing done, parser error */
    { "return", TokenReturn, NULL },
    { "short", TokenShortType, NULL },
    { "signed", TokenSignedType, NULL },
    { "sizeof", TokenSizeof, NULL },
    { "struct", TokenStructType, NULL },
    { "switch", TokenSwitch, NULL },
    /* { "typedef", TokenTypedef, NULL }, */   /* not really defined */
    { "union", TokenUnionType, NULL },
    { "unsigned", TokenUnsignedType, NULL },
    { "void", TokenVoidType, NULL },
    //extend
    { "uint32_t", TokenUint32Type, NULL },
    { "int32_t", TokenInt32Type, NULL },
    { "uint16_t", TokenUint16Type, NULL },
    { "int16_t", TokenInt16Type, NULL },
    { "uint8_t", TokenUint8Type, NULL },
    { "int8_t", TokenInt8Type, NULL },

    { "while", TokenWhile, NULL },

    /* extended for UVM */
    { "begin", TokenLeftBrace, NULL },
    { "end", TokenRightBrace, NULL },
    { "input", TokenIgnore, NULL },
    { "output", TokenIgnore, NULL },
    { "inout", TokenIgnore, NULL }
};

/* linked list of tokens used in interactive mode */
struct TokenLine
{
    struct TokenLine *Next;
    unsigned char *Tokens;
    int NumBytes;
};

static struct TokenLine *InteractiveHead = NULL;
static struct TokenLine *InteractiveTail = NULL;
static struct TokenLine *InteractiveCurrentLine = NULL;
static int LexUseStatementPrompt = FALSE;


/* initialise the lexer */
void LexInit(void)
{
    unsigned int Count;

    LexValue.Typ = (TValueType *)TypeVoid;
    LexValue.Val = &LexAnyValue;
    LexValue.LValueFrom = NULL;
    LexValue.ValOnHeap = FALSE;
    LexValue.ValOnStack = FALSE;
    LexValue.IsLValue = FALSE;
    InteractiveHead = NULL;
    InteractiveTail = NULL;
    InteractiveCurrentLine = NULL;
    LexUseStatementPrompt = FALSE;

    for (Count = 0; Count < sizeof(ReservedWords) / sizeof(struct ReservedWord); Count++)
        ReservedWords[Count].SharedWord = TableStrRegister(ReservedWords[Count].Word);
}

void LexRelease(void)
{
    InteractiveHead = NULL;
    InteractiveTail = NULL;
}

/* deallocate */
void LexCleanup(void)
{
    LexInteractiveClear(NULL);
}

/* check if a word is a reserved word - used while scanning */
enum LexToken LexCheckReservedWord(const char *Word)
{
    unsigned int Count;

    for (Count = 0; Count < sizeof(ReservedWords) / sizeof(struct ReservedWord); Count++)
    {
        if (Word == ReservedWords[Count].SharedWord)
            return ReservedWords[Count].Token;
    }

    return TokenNone;
}

int LexLookAhead(struct LexState *Lexer)
{
    struct LexState lexer;
    struct LexState *pLexer = &lexer;

    //keep the original untouched
    memcpy(pLexer, Lexer, sizeof(struct LexState));

    while ((*pLexer->Pos >= '0') && (*pLexer->Pos <= '9'))
        LEXER_INC(pLexer);

    //check if it is a single quote
    if (*pLexer->Pos == '\'')
    {
        LEXER_INC(pLexer);
        if (*pLexer->Pos == 'h')
            return 1;
    }
    return 0;
}

int LexLookAheadTickH(struct LexState *Lexer)
{
    struct LexState lexer;
    struct LexState *pLexer = &lexer;

    //keep the original untouched
    memcpy(pLexer, Lexer, sizeof(struct LexState));

    //check if it we have a h after ' and a digit
    if (*pLexer->Pos == 'h')
    {
        LEXER_INC(pLexer);
        if (isdigit((unsigned char)*pLexer->Pos))
            return 1;
    }
    return 0;
}

/* get a numeric literal - used while scanning */
enum LexToken LexGetNumber(struct LexState *Lexer, struct Value *Value)
{
    unsigned long Result = 0;
    int sawValSpec;
    int Base = 10;
    enum LexToken ResultToken;
#ifndef NO_FP
    double FPResult;
    double FPDiv;
#endif

    //for UVM: recognize 16'hxxxx or 'hxxxx
    sawValSpec = LexLookAhead(Lexer) || LexLookAheadTickH(Lexer);
    if (sawValSpec)
    {
        while ((*Lexer->Pos >= '0') && (*Lexer->Pos <= '9'))
        {
            //TODO: save the format, 16h should be short, long, etc. - but we do not need really
            LEXER_INC(Lexer);
        }

        //for UVM: the ' in 16h'xx
        if (*Lexer->Pos == '\'')        //' it is ment here in 16'hxx
            LEXER_INC(Lexer);

        if (Lexer->Pos != Lexer->End)
        {
            if (*Lexer->Pos == 'x' || *Lexer->Pos == 'X' || *Lexer->Pos == 'h' || *Lexer->Pos == 'H')
                { Base = 16; LEXER_INC(Lexer); }
            else if (*Lexer->Pos == 'd' || *Lexer->Pos == 'D')
                { Base = 16; LEXER_INC(Lexer); }
            else if (*Lexer->Pos == 'b' || *Lexer->Pos == 'B')
                { Base = 2; LEXER_INC(Lexer); }
            else if (*Lexer->Pos != '.')
                Base = 8;
        }
    }
    else
    {
        if (*Lexer->Pos == '0')   //it accepts just 0xh or 0xb
        {
            /* a binary, octal or hex literal */
            LEXER_INC(Lexer); //for UVM
            if (Lexer->Pos != Lexer->End)
            {
                if (*Lexer->Pos == 'x' || *Lexer->Pos == 'X' || *Lexer->Pos == 'h' || *Lexer->Pos == 'H')
                    { Base = 16; LEXER_INC(Lexer); }
                else if (*Lexer->Pos == 'b' || *Lexer->Pos == 'B')
                    { Base = 2; LEXER_INC(Lexer); }
                else if (*Lexer->Pos != '.')
                    Base = 8;
            }
        }
    }

    /* get the value */
    for (; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base); LEXER_INC(Lexer))
        Result = Result * Base + GET_BASE_DIGIT(*Lexer->Pos);

    if (Result <= MAX_CHAR_VALUE)
    {
        Value->Typ = &CharType;
        Value->Val->Character = (unsigned char)Result;
        ResultToken = TokenCharacterConstant;
    }
    else
    {
        Value->Typ = &IntType;
        Value->Val->Integer = Result;
        ResultToken = TokenIntegerConstant;
    }
#ifndef NO_FP
    if (Lexer->Pos == Lexer->End || *Lexer->Pos != '.')
        return ResultToken;

    Value->Typ = &FPType;
    LEXER_INC(Lexer);
    for (FPDiv = 1.0/Base, FPResult = (double)Result; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base); LEXER_INC(Lexer), FPDiv /= (double)Base)
        FPResult += GET_BASE_DIGIT(*Lexer->Pos) * FPDiv;

    if (Lexer->Pos != Lexer->End && (*Lexer->Pos == 'e' || *Lexer->Pos == 'E'))
    {
        double ExponentMultiplier = 1.0;

        LEXER_INC(Lexer);
        if (Lexer->Pos != Lexer->End && *Lexer->Pos == '-')
        {
            ExponentMultiplier = -1.0;
            LEXER_INC(Lexer);
        }

        for (Result = 0; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base); LEXER_INC(Lexer))
            Result = Result * (int)((double)Base + GET_BASE_DIGIT(*Lexer->Pos));

        FPResult *= math_pow((double)Base, (double)Result * ExponentMultiplier);
    }

    Value->Val->FP = FPResult;

    return TokenFPConstant;
#else
    return ResultToken;
#endif
}

/* get a reserved word or identifier - used while scanning */
enum LexToken LexGetWord(struct LexState *Lexer, struct Value *Value)
{
    const char *StartPos;
    enum LexToken Token;

    StartPos = Lexer->Pos;
    do {
        LEXER_INC(Lexer);
    } while (Lexer->Pos != Lexer->End && isCident((unsigned char)*Lexer->Pos));

    Value->Typ = NULL;
    Value->Val->Identifier = TableStrRegister2(StartPos, Lexer->Pos - StartPos);

    Token = LexCheckReservedWord(Value->Val->Identifier);
    if (Token != TokenNone)
        return Token;

    //UVM: try now if we have a `MACRO use
    if (*StartPos == '`')
        Value->Val->Identifier = TableStrRegister2(StartPos+1, Lexer->Pos - (StartPos+1));
    return TokenIdentifier;
}

/* unescape a character from an octal character constant */
unsigned char LexUnEscapeCharacterConstant(const char **From, const char *End, unsigned char FirstChar, int Base)
{
    (void)End;

    unsigned char Total = GET_BASE_DIGIT(FirstChar);
    int CCount;
    for (CCount = 0; IS_BASE_DIGIT(**From, Base) && CCount < 2; CCount++, (*From)++)
        Total = Total * Base + GET_BASE_DIGIT(**From);

    return Total;
}

/* unescape a character from a string or character constant */
unsigned char LexUnEscapeCharacter(const char **From, const char *End)
{
    unsigned char ThisChar;

    while ( *From != End && **From == '\\' &&
            &(*From)[1] != End && (*From)[1] == '\n')
        (*From) += 2;       /* skip escaped end of lines */

    if (*From == End)
        return '\\';

    if (**From == '\\')
    {
        /* it's escaped */
        (*From)++;
        if (*From == End)
            return '\\';

        ThisChar = *(*From)++;
        switch (ThisChar)
        {
            case '\\': return '\\';
            case '\'': return '\'';
            case '"':  return '"';
            case 'a':  return '\a';
            case 'b':  return '\b';
            case 'f':  return '\f';
            case 'n':  return '\n';
            case 'r':  return '\r';
            case 't':  return '\t';
            case 'v':  return '\v';
            case '0': case '1': case '2': case '3': return LexUnEscapeCharacterConstant(From, End, ThisChar, 8);
            case 'x': return LexUnEscapeCharacterConstant(From, End, '0', 16);
            default:   return ThisChar;
        }
    }
    else
        return *(*From)++;
}

/* get a string constant - used while scanning */
enum LexToken LexGetStringConstant(struct LexState *Lexer, struct Value *Value)
{
    int Escape = FALSE;
    const char *StartPos = Lexer->Pos;
    const char *EndPos;
    char *EscBuf;
    char *EscBufPos;
    char *RegString;
    struct Value *ArrayValue;

    while (Lexer->Pos != Lexer->End && (*Lexer->Pos != '"' || Escape))
    {
        /* find the end */
        if (Escape)
            Escape = FALSE;
        else if (*Lexer->Pos == '\\')
            Escape = TRUE;

        LEXER_INC(Lexer);
    }
    EndPos = Lexer->Pos;

    EscBuf = HeapAllocStack(EndPos - StartPos);
    if (EscBuf == NULL)
        LexFail(Lexer, "out of memory");

    for (EscBufPos = EscBuf, Lexer->Pos = StartPos; Lexer->Pos != EndPos;)
        *EscBufPos++ = LexUnEscapeCharacter(&Lexer->Pos, EndPos);

    /* try to find an existing copy of this string literal */
    RegString = TableStrRegister2(EscBuf, EscBufPos - EscBuf);
    HeapPopStack(EscBuf, EndPos - StartPos);
    ArrayValue = VariableStringLiteralGet(RegString);
    if (ArrayValue == NULL)
    {
        /* create and store this string literal */
        ArrayValue = VariableAllocValueAndData(NULL, 0, FALSE, NULL, TRUE);
        ArrayValue->Typ = CharArrayType;
        ArrayValue->Val = (union AnyValue *)RegString;
        VariableStringLiteralDefine(RegString, ArrayValue);
    }

    /* create the the pointer for this char* */
    Value->Typ = CharPtrType;
    Value->Val->NativePointer = RegString;
    if (*Lexer->Pos == '"')
        LEXER_INC(Lexer);

    return TokenStringConstant;
}

/* get a character constant - used while scanning */
enum LexToken LexGetCharacterConstant(struct LexState *Lexer, struct Value *Value)
{
    Value->Typ = &CharType;
    Value->Val->Character = LexUnEscapeCharacter(&Lexer->Pos, Lexer->End);
    if (Lexer->Pos != Lexer->End && *Lexer->Pos != '\'')
        LexFail(Lexer, "expected \"'\"");

    LEXER_INC(Lexer);
    return TokenCharacterConstant;
}

/* skip a comment - used while scanning */
void LexSkipComment(struct LexState *Lexer, char NextChar)
{
    LEXER_INC(Lexer);
    if (NextChar == '*')
    {
        /* conventional C comment */
        while (Lexer->Pos != Lexer->End && (*(Lexer->Pos-1) != '*' || *Lexer->Pos != '/'))
        {
            /* it can be also a multi-line comment, increment the line counter */
            if (*Lexer->Pos == '\n')
                Lexer->Line++;
            LEXER_INC(Lexer);
        }

        if (Lexer->Pos != Lexer->End)
            LEXER_INC(Lexer);
    }
    else
    {
        /* C++ style comment */
        while (Lexer->Pos != Lexer->End && *Lexer->Pos != '\n')
            LEXER_INC(Lexer);
    }
}

/* get a single token from the source - used while scanning */
enum LexToken LexScanGetToken(struct LexState *Lexer, struct Value **Value)
{
    unsigned char ThisChar;
    unsigned char NextChar;
    enum LexToken GotToken;
    int foundLineConcat;

    GotToken = TokenNone;

    foundLineConcat = 0;
    do
    {
        *Value = &LexValue;
#if 0
        while (Lexer->Pos != Lexer->End && isspace(*Lexer->Pos))
#else
    while (Lexer->Pos < Lexer->End && isspace((unsigned char)*Lexer->Pos))
#endif
        {
            if (*Lexer->Pos == '\n')
            {
                Lexer->Line++;
                Lexer->Pos++;
#ifdef FANCY_ERROR_REPORTING
                Lexer->CharacterPos = 0;
#endif
                if ( ! foundLineConcat)
                    return TokenEndOfLine;
            }

            if ( ! foundLineConcat)
                LEXER_INC(Lexer);
            else
                foundLineConcat = 0;
        }

#if 0
        if (Lexer->Pos == Lexer->End || *Lexer->Pos == '\0')
#else
        if (Lexer->Pos >= Lexer->End || *Lexer->Pos == '\0')
#endif
            return TokenEOF;

        ThisChar = *Lexer->Pos;
        if (isCidstart(ThisChar))
            return LexGetWord(Lexer, *Value);

        if (isdigit(ThisChar))
            return LexGetNumber(Lexer, *Value);

        NextChar = (Lexer->Pos+1 != Lexer->End) ? *(Lexer->Pos+1) : 0;
        LEXER_INC(Lexer);
        switch (ThisChar)
        {
            case '"': GotToken = LexGetStringConstant(Lexer, *Value); break;
            case '\'':
                      //UVM:
                      if (LexLookAheadTickH(Lexer))
                      {
                        return LexGetNumber(Lexer, *Value);
                      }
                      else
                      {
                        GotToken = LexGetCharacterConstant(Lexer, *Value);
                      }
                      break;
            case '(': GotToken = TokenOpenBracket; break;
            case ')': GotToken = TokenCloseBracket; break;
            case '=': NEXTIS('=', TokenEqual, TokenAssign); break;
            case '+': NEXTIS3('=', TokenAddAssign, '+', TokenIncrement, TokenPlus); break;
            case '-': NEXTIS4('=', TokenSubtractAssign, '>', TokenArrow, '-', TokenDecrement, TokenMinus); break;
            case '*': NEXTIS('=', TokenMultiplyAssign, TokenAsterisk); break;
            case '/': if (NextChar == '/' || NextChar == '*') LexSkipComment(Lexer, NextChar); else NEXTIS('=', TokenDivideAssign, TokenSlash); break;
            case '%': NEXTIS('=', TokenModulusAssign, TokenModulus); break;
            case '<': NEXTIS3PLUS('=', TokenLessEqual, '<', TokenShiftLeft, '=', TokenShiftLeftAssign, TokenLessThan); break;
            case '>': NEXTIS3PLUS('=', TokenGreaterEqual, '>', TokenShiftRight, '=', TokenShiftRightAssign, TokenGreaterThan); break;
            case ';': GotToken = TokenSemicolon; break;
            case '&': NEXTIS3('=', TokenArithmeticAndAssign, '&', TokenLogicalAnd, TokenAmpersand); break;
            case '|': NEXTIS3('=', TokenArithmeticOrAssign, '|', TokenLogicalOr, TokenArithmeticOr); break;
            case '{': GotToken = TokenLeftBrace; break;
            case '}': GotToken = TokenRightBrace; break;
            case '[': GotToken = TokenLeftSquareBracket; break;
            case ']': GotToken = TokenRightSquareBracket; break;
            case '!': NEXTIS('=', TokenNotEqual, TokenUnaryNot); break;
            case '^': NEXTIS('=', TokenArithmeticExorAssign, TokenArithmeticExor); break;
            case '~': GotToken = TokenUnaryExor; break;
            case ',': GotToken = TokenComma; break;
            case '.': NEXTISEXACTLY3('.', '.', TokenEllipsis, TokenDot); break;
            case '?': GotToken = TokenQuestionMark; break;
            case ':': GotToken = TokenColon; break;
            //handle line concatination
            case '\\': GotToken = TokenNone; foundLineConcat = 1; break;
            default:  LexFail(Lexer, "illegal character '%c'", ThisChar); break;
        }
    } while (GotToken == TokenNone);

    return GotToken;
}

/* what size value goes with each token */
int LexTokenSize(enum LexToken Token)
{
    switch (Token)
    {
        case TokenIdentifier:
        case TokenStringConstant: return sizeof(char *);
        case TokenIntegerConstant: return sizeof(int);
        case TokenCharacterConstant: return sizeof(unsigned char);
        case TokenFPConstant: return sizeof(double);
        default: return 0;
    }
}

/* produce tokens from the lexer and return a heap buffer with the result - used for scanning */
void *LexTokenise(struct LexState *Lexer, int *TokenLen)
{
    enum LexToken Token;
    void *HeapMem;
    struct Value *GotValue;
    int MemUsed = 0;
    int ValueSize;
    int ReserveSpace;
    void *TokenSpace;
    char *TokenPos;
#ifdef FANCY_ERROR_REPORTING
    int LastCharacterPos;
    LastCharacterPos = 0;
    ReserveSpace = (Lexer->End - Lexer->Pos) * 4 + 1;
#else
    ReserveSpace = (Lexer->End - Lexer->Pos) * 3 + 1;
#endif
    TokenSpace = HeapAllocStack(ReserveSpace);
    TokenPos = (char *)TokenSpace;

    if (TokenSpace == NULL)
        LexFail(Lexer, "out of memory");

    do
    {
        /* store the token at the end of the stack area */
        Token = LexScanGetToken(Lexer, &GotValue);
#ifdef DEBUG_LEXER
        printf("Token: %02x\n", Token);
#endif
        *(unsigned char *)TokenPos = Token;
        TokenPos++;
        MemUsed++;

#ifdef FANCY_ERROR_REPORTING
        *(unsigned char *)TokenPos = (unsigned char)LastCharacterPos;
        TokenPos++;
        MemUsed++;
#endif

        ValueSize = LexTokenSize(Token);
        if (ValueSize > 0)
        {
            /* store a value as well */
            memcpy((void *)TokenPos, (void *)GotValue->Val, ValueSize);
            TokenPos += ValueSize;
            MemUsed += ValueSize;
        }

#ifdef FANCY_ERROR_REPORTING
        LastCharacterPos = Lexer->CharacterPos;
#endif

    } while (Token != TokenEOF);

    HeapMem = HeapAllocMem(MemUsed);
    if (HeapMem == NULL)
        LexFail(Lexer, "out of memory");

#if 0
    assert(ReserveSpace >= MemUsed);
#else
  if (ReserveSpace < MemUsed)
    LexFail(Lexer, "unexpected error");
#endif

    memcpy(HeapMem, TokenSpace, MemUsed);
    HeapPopStack(TokenSpace, ReserveSpace);
#ifdef DEBUG_LEXER
    {
        int Count;
        printf("Tokens: ");
        for (Count = 0; Count < MemUsed; Count++)
            printf("%02x ", *((unsigned char *)HeapMem+Count));
        printf("\n");
    }
#endif
    if (TokenLen)
        *TokenLen = MemUsed;

    return HeapMem;
}

/* lexically analyse some source text */
void *LexAnalyse(const char *FileName, const char *Source, int SourceLen, int *TokenLen)
{
    struct LexState Lexer;

    Lexer.Pos = Source;
    Lexer.End = Source + SourceLen;
    Lexer.Line = 1;
    Lexer.FileName = FileName;
#ifdef FANCY_ERROR_REPORTING
    Lexer.CharacterPos = 0;
    Lexer.SourceText = Source;
#endif

    return LexTokenise(&Lexer, TokenLen);
}

/* prepare to parse a pre-tokenised buffer */
void LexInitParser(struct ParseState *Parser, const char *SourceText, void *TokenSource, const char *FileName, int RunIt)
{
#ifndef FANCY_ERROR_REPORTING
    (void)SourceText;
#endif
    Parser->Pos = TokenSource;
    Parser->Line = 1;
    Parser->FileName = FileName;
    Parser->Mode = RunIt ? RunModeRun : RunModeSkip;
    Parser->SearchLabel = 0;
#ifdef FANCY_ERROR_REPORTING
    Parser->CharacterPos = 0;
    Parser->SourceText = SourceText;
#endif
}

/* get the next token given a parser state */
enum LexToken LexGetToken(struct ParseState *Parser, struct Value **Value, int IncPos)
{
    enum LexToken Token = TokenNone;
    int ValueSize;

    do
    {
        /* get the next token */
        if (Parser->Pos == NULL && InteractiveHead != NULL)
            Parser->Pos = InteractiveHead->Tokens;

        if (Parser->FileName != StrEmpty || InteractiveHead != NULL)
        {
            /* skip leading newlines */
            while ((Token = (enum LexToken)*(unsigned char *)Parser->Pos) == TokenEndOfLine)
            {
                Parser->Line++;
                Parser->Pos += TOKEN_DATA_OFFSET;
            }
        }

        if (Parser->FileName == StrEmpty && (InteractiveHead == NULL || Token == TokenEOF))
        {
            /* we're at the end of an interactive input token list */
            char LineBuffer[LINEBUFFER_MAX];
            void *LineTokens;
            int LineBytes;
            struct TokenLine *LineNode;

            if (InteractiveHead == NULL || (unsigned char *)Parser->Pos == &InteractiveTail->Tokens[InteractiveTail->NumBytes-TOKEN_DATA_OFFSET])
            {
                /* get interactive input */
                if (LexUseStatementPrompt)
                {
                    PlatformPrintf(INTERACTIVE_PROMPT_STATEMENT);
                    LexUseStatementPrompt = FALSE;
                }
                //else
                //    PlatformPrintf(INTERACTIVE_PROMPT_LINE);

                if (PlatformGetLine(&LineBuffer[0], LINEBUFFER_MAX) == NULL)
                    return TokenEOF;

                /* put the new line at the end of the linked list of interactive lines */
                LineTokens = LexAnalyse(StrEmpty, &LineBuffer[0], strlen(LineBuffer), &LineBytes);

                LineNode = VariableAlloc(Parser, sizeof(struct TokenLine), TRUE);
                LineNode->Tokens = LineTokens;
                LineNode->NumBytes = LineBytes;
                if (InteractiveHead == NULL)
                {
                    /* start a new list */
                    InteractiveHead = LineNode;
                    Parser->Line = 1;
#ifdef FANCY_ERROR_REPORTING
                    Parser->CharacterPos = 0;
#endif
                }
                else
                    InteractiveTail->Next = LineNode;

                InteractiveTail = LineNode;
                InteractiveCurrentLine = LineNode;
                Parser->Pos = LineTokens;
            }
            else
            {
                /* go to the next token line */
                if (Parser->Pos != &InteractiveCurrentLine->Tokens[InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET])
                {
                    /* scan for the line */
                    for (InteractiveCurrentLine = InteractiveHead; Parser->Pos != &InteractiveCurrentLine->Tokens[InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET]; InteractiveCurrentLine = InteractiveCurrentLine->Next)
                    { assert(InteractiveCurrentLine->Next != NULL); }
                }

                assert(InteractiveCurrentLine != NULL);
                InteractiveCurrentLine = InteractiveCurrentLine->Next;
                assert(InteractiveCurrentLine != NULL);
                Parser->Pos = InteractiveCurrentLine->Tokens;
            }

            Token = (enum LexToken)*(unsigned char *)Parser->Pos;
        }
    } while ((Parser->FileName == StrEmpty && Token == TokenEOF) || Token == TokenEndOfLine);

#ifdef FANCY_ERROR_REPORTING
    Parser->CharacterPos = *((unsigned char *)Parser->Pos + 1);
#endif

    ValueSize = LexTokenSize(Token);
    if (ValueSize > 0)
    {
        /* this token requires a value - unpack it */
        if (Value != NULL)
        {
            switch (Token)
            {
                case TokenStringConstant:       LexValue.Typ = CharPtrType; break;
                case TokenIdentifier:           LexValue.Typ = NULL; break;
                case TokenIgnore:               LexValue.Typ = NULL; break;
                case TokenIntegerConstant:      LexValue.Typ = &IntType; break;
                case TokenCharacterConstant:    LexValue.Typ = &CharType; break;
#ifndef NO_FP
                case TokenFPConstant:           LexValue.Typ = &FPType; break;
#endif
                default: break;
            }

            memcpy((void *)LexValue.Val, (void *)((char *)Parser->Pos + TOKEN_DATA_OFFSET), ValueSize);
            LexValue.ValOnHeap = FALSE;
            LexValue.ValOnStack = FALSE;
            LexValue.IsLValue = FALSE;
            LexValue.LValueFrom = NULL;
            *Value = &LexValue;
        }

        if (IncPos)
            Parser->Pos += ValueSize + TOKEN_DATA_OFFSET;
    }
    else
    {
        if (IncPos && Token != TokenEOF)
            Parser->Pos += TOKEN_DATA_OFFSET;
    }

#ifdef DEBUG_LEXER
    printf("Got token=%02x inc=%d pos=%d\n", Token, IncPos, Parser->CharacterPos);
#endif
    /*
     * assert(Token >= TokenNone && Token <= TokenEndOfFunction);  //creates a warning!
     */
    return Token;
}

/* find the end of the line */
void LexToEndOfLine(struct ParseState *Parser)
{
    while (TRUE)
    {
        enum LexToken Token = (enum LexToken)*(unsigned char *)Parser->Pos;
        if (Token == TokenEndOfLine || Token == TokenEOF)
            return;
        else
            LexGetToken(Parser, NULL, TRUE);
    }
}

/* copy the tokens from StartParser to EndParser into new memory, removing TokenEOFs and terminate with a TokenEndOfFunction */
void *LexCopyTokens(struct ParseState *StartParser, struct ParseState *EndParser)
{
    int MemSize = 0;
    int CopySize;
    unsigned char *Pos;
    unsigned char *NewTokens;
    unsigned char *NewTokenPos;
    struct TokenLine *ILine;

    Pos = (unsigned char *)StartParser->Pos;
    if (InteractiveHead == NULL)
    {
        /* non-interactive mode - copy the tokens */
        MemSize = EndParser->Pos - StartParser->Pos;
        //what if nothing behind macro name? MemSize can be 0!
        NewTokens = VariableAlloc(StartParser, MemSize + 1, TRUE);
        memcpy(NewTokens, (void *)StartParser->Pos, MemSize);
    }
    else
    {
        /* we're in interactive mode - add up line by line */
        for (InteractiveCurrentLine = InteractiveHead; InteractiveCurrentLine != NULL && (Pos < &InteractiveCurrentLine->Tokens[0] || Pos >= &InteractiveCurrentLine->Tokens[InteractiveCurrentLine->NumBytes]); InteractiveCurrentLine = InteractiveCurrentLine->Next)
        {} /* find the line we just counted */

        //ATT: sometimes we crash here, e.g. with #include on command line
        if ( ! InteractiveCurrentLine)
        {
            //just to make sure not to hit code below with NULL
            ProgramFail(StartParser, "internal error");
            return NULL;
        }

        if (EndParser->Pos >= StartParser->Pos && EndParser->Pos < &InteractiveCurrentLine->Tokens[InteractiveCurrentLine->NumBytes])
        {
            /* all on a single line */
            MemSize = EndParser->Pos - StartParser->Pos;
            //what if nothing behind macro name? MemSize can be 0!
            NewTokens = VariableAlloc(StartParser, MemSize + 1, TRUE);
            memcpy(NewTokens, (void *)StartParser->Pos, MemSize);
        }
        else
        {
            /* it's spread across multiple lines */
            MemSize = &InteractiveCurrentLine->Tokens[InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET] - Pos;

            for (ILine = InteractiveCurrentLine->Next; ILine != NULL && (EndParser->Pos < &ILine->Tokens[0] || EndParser->Pos >= &ILine->Tokens[ILine->NumBytes]); ILine = ILine->Next)
                MemSize += ILine->NumBytes - TOKEN_DATA_OFFSET;

            assert(ILine != NULL);
            MemSize += EndParser->Pos - &ILine->Tokens[0];
            NewTokens = VariableAlloc(StartParser, MemSize + 1, TRUE);

            CopySize = &InteractiveCurrentLine->Tokens[InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET] - Pos;
            memcpy(NewTokens, Pos, CopySize);
            NewTokenPos = NewTokens + CopySize;
            for (ILine = InteractiveCurrentLine->Next; ILine != NULL && (EndParser->Pos < &ILine->Tokens[0] || EndParser->Pos >= &ILine->Tokens[ILine->NumBytes]); ILine = ILine->Next)
            {
                memcpy(NewTokenPos, &ILine->Tokens[0], ILine->NumBytes - TOKEN_DATA_OFFSET);
                NewTokenPos += ILine->NumBytes-TOKEN_DATA_OFFSET;
            }
            assert(ILine != NULL);
            memcpy(NewTokenPos, &ILine->Tokens[0], EndParser->Pos - &ILine->Tokens[0]);
        }
    }

    NewTokens[MemSize] = (unsigned char)TokenEndOfFunction;

    return NewTokens;
}

/* indicate that we've completed up to this point in the interactive input and free expired tokens */
void LexInteractiveClear(struct ParseState *Parser)
{
    while (InteractiveHead != NULL)
    {
        struct TokenLine *NextLine = InteractiveHead->Next;

        HeapFreeMem(InteractiveHead->Tokens);
        HeapFreeMem(InteractiveHead);
        InteractiveHead = NextLine;
    }

    if (Parser != NULL)
        Parser->Pos = NULL;
    InteractiveTail = NULL;
}

/* indicate that we've completed up to this point in the interactive input and free expired tokens */
void LexInteractiveCompleted(struct ParseState *Parser)
{
    while (InteractiveHead != NULL && !(Parser->Pos >= &InteractiveHead->Tokens[0] && Parser->Pos < &InteractiveHead->Tokens[InteractiveHead->NumBytes]))
    {
        /* this token line is no longer needed - free it */
        struct TokenLine *NextLine = InteractiveHead->Next;

        HeapFreeMem(InteractiveHead->Tokens);
        HeapFreeMem(InteractiveHead);
        InteractiveHead = NextLine;

        if (InteractiveHead == NULL)
        {
            /* we've emptied the list */
            Parser->Pos = NULL;
            InteractiveTail = NULL;
        }
    }
}

/* the next time we prompt, make it the full statement prompt */
void LexInteractiveStatementPrompt(void)
{
    LexUseStatementPrompt = TRUE;
}
