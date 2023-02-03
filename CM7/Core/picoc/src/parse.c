/*
 * parse.c
 *
 * Created: 11/27/2011 5:41:52 PM
 *  Author: tjaekel
 */ 

#include "picoc.h"

int GpicocAbort = 0;
int GparserIfCount = 0;
int GparserIfSkip[10] = {0,0,0,0,0,0,0,0,0,0};

/* a chunk of heap-allocated tokens we should cleanup when we're done */
static void *CleanupTokens = NULL;

/* deallocate any memory */
void ParseCleanup(void)
{
    if (CleanupTokens != NULL)
        HeapFreeMem(CleanupTokens);
}

/* parse a statement, but only run it if Condition is TRUE */
enum ParseResult ParseStatementMaybeRun(struct ParseState *Parser, int Condition, int CheckTrailingSemicolon)
{
    //check for abort or stopped
    picoc_CheckForAbort();

    if (Parser->Mode != RunModeSkip && !Condition)
    {
        enum RunMode OldMode = Parser->Mode;
        enum ParseResult Result;

        picoc_CheckForAbort();

        Parser->Mode = RunModeSkip;
        Result = ParseStatement(Parser, CheckTrailingSemicolon);
        Parser->Mode = OldMode;
        return Result;
    }
    else
        return ParseStatement(Parser, CheckTrailingSemicolon);
}

/* parse a function definition and store it for later */
struct Value *ParseFunctionDefinition(struct ParseState *Parser, struct ValueType *ReturnType, char *Identifier)
{
    struct ValueType *ParamType;
    char *ParamIdentifier;
    enum LexToken Token;
    struct Value *FuncValue;
    struct Value *OldFuncValue;
    struct ParseState ParamParser;
    struct ParseState FuncBody;
    int ParamCount = 0;

    if (TopStackFrame != NULL)
        ProgramFail(Parser, "nested function definitions are not allowed");

    LexGetToken(Parser, NULL, TRUE);  /* open bracket */
    ParamParser = *Parser;
    Token = LexGetToken(Parser, NULL, TRUE);
    if (Token != TokenCloseBracket && Token != TokenEOF)
    {
        /* count the number of parameters */
        ParamCount++;
        while ((Token = LexGetToken(Parser, NULL, TRUE)) != TokenCloseBracket && Token != TokenEOF)
        {
            if (Token == TokenComma)
                ParamCount++;
        }
    }
    if (ParamCount > PARAMETER_MAX)
        ProgramFail(Parser, "too many parameters");

    FuncValue = VariableAllocValueAndData(Parser, sizeof(struct FuncDef) + sizeof(struct ValueType *) * ParamCount + sizeof(const char *) * ParamCount, FALSE, NULL, TRUE);
    FuncValue->Typ = &FunctionType;
    FuncValue->Val->FuncDef.ReturnType = ReturnType;
    FuncValue->Val->FuncDef.NumParams = ParamCount;
    FuncValue->Val->FuncDef.VarArgs = FALSE;
    FuncValue->Val->FuncDef.ParamType = (struct ValueType **)((char *)FuncValue->Val + sizeof(struct FuncDef));
    FuncValue->Val->FuncDef.ParamName = (char **)((char *)FuncValue->Val->FuncDef.ParamType + sizeof(struct ValueType *) * ParamCount);

    for (ParamCount = 0; ParamCount < FuncValue->Val->FuncDef.NumParams; ParamCount++)
    {
        /* harvest the parameters into the function definition */
        if (ParamCount == FuncValue->Val->FuncDef.NumParams-1 && LexGetToken(&ParamParser, NULL, FALSE) == TokenEllipsis)
        {
            /* ellipsis at end */
            FuncValue->Val->FuncDef.NumParams--;
            FuncValue->Val->FuncDef.VarArgs = TRUE;
            break;
        }
        else
        {
            /* add a parameter */
            TypeParse(&ParamParser, &ParamType, &ParamIdentifier);
            FuncValue->Val->FuncDef.ParamType[ParamCount] = ParamType;
            FuncValue->Val->FuncDef.ParamName[ParamCount] = ParamIdentifier;
        }

        Token = LexGetToken(&ParamParser, NULL, TRUE);
        if (Token != TokenComma && ParamCount < FuncValue->Val->FuncDef.NumParams-1)
            ProgramFail(&ParamParser, "comma expected");
    }

