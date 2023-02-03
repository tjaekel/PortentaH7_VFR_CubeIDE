/*
 * platform_unix.c
 *
 * Created: 11/27/2011 5:43:25 PM
 *  Author: tjaekel
 */ 

#include "picoc.h"
#include "VCP_UART.h"
#include "MEM_Pool.h"

#include "cmsis_os.h"

#ifdef WITH_SCRIPTS
#include "ff.h"
#endif

/* read a file into memory */
/* a source file we need to clean up */
static unsigned char *CleanupText = NULL;

/* deallocate any storage */
void PlatformCleanup(void)
{
    /* just in case we have used malloc on PlatformReadFile() */
    if (CleanupText != NULL)
        free(CleanupText);
}

/* get a line of interactive input */
char *PlatformGetLine(char *Buf, int MaxLen)
{
	int l;
    /*
     * ATT: we wait here until something available
     * If we return here with NULL or empty string - we cannot proceed
     * on additional function body definition.
     * Everything had to be on one line, but if blocking read here,
     * we can run endlessly. Therefore put this into a task or wait for
     * characters received.
     */

	do {
			osThreadYield();
			l = UART_getString((unsigned char *)Buf, MaxLen);
	} while (!l);

	if (*Buf == '\r')
		return NULL;		/* empty line */

	return Buf;
}

/* get a character of interactive input */
int PlatformGetCharacter(void)
{
    /*
     * NOT USED - instead we expect input via PlatformGetLine
     */
    return (int)UART_getChar();
}

/* write a character to the console */
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *Stream)
{
    (void)Stream;

     UART_putChar(OutCh);
}

/**
 * @todo RunScript(str) command does not work - DEBUG and fix!
 */

#ifdef WITH_SCRIPTS

unsigned char *PlatformReadFile(const char *FileName)
{
#ifdef RUN_SCRIPT_VIA_UART
	/* read a file via TeraTerm */
	unsigned char *SourceStr;
	int c;
	////SourceStr = malloc(MAX_SCRIPT_SIZE);
	SourceStr = scriptBuf;
	if (SourceStr)
	{
		do
		{
			/* use function without echo */
#if FIX_ME
			do
				c = UART_getChar_nW();
			while (c == 0);
#else
			c = UART_getChar();
#endif
			if (c != 0x1A)
				*SourceStr++ = (unsigned char)c;
		} while (c != 0x1A);	/* until CTRL-Z */
		*SourceStr = '\0';
	}

	return scriptBuf;
#else
	/* use SD Card to read a file with Pico-C statements */
	/**
	 * ATTENTION: we cannot use stack variables or SDRAM:
	 * DMA from SDCard will not work, it times out
	 * So, we allocate here memory - this works!
	 */

	FIL *MyFile;     			/* File object, should not be on stack (DTCM) */
	unsigned char *scriptBuf;	/* buffer for the script - size is limited, access-able for DMA! */
	unsigned int numRd;

	MyFile = MEM_PoolAlloc(sizeof(FIL));
	if ( ! MyFile)
		return 0;
	scriptBuf = MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
	if ( ! scriptBuf)
	{
		MEM_PoolFree(MyFile);
		return 0;
	}

	if(f_open(MyFile, FileName, FA_READ) != FR_OK)
		return 0;

	f_read(MyFile, scriptBuf, MEM_POOL_SEG_SIZE -1, &numRd);
	*(scriptBuf + numRd) = '\0';

	f_close(MyFile);

	MEM_PoolFree(MyFile);
	MEM_PoolFree(scriptBuf);

	if (numRd)
		return scriptBuf;

	return 0;
#endif
}

#endif

/* read and scan a file for definitions */
void PlatformScanFile(const char *FileName)
{
    unsigned char *SourceStr;
    unsigned char *OrigCleanupText;
	long strLen;

    SourceStr = PlatformReadFile(FileName);
    if (SourceStr)
    {
    	OrigCleanupText = CleanupText;
    	if (CleanupText == NULL)
        {
    		CleanupText = SourceStr;
        }

		strLen = strlen((char *)SourceStr);
    	Parse(FileName, (char *)SourceStr, strLen, TRUE);

    	if (OrigCleanupText == NULL)
    		CleanupText = NULL;
    }
}

/* mark where to end the program for platforms which require this */
jmp_buf ExitBuf;

/* exit the program */
void PlatformExit(void)
{
    longjmp(ExitBuf, 1);
}

/* added for Qt integration */
/* collect single characters and process it when NEWLINE seen */
#define OUTPUT_BUFFER_SIZE  1024

static unsigned char sOutputPrintBuffer[OUTPUT_BUFFER_SIZE];
static int sOutputBufferIndex = 0;

void put_character(unsigned char c)
{
    sOutputPrintBuffer[sOutputBufferIndex++] = c;
    sOutputPrintBuffer[sOutputBufferIndex++] = '\0';

    if ((c == '\n') || (sOutputBufferIndex = (OUTPUT_BUFFER_SIZE - 1)))
    {
        UART_printString(sOutputPrintBuffer, UART_OUT);
        sOutputBufferIndex = 0;
    }
}

void put_string(char *s)
{
	UART_printString((unsigned char *)s, UART_OUT);
}
