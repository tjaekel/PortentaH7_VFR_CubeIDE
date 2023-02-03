/*
 * commands.c
 *
 * Created: 11/27/2011 5:38:18 PM
 *  Author: tjaekel
 */ 

/*
 * \file commands.c
 * \brief command extensions for PICOC
 *
 */

/**
 * This is an example how we can use Doxygen comments to enrich
 * the source code with additional information
 */

/**
 * when using printf include printf.h, instead of stdio.h
 * It allows us to substitute printf by a macros for our own
 * print function
 */

#include "picoc.h"       /** PICOC prototypes and type definitions */
#include "printf.h"      /** \warning include printf.h necessary for printf. Don't inlcude stdio.h */

#if 0
/* read a 32bit value from memory, param0 is address as LongInteger */
void LibReadMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    unsigned long *uPtr = (unsigned long *)(Param[0]->Val->UnsignedLongInteger);
    ReturnValue->Val->UnsignedLongInteger = *uPtr;
}

/* write a 32bit value to memory, param0 is address (as LongInteger), param1 is value (32bit LongInteger) */
void LibWriteMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    /* do not read the value, we need a WO function, reading can be a risk if Clear-On-Read */
    unsigned long *uPtr = (unsigned long *)Param[1]->Val->UnsignedLongInteger;
    *uPtr = Param[0]->Val->UnsignedLongInteger;
}

void CmdReadMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    unsigned long *uPtr = (unsigned long *)Param[0]->Val->UnsignedLongInteger;
    Param[0]->Val->UnsignedLongInteger &= 0xFFFFFFFC;
    print_log(UART_OUT, "0x%8.8lx = 0x%8.8lx\r\n", (unsigned long)(Param[0]->Val->UnsignedLongInteger),
        (unsigned long)(*((volatile unsigned long *)Param[0]->Val->UnsignedLongInteger)));
    ReturnValue->Val->UnsignedLongInteger = *uPtr;
}

void CmdWriteMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  Param[0]->Val->UnsignedLongInteger &= 0xFFFFFFFC;
  *((volatile unsigned long *)Param[0]->Val->UnsignedLongInteger) = Param[1]->Val->UnsignedLongInteger;
  /* read back the written value - ATT: risky if it is a WO register !! */
  CmdReadMem(Parser, ReturnValue, Param, NumArgs - 1);
}
#endif

#ifndef SAVE_SPACE
void CmdWriteOMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  Param[0]->Val->UnsignedShortInteger &= 0xFFFE;
  *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger) = Param[1]->Val->UnsignedShortInteger;
}

void CmdFillMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  Param[0]->Val->UnsignedShortInteger &= 0xFFFE;
  Param[1]->Val->UnsignedShortInteger &= 0xFFFE;
  while (Param[0]->Val->UnsignedShortInteger < Param[1]->Val->UnsignedShortInteger)
  {
    *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger) = Param[2]->Val->UnsignedShortInteger;
    Param[0]->Val->UnsignedShortInteger += sizeof(short);
  }
}

void CmdCmpMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  Param[0]->Val->UnsignedShortInteger &= 0xFFFE;
  Param[1]->Val->UnsignedShortInteger &= 0xFFFE;
  while (Param[0]->Val->UnsignedShortInteger < Param[1]->Val->UnsignedShortInteger)
  {
    if (*((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger) != Param[2]->Val->UnsignedShortInteger)
    {
      print_log(UART_OUT, "*E: 0x%4.4x (0x%4.4x) <> 0x%4.4x\r\n",
        Param[0]->Val->UnsignedShortInteger, *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger),
        Param[2]->Val->UnsignedShortInteger);
      ReturnValue->Val->Integer = 0;  //ERROR
      return;
    }
    Param[1]->Val->ShortInteger += sizeof(short);
  }
  ReturnValue->Val->Integer = 1;  //SUCCESS
  return;
}

void CmdCpyMem(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  Param[0]->Val->UnsignedShortInteger &= 0xFFFE;
  Param[1]->Val->UnsignedShortInteger &= 0xFFFE;
  Param[2]->Val->UnsignedShortInteger &= 0xFFFE;
  while (Param[2]->Val->UnsignedShortInteger)
  {
    *((volatile unsigned short *)Param[1]->Val->UnsignedShortInteger) = *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger);
    Param[0]->Val->UnsignedShortInteger += sizeof(short);
    Param[1]->Val->UnsignedShortInteger += sizeof(short);
    Param[2]->Val->UnsignedShortInteger -= sizeof(short);
  }
}
#endif

void CmdMemWordPrint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

  int i;

  i = 0;
  Param[1]->Val->UnsignedLongInteger &= 0xFFFFFFFC;
  while (Param[1]->Val->UnsignedLongInteger)
  {
    if (i == 0)
      print_log(UART_OUT, "0x%8.8lX | ", (unsigned long)Param[0]->Val->NativePointer);
    print_log(UART_OUT, "%8.8lX ", (unsigned long)(*((volatile unsigned long *)Param[0]->Val->NativePointer)));
    i++;
    if (i == 4)
    {
      print_log(UART_OUT, "\r\n");
      i = 0;
    }

    Param[1]->Val->UnsignedLongInteger -= sizeof(long);
    Param[0]->Val->NativePointer = (void *)((unsigned long *)Param[0]->Val->NativePointer + 1);

  }
  if (i)
    print_log(UART_OUT, "\r\n");
}