    if (FuncValue->Val->FuncDef.NumParams != 0 && Token != TokenCloseBracket && Token != TokenComma && Token != TokenEllipsis)
        ProgramFail(&ParamParser, "bad parameter");

    /* look for a function body */
    Token = LexGetToken(Parser, NULL, FALSE);
    if (Token == TokenSemicolon)
        LexGetToken(Parser, NULL, TRUE);    /* it's a prototype, absorb the trailing semicolon */
    if (Token != TokenSemicolon)
    //else
    {
        /* it's a full function definition with a body */
        if (Token != TokenLeftBrace)
            ProgramFail(Parser, "bad function definition");

        FuncBody = *Parser;
        if (ParseStatementMaybeRun(Parser, FALSE, TRUE) != ParseResultOk)
            ProgramFail(Parser, "function definition expected");

        FuncValue->Val->FuncDef.Body = FuncBody;
        FuncValue->Val->FuncDef.Body.Pos = LexCopyTokens(&FuncBody, Parser);

        /* is this function already in the global table? */
        if (TableGet(&GlobalTable, Identifier, &OldFuncValue))
        {
            if (OldFuncValue->Val->FuncDef.Body.Pos == NULL)
                TableDelete(&GlobalTable, Identifier);   /* override an old function prototype */
            else
                ProgramFail(Parser, "'%s' is already defined", Identifier);
        }
    }

    if (!TableSet(&GlobalTable, Identifier, FuncValue))
        ProgramFail(Parser, "'%s' is already defined", Identifier);

    return FuncValue;
}

/* assign an initial value to a variable */
void ParseDeclarationAssignment(struct ParseState *Parser, struct Value *NewVariable)
{
    struct Value *CValue;
    int ArrayIndex;
    enum LexToken Token = TokenComma;
    enum LexToken xToken;

    xToken = LexGetToken(Parser, NULL, FALSE);
    if (xToken == TokenLeftBrace)
    //if (LexGetToken(Parser, NULL, FALSE) == TokenLeftBrace)
    {
        /* this is an array initialiser */
        LexGetToken(Parser, NULL, TRUE);

        for (ArrayIndex = 0; (Parser->Mode != RunModeRun && Token == TokenComma) || (Parser->Mode == RunModeRun && ArrayIndex < NewVariable->Typ->ArraySize); ArrayIndex++)
        {
            struct Value *ArrayElement = NULL;

            if (Token != TokenComma)
                ProgramFail(Parser, "comma expected");

            if (Parser->Mode == RunModeRun)
                ArrayElement = VariableAllocValueFromExistingData(Parser, NewVariable->Typ->FromType, (union AnyValue *)(&NewVariable->Val->ArrayMem[0] + TypeSize(NewVariable->Typ->FromType, 0, TRUE) * ArrayIndex), TRUE, NewVariable);

            if (!ExpressionParse(Parser, &CValue))
                ProgramFail(Parser, "expression expected");

            if (Parser->Mode == RunModeRun)
            {
                ExpressionAssign(Parser, ArrayElement, CValue, FALSE, NULL, 0, FALSE);
                VariableStackPop(Parser, CValue);
                VariableStackPop(Parser, ArrayElement);
            }

            Token = LexGetToken(Parser, NULL, TRUE);
        }

        if (Token == TokenComma)
            Token = LexGetToken(Parser, NULL, TRUE);

        if (Token != TokenRightBrace)
            ProgramFail(Parser, "'}' expected");
    }
    else
    {
        //TODO: add here:   char s[X] = "...";
        //but X must be known
        //check if NewVariable is TypeArray, derived from TypeChar
        //and Token is TypePointer with Identifier for the string,
        //separate into array elements

        /* this is a normal expression initialiser */
        if (!ExpressionParse(Parser, &CValue))
            ProgramFail(Parser, "expression expected");

        if (Parser->Mode == RunModeRun)
        {
            ExpressionAssign(Parser, NewVariable, CValue, FALSE, NULL, 0, FALSE);
            VariableStackPop(Parser, CValue);
        }
    }
}

