/**
 * cmd_dec.h
 *
 *  Created on: May 22, 2018
 *  Author: Torsten Jaekel
 */

#ifndef CMD_DEC_H_
#define CMD_DEC_H_

#include <string.h>
#include "VCP_UART.h"
#include "MEM_Pool.h"

/**
 * make sure this correlates with CHIP_NUM_FIFO_WORDS
 */
#define CMD_DEC_NUM_VAL		(512 + 1)		/* maximum number of value parameters, 512 values for write */

#define MAX_ADC_MEM_SIZE	16384			/* max. ADC memory size in words - avoid wrap around */

#define USR_CMD_STR_SIZE	256				/* length of USR and INT command string */

/**
 * ATT: we need the maximum length for RBLK on ADC memory which is 4K 16bit words!
 */

#define CMD_DEC_OPT_SIGN	'-'				/* option starts with '-' */
#define CMD_DEC_SEPARATOR	';'				/* different commands in single line */
#define CMD_DEC_COMMENT		'#'				/* comment after command - rest of line ignored */

typedef enum {
	CMD_DEC_OK = 0,							/* all OK */
	CMD_DEC_UNKNOWN,						/* command does not exist */
	CMD_DEC_INVALID,						/* wrong command syntax */
	CMD_DEC_ERROR,							/* error on command execution */
	CMD_DEC_EMPTY,							/* empty command, no keyword, e.g. just ENTER */
	CMD_DEC_OOMEM,							/* out of memory to get buffer from mempool */
	CMD_DEC_INVPARAM,						/* invalid parameter, e.g. length too large */
	CMD_DEC_TIMEOUT							/* time out on command */
} ECMD_DEC_Status;

typedef struct {
	char *cmd;								/* command key word */
	char *opt;								/* if option starting with '-' - the string afterwards */
	char *str;								/* rest of line as string */
	unsigned long cmdLen;					/* character length of CMD keyword */
	unsigned long  offset;					/* end of command string, or next semicolon */
	unsigned long  num;						/* number of specified values */
	unsigned long  ctl;						/* break outer command interpreter loop, 'concur' seen */
	unsigned long val[CMD_DEC_NUM_VAL + 1];	/* index 0 can be an optVal, a bit larger to avoid buffer overrun */
} TCMD_DEC_Results;

typedef ECMD_DEC_Status (*TCmdFunc)(TCMD_DEC_Results *res, EResultOut out);
typedef void (*TCallFunc)(void);

typedef struct {
	const char *cmd;						/* command key word */
	const char *help;						/* help text */
	const TCmdFunc func;					/* the command handler function */
	const int manPage;						/* the man page number for QSPI (index of 4K block) */
	const int sepLine;						/* if 1 - we just print a separator line */
} TCMD_DEC_Command;

int convertURL(char *out, const char *in, int maxLenOut);
void SendPrompt(void);
void SendPromptHTTPD(void);
int CMD_invokeAINT(void);
void hex_dump(uint8_t *ptr, uint16_t len, int mode, EResultOut out);

ECMD_DEC_Status CMD_DEC_execute(char *cmd, EResultOut out);

#endif /* CMD_DEC_H_ */