void CmdMemShortPrint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

  int i;

  i = 0;
  Param[1]->Val->UnsignedLongInteger &= 0xFFFFFFFE;
  while (Param[1]->Val->UnsignedLongInteger)
  {
    if (i == 0)
    	print_log(UART_OUT, "0x%8.8lX | ", (unsigned long)Param[0]->Val->NativePointer);
    print_log(UART_OUT, "%4.4X ", (unsigned short)(*((volatile unsigned short *)Param[0]->Val->NativePointer)));
    i++;
    if (i == 8)
    {
    	print_log(UART_OUT, "\r\n");
      i = 0;
    }

    Param[1]->Val->UnsignedLongInteger -= sizeof(short);
    Param[0]->Val->NativePointer = (void *)((unsigned short *)Param[0]->Val->NativePointer + 1);

  }
  if (i)
	  print_log(UART_OUT, "\r\n");
}

void CmdMemBytePrint(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

  int i;
  volatile unsigned char *p;

  i = 0;
  p = (volatile unsigned char *)Param[0]->Val->NativePointer;  //avoid a warning
  while (Param[1]->Val->UnsignedLongInteger)
  {
    if (i == 0)
    {
      p = (volatile unsigned char *)Param[0]->Val->NativePointer;
      print_log(UART_OUT, "0x%8.8lX | ", (unsigned long)Param[0]->Val->UnsignedLongInteger);
    }
    print_log(UART_OUT, "%2.2X ", (unsigned char)(*((volatile unsigned char *)Param[0]->Val->NativePointer)));
    i++;
    if (i == 16)
    {
      //print ASCII characters
    	print_log(UART_OUT, " | ");
      for (i = 0; i < 16; i++)
      {
        if ((*p < ' ') || (*p >= 0x7f))
        	print_log(UART_OUT, ".");
        else
        	print_log(UART_OUT, "%c", *p);
        p++;
      }
      print_log(UART_OUT, "\r\n");
      i = 0;
    }

    Param[1]->Val->UnsignedLongInteger -= sizeof(char);
    Param[0]->Val->NativePointer = (void *)((unsigned char *)Param[0]->Val->NativePointer + 1);

  }
  if (i)
	  print_log(UART_OUT, "\r\n");
}

#ifndef SAVE_SPACE
void CmdCheckRead(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  unsigned short val;

  Param[0]->Val->UnsignedShortInteger &= 0xFFFE;
  val = *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger);
  if ( ! ((val & Param[2]->Val->UnsignedShortInteger) == (Param[1]->Val->UnsignedShortInteger & Param[2]->Val->UnsignedShortInteger)))
  {
	print_log(UART_OUT, "*E: 0x%4.4x (0x%4.4x) <> 0x%4.4x\r\n",
      Param[0]->Val->UnsignedShortInteger, val, (Param[1]->Val->UnsignedShortInteger & Param[2]->Val->UnsignedShortInteger));
    ReturnValue->Val->Integer = 0;
    return;
  }
  ReturnValue->Val->Integer = 1;  //SUCCESS
  return;
}

void CmdCheckWrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  Param[0]->Val->UnsignedShortInteger &= 0xFFFE;
  *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger) = Param[1]->Val->UnsignedShortInteger;
  CmdCheckRead(Parser, ReturnValue, Param, NumArgs);
}

void CmdPoll(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
  unsigned short val;
  do
  {
    val = *((volatile unsigned short *)Param[0]->Val->UnsignedShortInteger);
  } while ((val & Param[2]->Val->UnsignedShortInteger) == (Param[1]->Val->UnsignedShortInteger & Param[2]->Val->UnsignedShortInteger));
}
#endif

#ifdef WITH_SCRIPTS
void LibOpenScript(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

  char filename[255];
  strcpy(filename, Param[0]->Val->NativePointer);

  //ATT: this deletes all definitions done before!
  Cleanup();
  Initialise();
  /*
   * if we do the lines above we can reload a script with definitions, but there is an error right after
   * the RunScript and next command - no idea why (an invalid character left somewhere?)
   * If we do not clean it works fine, except: a variable definition remains defined, we get an error.
   * We could undefine via #undef var; which works for variables. What about function definitions?
   * But if we use #undef var; and var already exists - also an error.
   */
  PlatformScanFile(filename);
  /*
   * cleanup afterwards - otherwise the parser is confused and sees an invalid identifier
   */
  PlatformExit();
}

extern int ExitValue;

void LibGetExitVal(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)Param;
    (void)NumArgs;

    ReturnValue->Val->Integer = ExitValue;
}
#endif