/* declare a variable or function */
int ParseDeclaration(struct ParseState *Parser, enum LexToken Token)
{
    char *Identifier;
    struct ValueType *BasicType;
    struct ValueType *Typ;
    struct Value *NewVariable = NULL;

    TypeParseFront(Parser, &BasicType);

    do
    {
        TypeParseIdentPart(Parser, BasicType, &Typ, &Identifier);
        if ((Token != TokenVoidType && Token != TokenStructType && Token != TokenUnionType && Token != TokenEnumType) && Identifier == StrEmpty)
            ProgramFail(Parser, "identifier expected");

        if (Identifier != StrEmpty)
        {
            /* handle function definitions */
            if (LexGetToken(Parser, NULL, FALSE) == TokenOpenBracket)
            {
                ParseFunctionDefinition(Parser, Typ, Identifier);
                return FALSE;
            }
            else
            {
                if (Typ == &VoidType && Identifier != StrEmpty)
                    ProgramFail(Parser, "can't define a void variable");

                if (Parser->Mode == RunModeRun)
                    NewVariable = VariableDefine(Parser, Identifier, NULL, Typ, TRUE);

                if (LexGetToken(Parser, NULL, FALSE) == TokenAssign)
                {
                    /* we're assigning an initial value */
                    LexGetToken(Parser, NULL, TRUE);
                    ParseDeclarationAssignment(Parser, NewVariable);
                }
            }
        }

        Token = LexGetToken(Parser, NULL, FALSE);
        if (Token == TokenComma)
            LexGetToken(Parser, NULL, TRUE);

    } while (Token == TokenComma);

    return TRUE;
}

