/*
 * library_unix.c
 *
 * Created: 11/27/2011 5:40:01 PM
 *  Author: tjaekel
 */ 

#include "picoc.h"

void PlatformLibraryInit(void)
{
}

void Clineno (struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    (void)Param;
    (void)NumArgs;

    ReturnValue->Val->Integer = Parser->Line;
}

void Cerrormsg (struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) {
    PlatformErrorPrefix(Parser);
    LibPrintf(Parser, ReturnValue, Param, NumArgs);
}

#ifdef USE_PLATFORM_LIB
/*
 * this is optional, we can define some extensions, platform dependent functions
 */

/* list of all library functions and their prototypes */
struct const LibraryFunction PlatformLibrary[] =
{
    { Clineno,      "int lineno();" },
    { Cerrormsg,    "void errorprintf(char *,...);" },
    { NULL,         NULL }
};
#endif
