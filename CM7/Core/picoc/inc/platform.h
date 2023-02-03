/*
 * platform.h
 *
 * Created: 11/27/2011 5:46:38 PM
 *  Author: tjaekel
 */ 
#include "SDRAM.h"

#ifndef PLATFORM_H_
#define PLATFORM_H_

#define LARGE_INT_POWER_OF_TEN		10000	/* the largest power of ten which fits in an int on this architecture */
#define ALIGN_TYPE void *                   /* the data type to use for alignment */

#define GLOBAL_TABLE_SIZE           100     /* global variable table */
#define STRING_TABLE_SIZE           100     /* shared string table size */
#define STRING_LITERAL_TABLE_SIZE   100     /* string literal table size */
#define PARAMETER_MAX				 10     /* maximum number of parameters to a function */
#define LINEBUFFER_MAX			   1024     /* maximum number of characters on a line */
#define LOCAL_TABLE_SIZE			 20     /* size of local variable table (can expand) */
#define STRUCT_TABLE_SIZE			 20 	/* size of struct/union member table (can expand) */

#define INTERACTIVE_PROMPT_STATEMENT	": "
////#define INTERACTIVE_PROMPT_LINE			": "

#define PlatformSetExitPoint() setjmp(ExitBuf)

#define HEAP_SIZE (SDRAM_SIZE_BYTES)			/* space for the heap and the stack, minimal >2K */

// # include <stdio.h>				/* fflush, stdout, fgets, stdin, getchar, putchar */
# include "printf.h"
# include <stdlib.h>				/* malloc, free, exit, calloc, realloc */
# include <ctype.h>					/* isalnum, isspace, isalpha, isdigit */
# include <string.h>				/* memcpy, strlen, memset, strcpy, strncmp, strncpy */
# include <assert.h>				/* assert */
//# include <sys/types.h>			/* not really needed */

#ifndef WITHOUT_SYSSTAT_H
// # include <sys/stat.h>			/* needed for file status (file size) */
#endif

//# include <unistd.h>				/* not found, not needed */
# include <stdarg.h>				/* needed for va_start, va_end, va_arg */
# include <setjmp.h>				/* jump_buf, set_jmp, longjmp */
# ifndef NO_FP						/* with or without Floating Point */
/* defines for the optional "fdlibm" maths library */
#  define _IEEE_LIBM
#  include <math.h>
/*#  define PICOC_MATH_LIBRARY*/
/*#  define NEED_MATH_LIBRARY*/
#  undef BIG_ENDIAN
# endif

typedef unsigned long	PTR_TYPE_INT;

extern jmp_buf ExitBuf;
extern jmp_buf RestartBuf;
extern int GPicocRestart;

#define math_abs(x) (((x) < 0) ? (-(x)) : (x))

#ifdef NEED_MATH_LIBRARY
extern double math_sin(double x);
extern double math_cos(double x);
extern double math_tan(double x);
extern double math_asin(double x);
extern double math_acos(double x);
extern double math_atan(double x);
extern double math_sinh(double x);
extern double math_cosh(double x);
extern double math_tanh(double x);
extern double math_asinh(double x);
extern double math_acosh(double x);
extern double math_atanh(double x);
extern double math_exp(double x);
extern double math_fabs(double x);
extern double math_log(double x);
extern double math_log10(double x);
extern double math_pow(double x, double y);
extern double math_sqrt(double x);
extern double math_floor(double x);
extern double math_ceil(double x);
#else /* NEED_MATH_LIBRARY */
#define math_sin(x) sin(x)
#define math_cos(x) cos(x)
#define math_tan(x) tan(x)
#define math_asin(x) asin(x)
#define math_acos(x) acos(x)
#define math_atan(x) atan(x)
#define math_sinh(x) sinh(x)
#define math_cosh(x) cosh(x)
#define math_tanh(x) tanh(x)
#define math_asinh(x) asinh(x)
#define math_acosh(x) acosh(x)
#define math_atanh(x) atanh(x)
#define math_exp(x) exp(x)
#define math_fabs(x) fabs(x)
#define math_log(x) log(x)
#define math_log10(x) log10(x)
#define math_pow(x,y) pow(x,y)
#define math_sqrt(x) sqrt(x)
#define math_floor(x) floor(x)
#define math_ceil(x) ceil(x)
#endif /* NEED_MATH_LIBRARY */

#endif /* PLATFORM_H_ */