void LibSetLED(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
	switch (Param[0]->Val->Integer)
	{
	case 0 :
		if (Param[1]->Val->Integer)
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_SET);
		break;
	case 1 :
		if (Param[1]->Val->Integer)
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);
		break;
	case 2 :
		if (Param[1]->Val->Integer)
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_SET);
		break;
	}

	//no return value
}

/* set string for INT handler */
void LibSetINThandler(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
	extern char picoc_INThandlerCMD[MAX_SCRIPT_SIZE];

	strncpy(picoc_INThandlerCMD, Param[0]->Val->NativePointer, sizeof(picoc_INThandlerCMD) - 1);
}

/*
 * UVM extensions
 */
void UVMInfo(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    if (Param[2]->Val->Integer > 1)
        PlatformPrintf("WARNING: %s : %s\n", (char *)Param[0]->Val->NativePointer, (char *)Param[1]->Val->NativePointer);
    else
        if (Param[2]->Val->Integer > 0)
            PlatformPrintf("INFO: %s : %s\n", (char *)Param[0]->Val->NativePointer, (char *)Param[1]->Val->NativePointer);
        else
            PlatformPrintf("..... %s : %s\n", (char *)Param[0]->Val->NativePointer, (char *)Param[1]->Val->NativePointer);
}

void UVMError(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    /* verbosity_level is not used (yet) */
    PlatformPrintf("ERROR: %s : %s\n", (char *)Param[0]->Val->NativePointer, (char *)Param[1]->Val->NativePointer);
}

void LibMsSleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

    picoc_MsSleep(Param[0]->Val->LongInteger);
}

void LibExecuteCommand(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    int retVal;
    (void)Parser;
    (void)ReturnValue;
    (void)NumArgs;

#if 0
    //TODO: we can use also elipse, ... here, for the parameters, to substitute in the string
    //or use sprintf to generate final string
    retVal = picoc_ExecuteCommand(Param[0]->Val->NativePointer);
    ReturnValue->Val->Integer = retVal;
#else
    static char fmtOutput[1024];

    struct OutputStream StrStream;

    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = fmtOutput;

    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &StrStream);
    PrintCh(0, &StrStream);
    retVal = picoc_ExecuteCommand(fmtOutput);
    ReturnValue->Val->Integer = retVal;
#endif
}

void LibGetGPIO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    (void)Param;

    int gpio;
    gpio = picoc_ReadGPIO();

    ReturnValue->Val->UnsignedLongInteger = gpio;
}

void LibPutGPIO(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    (void)ReturnValue;

    picoc_WriteGPIO(Param[0]->Val->UnsignedLongInteger);
}

#if 0
void LibCpuWriteSingle(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    (void)ReturnValue;

    int bytes;
    unsigned long val;

    bytes = Param[2]->Val->Integer;
    val = Param[1]->Val->UnsignedLongInteger;
    if ((bytes > 0) && (bytes < 5))
        picoc_WriteConsecutive(Param[0]->Val->UnsignedLongInteger, &val, bytes, 0);
}

void LibCpuReadSingle(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    int bytes;
    unsigned long val = 0;

    bytes = Param[1]->Val->Integer;
    if ((bytes > 0) && (bytes < 5))
        picoc_ReadConsecutive(Param[0]->Val->UnsignedLongInteger, &val, bytes, 0);

    ReturnValue->Val->UnsignedLongInteger = val;
}
#endif

void LibSpiTrans(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    int retVal;
    retVal = picoc_SpiTransaction(Param[0]->Val->NativePointer,
                                  Param[1]->Val->NativePointer,
                                  Param[2]->Val->Integer
                                  );
    ReturnValue->Val->Integer = retVal;
}

void LibI2CWrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    int bytes;
    unsigned char slaveAddr;
    int flags;
    unsigned char *data;

    slaveAddr = Param[0]->Val->Character;
    data  = Param[1]->Val->NativePointer;
    bytes = Param[2]->Val->Integer;
    flags = Param[3]->Val->Integer;

    bytes = picoc_I2CWrite(slaveAddr, data, bytes, flags);

    ReturnValue->Val->UnsignedLongInteger = bytes;
}

void LibI2CRead(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;

    int bytes;
    unsigned char slaveAddr;
    int flags;
    unsigned char regAddr, *data;

    slaveAddr = Param[0]->Val->Character;
    regAddr = Param[1]->Val->Character;
    data  = Param[2]->Val->NativePointer;
    bytes = Param[3]->Val->Integer;
    flags = Param[4]->Val->Integer;

    bytes = picoc_I2CRead(slaveAddr, regAddr, data, bytes, flags);

    ReturnValue->Val->UnsignedLongInteger = bytes;
}

void LibPicocRestart(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    (void)ReturnValue;
    (void)Param;
    GPicocRestart = 1;
    longjmp(RestartBuf, 1);
}

void LibMethodSize(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    (void)Parser;
    (void)NumArgs;
    (void)Param;

    ReturnValue->Val->UnsignedLongInteger = 4;
}
