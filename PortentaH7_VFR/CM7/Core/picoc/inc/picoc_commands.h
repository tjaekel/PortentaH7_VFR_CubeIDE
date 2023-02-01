/*
 * commands.h
 *
 * Created: 11/27/2011 5:45:19 PM
 *  Author: tjaekel
 */ 

#ifndef PICOC_COMMANDS_H_
#define PICOC_COMMANDS_H_

/*
 * \file commands.h
 * \brief command extensions for PICOC
 *
 */

/* commands as regular C functions, no print messages */
//extern void LibReadMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
//extern void LibWriteMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

/* commands with prints, as interactive shell commands */
//extern void CmdReadMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
//extern void CmdWriteMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdMemWordPrint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdMemShortPrint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdMemBytePrint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

#ifndef SAVE_SPACE
extern void CmdWriteOMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdFillMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdCmpMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdCpyMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdCheckRead(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdCheckWrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void CmdPoll(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
#endif
#ifdef WITH_SCRIPTS
extern void LibOpenScript(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
extern void LibGetExitVal(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
#endif

void LibMsSleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void LibExecuteCommand(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

#if 0
void LibWriteConsec(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void LibReadConsec(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
#endif

#if 0
void LibCpuWriteSingle(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void LibCpuReadSingle(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
#endif
void LibSpiTrans(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

void LibI2CWrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void LibI2CRead(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

void LibPicocRestart(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

void LibMethodSize(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

void LibGetGPIO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void LibPutGPIO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

void LibSetLED(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void LibSetINThandler(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

/*
 * UVM extensions
 */
void UVMInfo(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);
void UVMError(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs);

#endif /* PICOC_COMMANDS_H_ */