/* parse a #define macro definition and store it for later */
void ParseMacroDefinition(struct ParseState *Parser)
{
    struct Value *MacroName;
    struct Value *MacroValue = VariableAllocValueAndData(Parser, sizeof(struct ParseState), FALSE, NULL, TRUE);

    if (LexGetToken(Parser, &MacroName, TRUE) != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");

    MacroValue->Val->Parser = *Parser;
    MacroValue->Typ = &MacroType;
    LexToEndOfLine(Parser);
    MacroValue->Val->Parser.Pos = LexCopyTokens(&MacroValue->Val->Parser, Parser);

    if (!TableSet(&GlobalTable, MacroName->Val->Identifier, MacroValue))
        ProgramFail(Parser, "'%s' is already defined", MacroName->Val->Identifier);
}

void ParseMacroIfDefine(struct ParseState *Parser)
{
    struct Value *MacroName;
    struct Value *MacroValue;

    if (Parser->Mode != RunModeRun)
    {
        MacroValue = VariableAllocValueAndData(Parser, sizeof(struct ParseState), FALSE, NULL, TRUE);
        if (LexGetToken(Parser, &MacroName, TRUE) != TokenIdentifier)
            ProgramFail(Parser, "identifier expected");

        MacroValue->Val->Parser = *Parser;
        MacroValue->Typ = &MacroType;

        MacroValue->Val->Parser.Pos = LexCopyTokens(&MacroValue->Val->Parser, Parser);

        if (GparserIfCount == 10)
            ProgramFail(Parser, "too many nested #ifdef");
        else
        {
            GparserIfCount++;
            if ( ! TableGet(&GlobalTable, MacroName->Val->Identifier, &MacroValue))
                GparserIfSkip[GparserIfCount - 1] = 1;
            else
                GparserIfSkip[GparserIfCount - 1] = 0;
        }
    }
    else
    {
        struct Value xMacroValue;
        MacroValue = &xMacroValue;
        if (LexGetToken(Parser, &MacroName, TRUE) != TokenIdentifier)
            ProgramFail(Parser, "identifier expected");

        MacroValue->Typ = &MacroType;

        if (GparserIfCount == 10)
            ProgramFail(Parser, "too many nested #ifdef");
        else
        {
            GparserIfCount++;
            if ( ! TableGet(&GlobalTable, MacroName->Val->Identifier, &MacroValue))
                GparserIfSkip[GparserIfCount - 1] = 1;
            else
                GparserIfSkip[GparserIfCount - 1] = 0;
        }
    }
}

void ParseMacroIfNDefine(struct ParseState *Parser)
{
    struct Value *MacroName;
    struct Value *MacroValue;

    if (Parser->Mode != RunModeRun)
    {
        MacroValue = VariableAllocValueAndData(Parser, sizeof(struct ParseState), FALSE, NULL, TRUE);
        if (LexGetToken(Parser, &MacroName, TRUE) != TokenIdentifier)
            ProgramFail(Parser, "identifier expected");

        MacroValue->Val->Parser = *Parser;
        MacroValue->Typ = &MacroType;
        LexToEndOfLine(Parser);
        MacroValue->Val->Parser.Pos = LexCopyTokens(&MacroValue->Val->Parser, Parser);

        if (GparserIfCount == 10)
            ProgramFail(Parser, "too many nested #ifndef");
        else
        {
            GparserIfCount++;
            if (TableGet(&GlobalTable, MacroName->Val->Identifier, &MacroValue))
                GparserIfSkip[GparserIfCount - 1] = 1;
            else
                GparserIfSkip[GparserIfCount - 1] = 0;
        }
    }
    else
    {
        struct Value xMacroValue;
        MacroValue = &xMacroValue;
        if (LexGetToken(Parser, &MacroName, TRUE) != TokenIdentifier)
            ProgramFail(Parser, "identifier expected");

        MacroValue->Typ = &MacroType;

        if (GparserIfCount == 10)
            ProgramFail(Parser, "too many nested #ifndef");
        else
        {
            GparserIfCount++;
            if (TableGet(&GlobalTable, MacroName->Val->Identifier, &MacroValue))
                GparserIfSkip[GparserIfCount - 1] = 1;
            else
                GparserIfSkip[GparserIfCount - 1] = 0;
        }
    }
}

void ParseMacroIfDefineVal(struct ParseState *Parser)
{
    int Condition;

    if (GparserIfCount == 10)
        ProgramFail(Parser, "too many nested #if");
    else
    {
        struct Value *LexValue;
        enum LexToken Token = LexGetToken(Parser, &LexValue, TRUE);
        if (Token == TokenCharacterConstant)
        {
            Condition = LexValue->Val->Character;

            GparserIfCount++;
            if ( ! Condition)
                GparserIfSkip[GparserIfCount - 1] = 1;
            else
                GparserIfSkip[GparserIfCount - 1] = 0;
        }
        else
            ProgramFail(Parser, "#if value expected");
    }
}

void ParseMacroElseDefine(struct ParseState *Parser)
{
    if (GparserIfCount)
    {
        if (GparserIfSkip[GparserIfCount - 1])
            GparserIfSkip[GparserIfCount - 1] = 0;
        else
            GparserIfSkip[GparserIfCount - 1] = 1;
    }
    else
        ProgramFail(Parser, "#else without #if/#ifdef");
}

void ParseMacroEndDefine(struct ParseState *Parser)
{
    if (GparserIfCount)
    {
        GparserIfCount--;
        GparserIfSkip[GparserIfCount] = 0;
    }
    else
        ProgramFail(Parser, "#endif without #if/#ifdef");
}

/* parse a #undef macro definition and delete it from table */
void ParseMacroUndef(struct ParseState *Parser)
{
  struct Value *MacroName;
  struct Value *MacroValue;

  if (LexGetToken(Parser, &MacroName, TRUE) != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");

  LexToEndOfLine(Parser);

  MacroValue = TableDelete(&GlobalTable, MacroName->Val->Identifier);

  if (MacroValue == NULL)
  {
#if 0
        //do we handle undefined (does not exist) as error or not?
        //make it silent - let us #undef a macro even it does not exist
        ProgramFail(Parser, "'%s' is not defined", MacroName->Val->Identifier);
        /* no need for else, via longjmp it is jumping away? */
#endif
  }
  else
        VariableFree(MacroValue);
}

/* copy where we're at in the parsing */
void ParserCopyPos(struct ParseState *To, struct ParseState *From)
{
    To->Pos = From->Pos;
    To->Line = From->Line;
#ifdef FANCY_ERROR_REPORTING
    To->CharacterPos = From->CharacterPos;
#endif
}

/* parse a "for" statement */
void ParseFor(struct ParseState *Parser)
{
    int Condition;
    struct ParseState PreConditional;
    struct ParseState PreIncrement;
    struct ParseState PreStatement;
    struct ParseState After;

    if (LexGetToken(Parser, NULL, TRUE) != TokenOpenBracket)
        ProgramFail(Parser, "'(' expected");

    /*
     * Remark: we cannot leave statement empty as "for (; ...)", at least:
     * for (i; ...
     * for (i=i; ...
     */
    if (ParseStatement(Parser, TRUE) != ParseResultOk)
        ProgramFail(Parser, "statement expected");

    ParserCopyPos(&PreConditional, Parser);
    Condition = ExpressionParseInt(Parser);

    if (LexGetToken(Parser, NULL, TRUE) != TokenSemicolon)
        ProgramFail(Parser, "';' expected");

    ParserCopyPos(&PreIncrement, Parser);
    ParseStatementMaybeRun(Parser, FALSE, FALSE);

    if (LexGetToken(Parser, NULL, TRUE) != TokenCloseBracket)
        ProgramFail(Parser, "')' expected");

    ParserCopyPos(&PreStatement, Parser);
    if (ParseStatementMaybeRun(Parser, Condition, TRUE) != ParseResultOk)
        ProgramFail(Parser, "statement expected");

    if (Parser->Mode == RunModeContinue)
        Parser->Mode = RunModeRun;

    ParserCopyPos(&After, Parser);

    while (Condition && Parser->Mode == RunModeRun)
    {
        ParserCopyPos(Parser, &PreIncrement);
        ParseStatement(Parser, FALSE);

        ParserCopyPos(Parser, &PreConditional);
        Condition = ExpressionParseInt(Parser);

        if (Condition)
        {
            ParserCopyPos(Parser, &PreStatement);
            ParseStatement(Parser, TRUE);

            if (Parser->Mode == RunModeContinue)
                Parser->Mode = RunModeRun;
        }
    }

    if (Parser->Mode == RunModeBreak)
        Parser->Mode = RunModeRun;

    ParserCopyPos(Parser, &After);
}

/* parse a block of code and return what mode it returned in */
enum RunMode ParseBlock(struct ParseState *Parser, int AbsorbOpenBrace, int Condition)
{
    if (AbsorbOpenBrace && LexGetToken(Parser, NULL, TRUE) != TokenLeftBrace)
        ProgramFail(Parser, "'{' expected");

    if (Parser->Mode == RunModeSkip || !Condition)
    {
        /* condition failed - skip this block instead */
        enum RunMode OldMode = Parser->Mode;
        Parser->Mode = RunModeSkip;
        while (ParseStatement(Parser, TRUE) == ParseResultOk)
        {}
        Parser->Mode = OldMode;
    }
    else
    {
        /* just run it in its current mode */
        while (ParseStatement(Parser, TRUE) == ParseResultOk)
        {}
    }

    if (LexGetToken(Parser, NULL, TRUE) != TokenRightBrace)
        ProgramFail(Parser, "'}' expected");

    return Parser->Mode;
}

/* parse a statement */
enum ParseResult ParseStatement(struct ParseState *Parser, int CheckTrailingSemicolon)
{
    struct Value *CValue;
    struct Value *LexerValue;
    int Condition;
    struct ParseState PreState = *Parser;
    enum LexToken Token = LexGetToken(Parser, NULL, TRUE);

    picoc_CheckForAbort();

    if (GparserIfCount)
        //we are inside an #ifdef block
        if (GparserIfSkip[GparserIfCount - 1])
        {
            //the #ifdef block is inactive - allow just these keywords here
            switch (Token)
            {
                case TokenEOF:
                case TokenIfDefine:
                case TokenIfNDefine:
                case TokenIfDefineVal:
                case TokenElseDefine:
                case TokenEndDefine:
                    break;
                default:
                    //all other we ignore and skip
                    return ParseResultOk;
            }
        }

    switch (Token)
    {
        case TokenEOF:
            return ParseResultEOF;

        case TokenIdentifier:
        case TokenAsterisk:
        case TokenAmpersand:
        case TokenIncrement:
        case TokenDecrement:
            *Parser = PreState;
            ExpressionParse(Parser, &CValue);
            if (Parser->Mode == RunModeRun)
                VariableStackPop(Parser, CValue);
            break;

        case TokenLeftBrace:
            ParseBlock(Parser, FALSE, TRUE);
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenIf:
            if (LexGetToken(Parser, NULL, TRUE) != TokenOpenBracket)
                ProgramFail(Parser, "'(' expected");

            Condition = ExpressionParseInt(Parser);

            if (LexGetToken(Parser, NULL, TRUE) != TokenCloseBracket)
                ProgramFail(Parser, "')' expected");

            if (ParseStatementMaybeRun(Parser, Condition, TRUE) != ParseResultOk)
                ProgramFail(Parser, "statement expected");

            if (LexGetToken(Parser, NULL, FALSE) == TokenElse)
            {
                LexGetToken(Parser, NULL, TRUE);
                if (ParseStatementMaybeRun(Parser, !Condition, TRUE) != ParseResultOk)
                    ProgramFail(Parser, "statement expected");
            }
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenWhile:
            {
                struct ParseState PreConditional;

                if (LexGetToken(Parser, NULL, TRUE) != TokenOpenBracket)
                    ProgramFail(Parser, "'(' expected");

                ParserCopyPos(&PreConditional, Parser);
                do
                {
                    ParserCopyPos(Parser, &PreConditional);
                    Condition = ExpressionParseInt(Parser);
                    if (LexGetToken(Parser, NULL, TRUE) != TokenCloseBracket)
                        ProgramFail(Parser, "')' expected");

                    if (ParseStatementMaybeRun(Parser, Condition, TRUE) != ParseResultOk)
                        ProgramFail(Parser, "statement expected");

                    if (Parser->Mode == RunModeContinue)
                        Parser->Mode = RunModeRun;

                } while (Parser->Mode == RunModeRun && Condition);

                if (Parser->Mode == RunModeBreak)
                    Parser->Mode = RunModeRun;

                CheckTrailingSemicolon = FALSE;
            }
            break;

        case TokenDo:
            {
                struct ParseState PreStatement;
                ParserCopyPos(&PreStatement, Parser);
                do
                {
                    ParserCopyPos(Parser, &PreStatement);
                    if (ParseStatement(Parser, TRUE) != ParseResultOk)
                        ProgramFail(Parser, "statement expected");

                    if (Parser->Mode == RunModeContinue)
                        Parser->Mode = RunModeRun;

                    if (LexGetToken(Parser, NULL, TRUE) != TokenWhile)
                        ProgramFail(Parser, "'while' expected");

                    if (LexGetToken(Parser, NULL, TRUE) != TokenOpenBracket)
                        ProgramFail(Parser, "'(' expected");

                    Condition = ExpressionParseInt(Parser);
                    if (LexGetToken(Parser, NULL, TRUE) != TokenCloseBracket)
                        ProgramFail(Parser, "')' expected");

                } while (Condition && Parser->Mode == RunModeRun);

                if (Parser->Mode == RunModeBreak)
                    Parser->Mode = RunModeRun;
            }
            break;

        case TokenFor:
            ParseFor(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenSemicolon:
            CheckTrailingSemicolon = FALSE; //XXXX
            break;

        case TokenIntType:
        case TokenShortType:
        case TokenCharType:
        case TokenLongType:
        case TokenFloatType:
        case TokenDoubleType:
        case TokenVoidType:
        case TokenStructType:
        case TokenUnionType:
        case TokenEnumType:
        case TokenSignedType:
        case TokenUnsignedType:
        //extend
        case TokenUint32Type:
        case TokenInt32Type:
        case TokenUint16Type:
        case TokenInt16Type:
        case TokenUint8Type:
        case TokenInt8Type:
            *Parser = PreState;
            CheckTrailingSemicolon = ParseDeclaration(Parser, Token);
            break;

        case TokenHashDefine:
            ParseMacroDefinition(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenHashUndef:
            ParseMacroUndef(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

#ifndef NO_HASH_INCLUDE
        case TokenHashInclude:
            if (LexGetToken(Parser, &LexerValue, TRUE) != TokenStringConstant)
                ProgramFail(Parser, "\"filename.h\" expected");

            //we should use #include just in a running script - otherwise #include on
            //command line crashes
            //just added a fatal "internal error" but #include does not
            //work on command line (with #define inside), just use RunScript()
            {
                //free the LexerValue, if recursively called
                char filename[255];
                strcpy(filename, (char *)LexerValue->Val->NativePointer);
                PlatformScanFile(filename);
            }
            CheckTrailingSemicolon = FALSE;
            break;
#endif

        case TokenSwitch:
            if (LexGetToken(Parser, NULL, TRUE) != TokenOpenBracket)
                ProgramFail(Parser, "'(' expected");

            Condition = ExpressionParseInt(Parser);

            if (LexGetToken(Parser, NULL, TRUE) != TokenCloseBracket)
                ProgramFail(Parser, "')' expected");

            if (LexGetToken(Parser, NULL, FALSE) != TokenLeftBrace)
                ProgramFail(Parser, "'{' expected");

            {
                /* new block so we can store parser state */
                enum RunMode OldMode = Parser->Mode;
                int OldSearchLabel = Parser->SearchLabel;
                Parser->Mode = RunModeCaseSearch;
                Parser->SearchLabel = Condition;

                ParseBlock(Parser, TRUE, OldMode != RunModeSkip);

                Parser->Mode = OldMode;
                Parser->SearchLabel = OldSearchLabel;
            }

            CheckTrailingSemicolon = FALSE;
            break;

        case TokenCase:
            if (Parser->Mode == RunModeCaseSearch)
            {
                Parser->Mode = RunModeRun;
                Condition = ExpressionParseInt(Parser);
                Parser->Mode = RunModeCaseSearch;
            }
            else
                Condition = ExpressionParseInt(Parser);

            if (LexGetToken(Parser, NULL, TRUE) != TokenColon)
                ProgramFail(Parser, "':' expected");

            if (Parser->Mode == RunModeCaseSearch && Condition == Parser->SearchLabel)
                Parser->Mode = RunModeRun;

            CheckTrailingSemicolon = FALSE;
            break;

        case TokenDefault:
            if (LexGetToken(Parser, NULL, TRUE) != TokenColon)
                ProgramFail(Parser, "':' expected");

            if (Parser->Mode == RunModeCaseSearch)
                Parser->Mode = RunModeRun;

            CheckTrailingSemicolon = FALSE;
            break;

        case TokenBreak:
            if (Parser->Mode == RunModeRun)
                Parser->Mode = RunModeBreak;
            break;

        case TokenContinue:
            if (Parser->Mode == RunModeRun)
                Parser->Mode = RunModeContinue;
            break;

        case TokenReturn:
            if (Parser->Mode == RunModeRun)
            {
                if (TopStackFrame->ReturnValue->Typ->Base != TypeVoid)
                {
                    if (!ExpressionParse(Parser, &CValue))
                        ProgramFail(Parser, "value required in return");

                    ExpressionAssign(Parser, TopStackFrame->ReturnValue, CValue, TRUE, NULL, 0, FALSE);
                    VariableStackPop(Parser, CValue);
                }
                else
                {
                    if (ExpressionParse(Parser, &CValue))
                        ProgramFail(Parser, "value in return from a void function");
                }

                Parser->Mode = RunModeReturn;
            }
            else
                ExpressionParse(Parser, &CValue);
            break;

        case TokenDelete:
        {
            /* try it as a function or variable name to delete */
            if (LexGetToken(Parser, &LexerValue, TRUE) != TokenIdentifier)
                ProgramFail(Parser, "identifier expected");

            if (Parser->Mode == RunModeRun)
            {
                /* delete this variable or function */
                CValue = TableDelete(&GlobalTable, LexerValue->Val->Identifier);

                //delete silently
                if (CValue == NULL)
                {
                    //delete silently
                    //ProgramFail(Parser, "'%s' is not defined", LexerValue->Val->Identifier);
                }
                else
                    VariableFree(CValue);
            }
            break;
        }

        case TokenIgnore :
            break;

        case TokenIfDefine :
            //ATT: done when script loaded and parsed
            //when code is running, e.g. a function called - no need
            //to parse again, already done
            //IT CRASHES if found in a function during run mode
            ParseMacroIfDefine(Parser);
            CheckTrailingSemicolon = FALSE;

            break;

        case TokenIfNDefine :
            ParseMacroIfNDefine(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenIfDefineVal :
            ParseMacroIfDefineVal(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenElseDefine :
            ParseMacroElseDefine(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

        case TokenEndDefine :
            ParseMacroEndDefine(Parser);
            CheckTrailingSemicolon = FALSE;
            break;

        default:
            *Parser = PreState;
            //we return this if function defined, no semicolon - but OK
            return ParseResultError;
    }

    if (CheckTrailingSemicolon)
    {
        if (LexGetToken(Parser, NULL, TRUE) != TokenSemicolon)
            ProgramFail(Parser, "';' expected");
    }

    return ParseResultOk;
}

/* quick scan a source file for definitions */
void Parse(const char *FileName, const char *Source, int SourceLen, int RunIt)
{
    struct ParseState Parser;
    enum ParseResult Ok;

    void *OldCleanupTokens = CleanupTokens;
    void *Tokens = NULL;

    //save the ExitBuf
    jmp_buf savedExitBuf;
    memcpy(savedExitBuf, ExitBuf, sizeof(jmp_buf));

    if ( ! setjmp(ExitBuf))
    {
        Tokens = LexAnalyse(FileName, Source, SourceLen, NULL);
        if (OldCleanupTokens == NULL)
            CleanupTokens = Tokens;

        LexInitParser(&Parser, Source, Tokens, FileName, RunIt);

        do {
            Ok = ParseStatement(&Parser, TRUE);
        } while (Ok == ParseResultOk);

        if (Tokens)
            HeapFreeMem(Tokens);
        if (OldCleanupTokens == NULL)
            CleanupTokens = NULL;
        Tokens = NULL;
    }
    else
    {
        Ok = ParseResultEOF;    //exit(int) is like end of file
    }

    memcpy(ExitBuf, savedExitBuf, sizeof(jmp_buf));

    if (Tokens)
        HeapFreeMem(Tokens);
    if (OldCleanupTokens == NULL)
        CleanupTokens = NULL;

    if (Ok == ParseResultError)
        PlatformPrintf("parse error");

    if (GparserIfCount)
        PlatformPrintf("#endif missing");

    GparserIfCount = 0;
}

struct ParseState *gTopParser;

#include <signal.h>
#include <stdlib.h>
//#include <stdio.h>

void handler(int s){
    (void)s;
    ProgramFail(gTopParser, "FATAL: not initialized pointer? internal crash?");
    /*
     * remark: we get this error if we assign a Result value when it is not exepcted to need: void function()
     */
}

/* parse interactively */
void ParseInteractive(void)
{
    struct ParseState Parser;
    enum ParseResult Ok;

    LexInitParser(&Parser, NULL, NULL, StrEmpty, TRUE);
    gTopParser = &Parser;

    do
    {
        LexInteractiveClear(&Parser);

        if ( ! setjmp(ExitBuf))
        {
            signal(SIGABRT, handler);
            signal(SIGSEGV, handler);

            do
            {
                GpicocAbort = 0;
                LexInteractiveStatementPrompt();
                Ok = ParseStatement(&Parser, TRUE);
                LexInteractiveCompleted(&Parser);
            } while (Ok == ParseResultOk);

            GparserIfCount = 0;
        }
        else
        {
            if (GpicocAbort)
            {
                PlatformPrintf("PICO-C aborted\n");
            }
            if (picoc_CheckForStopped())
            {
                PlatformPrintf("PICO-C killed\n");
                return;                         //thread stopped
            }
        }
    } while(1);
}
