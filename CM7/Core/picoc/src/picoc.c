/*
 * picoc.c
 *
 * Created: 09/29/2012 5:35:41 PM
 *  Author: tjaekel
 */ 

#include "picoc.h"
#include "printf.h"

#define  VERSION_STR  "3.2"

char picoc_INThandlerCMD[MAX_SCRIPT_SIZE] = {'\0'};
static int picoc_IsRunning = 0;

/* initialize everything */
void Initialise(void)
{
    HeapInit();
    TableInit();
    VariableInit();
    LexInit();
    TypeInit();
    LibraryInit(&GlobalTable, (const char*)"c library", &CLibrary);
#ifdef USE_PLATFORM_LIB
    CLibraryInit();				/* optional: defines TRUE, FALSE, NULL */
    PlatformLibraryInit();      /* empty */
    LibraryInit(&GlobalTable, "platform library", &PlatformLibrary);
#endif
}

/* free memory */
void Cleanup(void)
{
    PlatformCleanup();
    ParseCleanup();
    LexCleanup();
    VariableCleanup();
    TypeCleanup();
    TableStrFree();
}

void platform_init(void)
{
}

int GPicocRestart = 0;
jmp_buf RestartBuf;

/* platform-dependent code for running programs is in this file */
int pico_c_main_interactive(int argc, char **argv)
{
    (void)argc;
    (void)argv;

RESTART:
	GPicocRestart = 0;
    picoc_ClearStopped();

    if ( ! setjmp(RestartBuf))
    {
    	picoc_IsRunning = 1;
        platform_init();
        Initialise();
	
//#ifndef SAVE_SPACE
        PlatformPrintf("***** PICO-C command interpreter *****\n");
        PlatformPrintf("      (version: %s, %s)\n", VERSION_STR, __DATE__);
//#else
//    PlatformPrintf("\nPICO-C\n");
//#endif

        ParseInteractive();
    }
    else
    {
        if (GPicocRestart)
        {
            GPicocRestart = 0;
            goto RESTART;
        }
    }

    //actually we never return to here - except with PlatformExit
    Cleanup();
    picoc_IsRunning = 0;
    return 0;
}

/* INT handling */
int picoc_INThandler(void)
{
	if (picoc_IsRunning)
	{
		unsigned char *SourceStr;
		long strLen;

		SourceStr = (unsigned char *)picoc_INThandlerCMD;
		strLen = strlen((char *)SourceStr);
		if (strLen)
		{
			Parse("INT", (char *)SourceStr, strLen, TRUE);
			return 1;
		}
		return 0;
	}
	else
		return 0;
}
