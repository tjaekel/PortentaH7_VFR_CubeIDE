/**
 * cmd_dec.c
 *
 *  Created on: May 22, 2018
 *  Author: Torsten Jaekel
 */

#include <stdio.h>				/* for NULL */
#include <stdlib.h>
#include "main.h"				/* for FW version */
#include "cmd_dec.h"

#include "I2C_PMIC.h"
#include "SPI.h"
#include "I2C3_user.h"
#include "GPIO_user.h"
#include "SDRAM.h"
#include "SDCard.h"
#include "QSPI.h"
#include "ADC3_temp.h"
#include "CHIP_cmd.h"			/* generic for SPI commands on different CHIPs */
#include "VT100.h"

#include "PDM_MIC.h"
#include "PWM_servo.h"
#include "power_mngt.h"
#include "app_tof.h"

#include "usbd_main.h"

#include "picoc.h"

/* user command string buffer */
//#define USR_CMD_STR_SIZE		80
static char usrCmdBuf[USR_CMD_STR_SIZE]  = {'\0'};
static char usrCmdBuf2[USR_CMD_STR_SIZE] = {'\0'};
static char intCmdBuf[USR_CMD_STR_SIZE]  = {'\0'};

static unsigned long usrVar = 0l;

#define HLP_STR_OPT		"[-P|-A|-T|-D]"

/* prototypes */
ECMD_DEC_Status CMD_help(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_man(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_print(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_fwreset(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_diag(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_debug(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_usr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_usr2(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_var(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_led(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_ipaddr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_syscfg(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_setcfg(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_syserr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_dumpm(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_bootld(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_pmicr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_pmicw(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_epmiccfg(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_rawspi(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_spiclk(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_spitr(TCMD_DEC_Results* res, EResultOut out);

#if 0
ECMD_DEC_Status CMD_cid(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_rreg(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_wreg(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_rblk(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_wblk(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_rslice(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_wslice(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_fslice(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_noop(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_peek(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_poke(TCMD_DEC_Results* res, EResultOut out);
#endif

ECMD_DEC_Status CMD_res(TCMD_DEC_Results* res, EResultOut out);

ECMD_DEC_Status CMD_i2crr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_i2cwr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_i2c2rr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_i2c2wr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_i2cclk(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_tofc(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_tofp(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_cgpio(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_pgpio(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_ggpio(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_bggpio(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_bpgpio(TCMD_DEC_Results* res, EResultOut out);

ECMD_DEC_Status CMD_cint(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_aint(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_test(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_test2(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_syshd(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_sdinit(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sddel(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sdwrite(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sdexec(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sd2qspi(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_picoc(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_msdelay(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_qspi(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_volt(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_udpip(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_usb(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_mic(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_mics(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_cservo(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sservo(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_pwr(TCMD_DEC_Results *res, EResultOut out);

/* last man index: 47 - 60 - 67 */

#ifdef UART_TEST
ECMD_DEC_Status CMD_uspeedtest(TCMD_DEC_Results *res, EResultOut out);
#endif

const TCMD_DEC_Command Commands[] = {
		{
				.help = (const char*)"GENERAL:",
				.sepLine = 1,
		},
		{
				.cmd = (const char *)"help",
				.help = (const char *) "list all defined commands or help for specific [cmd]",
				.func = CMD_help,
				.manPage = 0,
		},
		{
				.cmd = (const char *)"man",
				.help = (const char *)"manual page for <cmd>",
				.func = CMD_man,
				.manPage = 1,
		},
		{
				.cmd = (const char *)"print",
				.help = (const char *)"print [-n|-u] [restOfLine]",
				.func = CMD_print,
				.manPage = 2,
		},
		{
				.cmd = (const char *)"fwreset",
				.help = (const char *)"reboot the MCU firmware",
				.func = CMD_fwreset,
				.manPage = 3,
		},
		{
				.cmd = (const char *)"diag",
				.help = (const char *)"show system diagnosis: FW version, CPU ID, board, CPU clock",
				.func = CMD_diag,
				.manPage = 4,
		},
		{
				.cmd = (const char *)"ipaddr",
				.help = (const char *)"get MCU IP address",
				.func = CMD_ipaddr,
				.manPage = 5,
		},
		{
				.cmd = (const char *)"syscfg",
				.help = (const char *)"show the syscfg settings",
				.func = CMD_syscfg,
				.manPage = 6,
		},
		{
				.cmd = (const char *)"setcfg",
				.help = (const char *)"set config <idx> [val]",
				.func = CMD_setcfg,
				.manPage = 7,
		},
		{
				.cmd = (const char *)"syserr",
				.help = (const char *)"show syserr [-d]",
				.func = CMD_syserr,
				.manPage = 8,
		},
		{
				.cmd = (const char *)"debug",
				.help = (const char *)"set or clear debug flags [value]",
				.func = CMD_debug,
				.manPage = 9,
		},
		{
				.cmd = (const char *)"dumpm",
				.help = (const char *)"display MCU memory <addr> <bytes> [mode]",
				.func = CMD_dumpm,
				.manPage = 9,			/* update! */
		},
		{
				.cmd = (const char *)"bootld",
				.help = (const char *)"activate bootloader",
				.func = CMD_bootld,
				.manPage = 9,			/* update! */
		},
		{
				.cmd = (const char *)"usr",
				.help = (const char *)"define [-p|-d [cmd [cmd; ...]]] print, define or invoke usr command",
				.func = CMD_usr,
				.manPage = 10,
		},
		{
				.cmd = (const char *)"usr2",
				.help = (const char *)"define [-p|-d [cmd [cmd; ...]]] print, define or invoke usr2 command",
				.func = CMD_usr2,
				.manPage = 11,
		},
		{
				.cmd = (const char *)"$",
				.help = (const char *)"$ use variable or set $ [-d <val>]",
				.func = CMD_var,
				.manPage = 12,
		},
		{
				.cmd = (const char *)"led",
				.help = (const char *)"control [led] [On|Off]",
				.func = CMD_led,
				.manPage = 13,
		},
		{
				.cmd = (const char *)"rawspi",
				.help = (const char *)"SPI transaction <byte ...> - byte based",
				.func = CMD_rawspi,
				.manPage = 14,
		},
		{
				.cmd = (const char*)"spitr",
				.help = (const char*)"spitr <word ...> - config word based",
				.func = CMD_spitr,
				.manPage = 15,
		},
		{
				.cmd = (const char *)"spiclk",
				.help = (const char *)"get or set SPI clock [0|khz]",
				.func = CMD_spiclk,
				.manPage = 16,
		},
#if 0
		{
				.help = (const char*)"CHIP SPECIFIC:",
				.sepLine = 1,
		},
		{
				.cmd = (const char*)"cid",
				.help = (const char*)"read the ChipID register",
				.func = CMD_cid,
				.manPage = 17,
		},
		{
				.cmd = (const char*)"rreg",
				.help = (const char*)"read registers <addr> [num]",
				.func = CMD_rreg,
				.manPage = 18,
		},
		{
				.cmd = (const char*)"wreg",
				.help = (const char*)"write registers <addr> [word ...]",
				.func = CMD_wreg,
				.manPage = 19,
		},
		{
				.cmd = (const char*)"rblk",
				.help = (const char*)"read RAM block [-b|-s] <addr> [num]",
				.func = CMD_rblk,
				.manPage = 20,
		},
		{
				.cmd = (const char*)"wblk",
				.help = (const char*)"write RAM block [-b|-s] <addr> [val ...]",
				.func = CMD_wblk,
				.manPage = 21,
		},
		{
				.cmd = (const char*)"rslice",
				.help = (const char*)"read slice RAM [-b|-s] <addr> [numSlices]",
				.func = CMD_rslice,
				.manPage = 45,
		},
		{
				.cmd = (const char*)"wslice",
				.help = (const char*)"write slice RAM [-b|-s] <addr> [sliceVal ...]",
				.func = CMD_wslice,
				.manPage = 46,
		},
		{
				.cmd = (const char*)"fslice",
				.help = (const char*)"fill slice RAM [-b|-s] <addr> [sliceVal [numSlices]]",
				.func = CMD_fslice,
				.manPage = 47,
		},
		{
				.cmd = (const char*)"noop",
				.help = (const char*)"send noop commands [code] [num]",
				.func = CMD_noop,
				.manPage = 22,
		},
		{
				.cmd = (const char*)"peek",
				.help = (const char*)"read registers <addr> [num]",
				.func = CMD_rreg,			/* alias - the same */
				.manPage = 23,
		},
		{
				.cmd = (const char*)"poke",
				.help = (const char*)"write registers <addr> [word ...]",
				.func = CMD_wreg,			/* alias - the same */
				.manPage = 24,
		},
#endif
		{
				.help = (const char*)"I2C IMU:",
				.sepLine = 25,
		},
		{
				.cmd = (const char *)"i2crr",
				.help = (const char *)"I2C register read <addr> [num]",
				.func = CMD_i2crr,
				.manPage = 26,
		},
		{
				.cmd = (const char *)"i2cwr",
				.help = (const char *)"I2C write register <addr> [val ...]",
				.func = CMD_i2cwr,
				.manPage = 27,
		},
		{
				.cmd = (const char *)"i2c2rr",
				.help = (const char *)"I2C addr2 register read <addr> [num]",
				.func = CMD_i2c2rr,
				.manPage = 26,
		},
		{
				.cmd = (const char *)"i2c2wr",
				.help = (const char *)"I2C addr2 write register <addr> [val ...]",
				.func = CMD_i2c2wr,
				.manPage = 27,
		},
		{
				.cmd = (const char *)"i2cclk",
				.help = (const char *)"I2C set clock [0|1|2]",
				.func = CMD_i2cclk,
				.manPage = 28,
		},
		{
				.help = (const char*)"GPIO:",
				.sepLine = 1,
		},
		{
				.cmd = (const char *)"res",
				.help = (const char *)"set reset GPIO [0|1]",
				.func = CMD_res,
				.manPage = 29,
		},
		{
				.cmd = (const char *)"cgpio",
				.help = (const char *)"configure GPIO <IOmask> <ODmask>",
				.func = CMD_cgpio,
				.manPage = 30,
		},
		{
				.cmd = (const char *)"pgpio",
				.help = (const char *)"put [bitvals] [bitmask] to output GPIOs",
				.func = CMD_pgpio,
				.manPage = 31,
		},
		{
				.cmd = (const char *)"ggpio",
				.help = (const char *)"get GPIO input from all pins",
				.func = CMD_ggpio,
				.manPage = 32,
		},
		{
				.cmd = (const char*)"bggpio",
				.help = (const char*)"get single bit GPIO [bitNo]",
				.func = CMD_bggpio,
				.manPage = 33,
		},
		{
				.cmd = (const char*)"bpgpio",
				.help = (const char*)"put/set single bit GPIO [bitno] [val]",
				.func = CMD_bpgpio,
				.manPage = 34,
		},
		{
				.cmd = (const char *)"cint",
				.help = (const char *)"configure GPIO interrupt edge <0|1> disable/enable [0|1]",
				.func = CMD_cint,
				.manPage = 35,
		},
		{
				.cmd = (const char *)"aint",
				.help = (const char *)"define [-p|-d [cmd [cmd; ...]]] print, delete interrupt command",
				.func = CMD_aint,
				.manPage = 36,
		},
		{
				.help = (const char*)"SD CARD:",
				.sepLine = 1,
		},
		{
				.cmd = (const char *)"sdinit",
				.help = (const char *)"initialize or format SD Card [-f|-s] [0|1]",
				.func = CMD_sdinit,
				.manPage = 37,
		},
		{
				.cmd = (const char *)"sddir",
				.help = (const char *)"list [subdir] SD card directory",
				.func = CMD_sddir,
				.manPage = 38,
		},
		{
				.cmd = (const char *)"sdprint",
				.help = (const char *)"[-h|-H] show text [or binary] <filename> from SD card",
				.func = CMD_sdprint,
				.manPage = 39,
		},
		{
				.cmd = (const char *)"sddel",
				.help = (const char *)"delete <filename> from SD card",
				.func = CMD_sddel,
				.manPage = 40,
		},
		{
				.cmd = (const char *)"sdwrite",
				.help = (const char *)"write [-m] <filename> [restOfLine] to SD card, [-m] multi-lines (CTRL-Z)",
				.func = CMD_sdwrite,
				.manPage = 41,
		},
		{
				.cmd = (const char *)"sdexec",
				.help = (const char *)"execute a command text <filename> from SD card, [-s] silent",
				.func = CMD_sdexec,
				.manPage = 42,
		},
		{
				.help = (const char*)"AUXILIARY:",
				.sepLine = 1,
		},
		{
				.cmd = (const char *)"picoc",
				.help = (const char *)"start Pico-C interpreter",
				.func = CMD_picoc,
				.manPage = 43,
		},
		{
				.cmd = (const char *)"msdelay",
				.help = (const char *)"delay [millisec], wait for command line processing",
				.func = CMD_msdelay,
				.manPage = 44,
		},
		{
				.help = (const char*)"EXPERT:",
				.sepLine = 1,
		},
		{
				.cmd = (const char *)"pmicr",
				.help = (const char *)"read PMIC register <addr>",
				.func = CMD_pmicr,
				.manPage = 60,
		},
		{
				.cmd = (const char *)"pmicw",
				.help = (const char *)"write PMIC register <addr> <val>",
				.func = CMD_pmicw,
				.manPage = 61,
		},
		{
				.cmd = (const char *)"epmiccfg",
				.help = (const char *)"recover external PMIC",
				.func = CMD_epmiccfg,
				.manPage = 62,
		},
		{
				.cmd = (const char *)"test",
				.help = (const char *)"test command [par]",
				.func = CMD_test,
				.manPage = 63,
		},
		{
				.cmd = (const char *)"test2",
				.help = (const char *)"test2 command [par]",
				.func = CMD_test2,
				.manPage = 63,
		},
		{
				.cmd = (const char *)"udpip",
				.help = (const char *)"set <IPaddr> for enable or close [0] UDP destination and transmission",
				.func = CMD_udpip,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"syshd",
				.help = (const char *)"MCU system hex dump <addr> <bytes> [mode]",
				.func = CMD_syshd,
				.manPage = 64,
		},
		{
				.cmd = (const char *)"qspi",
				.help = (const char *)"QSPI test [par]",
				.func = CMD_qspi,
				.manPage = 65,
		},
		{
				.cmd = (const char *)"sd2qspi",
				.help = (const char *)"write file from SD card to QSPI <filename> <idx> : max. 4KB",
				.func = CMD_sd2qspi,
				.manPage = 66,
		},
		{
				.cmd = (const char *)"volt",
				.help = (const char *)"measure and display voltage on INP0, INP1",
				.func = CMD_volt,
				.manPage = 68,
		},
		{
				.cmd = (const char *)"usb",
				.help = (const char *)"enable <1..> or disable [0] USB device",
				.func = CMD_usb,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"mic",
				.help = (const char *)"enable <1..52> [freq] [sine] as db> [0|1] or disable [0] microphone",
				.func = CMD_mic,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"mics",
				.help = (const char *)"capture PDM signal",
				.func = CMD_mics,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"cservo",
				.help = (const char *)"configure PWM servo, set 90 neutral",
				.func = CMD_cservo,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"sservo",
				.help = (const char *)"set <num> PWM servo to <degree>",
				.func = CMD_sservo,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"pwr",
				.help = (const char *)"set <num> for power mode",
				.func = CMD_pwr,
				.manPage = 63,		//FIX it
		},
		{
				.help = (const char*)"SENSORS:",
				.sepLine = 1,
		},
		{
				.cmd = (const char *)"tofc",
				.help = (const char *)"initialize and start TOF sensor, [-i] endless on UART",
				.func = CMD_tofc,
				.manPage = 63,		//FIX it
		},
		{
				.cmd = (const char *)"tofp",
				.help = (const char *)"toggle size and ambient [0|1] [0|1]",
				.func = CMD_tofp,
				.manPage = 63,		//FIX it
		},
#ifdef UART_TEST
		{
				.cmd = (const char *)"uspeedtest",
				.help = (const char *)"UART speed test",
				.func = CMD_uspeedtest,
				.manPage = 67,
		},
#endif
};

static unsigned int CMD_DEC_findCmd(const char *cmd, unsigned int len)
{
	size_t i;
	for (i = 0; i < (sizeof(Commands)/sizeof(TCMD_DEC_Command)); i++)
	{
		if (Commands[i].func == NULL)
			continue;					//also if sepLine is set
		/* if length does not match - keep going */
		if (len != (unsigned int)strlen(Commands[i].cmd))
			continue;
		if (strncmp(cmd, Commands[i].cmd, (size_t)len) == 0)
			return (unsigned int)i;
	}

	return (unsigned int)-1;
}

static void CMD_DEC_DecodeError(ECMD_DEC_Status err, EResultOut out)
{
	/*
	 * TODO: decode command errors - empty for now, silently ignored on errors
	 */
	(void)err;
	(void)out;

	return;
}

static unsigned int __attribute__((section(".itcmram"))) CMD_DEC_decode(char *cmdStr, TCMD_DEC_Results *res)
{
	unsigned int i;
	int state;
	char *xCmdStr = cmdStr;

	/* set default results */
	res->cmd = NULL;
	res->cmdLen = 0;
	res->offset = 0;
	res->opt = NULL;
	res->str = NULL;
	res->ctl = 0;
	res->num = 0;
	/* set all values to 0 as default, if omitted but used */
	for (i = 0; i < CMD_DEC_NUM_VAL; i++)
		res->val[i] = 0L;

	state = 0;
	i = 0;
	while ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
	{
		/* skip leading spaces, tabs */
		while ((*cmdStr == ' ') || (*cmdStr == '\t'))
		{
			if ((state == 1) || (state == 3))
			{
				////*cmdStr = '\0';
				state++;
			}
			if (state == 5)
				state--;

			cmdStr++;
		}

		switch (state)
		{
		case 0:	/* find command keyword	*/
			res->cmd = cmdStr;
			////res->cmdLen = 0;
			state = 1;
			break;
		case 1:	/* find end of keyword */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
			{
				cmdStr++;
				res->cmdLen++;
			}
			break;
		case 2:	/* check if option is there */
			if (*cmdStr == CMD_DEC_OPT_SIGN)
			{
				res->opt = cmdStr;
				state = 3;
			}
			else
				state = 4;
			break;
		case 3:	/* find end of option string */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
				cmdStr++;
			break;
		case 4: /* now we scan just values or option value */
			if (i < CMD_DEC_NUM_VAL)
			{
				if (i == 0)
					if ( ! res->str)
						res->str = cmdStr;
				/* ignore incorrect values, e.g. strings, missing 0xAA etc. */
				/*
				 * ATTENTION: this sscanf just signed values (decimal or hex), values above
				 * 0x8000000 results in a wrong value!
				 */
				////if (sscanf(cmdStr, "%li", (long int *)(&res->val[i])) == 1)
				////	i++;

				////fixed, and add '$' for user variable
				if (*cmdStr == '$')
				{
					res->val[i] = usrVar;
					i++;
				}
				else
				{
					if ((cmdStr[1] == 'x') || (cmdStr[1] == 'X'))
					{
						if (sscanf(cmdStr, "%lx", (unsigned long *)(&res->val[i])) == 1)
							i++;
					}
					else
					{
						if (sscanf(cmdStr, "%lu", (unsigned long *)(&res->val[i])) == 1)
							i++;
					}
				}

				res->num = i;
			}
			state = 5;
			break;
		case 5:	/* skip value characters */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
				cmdStr++;
			break;
		}
	} /* end while */

	if (*cmdStr == CMD_DEC_SEPARATOR)
	{
		////*cmdStr++ = '\0';
		cmdStr++;
		res->offset = (unsigned long)(cmdStr - xCmdStr);
		return (unsigned int)(cmdStr - xCmdStr);
	}

	if (*cmdStr == CMD_DEC_COMMENT)
		*cmdStr = '\0';

	if (res->cmd)
		if (*res->cmd == '\0')
			res->cmd = NULL;

	if ((*cmdStr == '\r') || (*cmdStr == '\n'))
		*cmdStr = '\0';

	return 0;
}

ECMD_DEC_Status __attribute__((section(".itcmram"))) CMD_DEC_execute(char *cmd, EResultOut out)
{
	TCMD_DEC_Results *cmdRes;
	unsigned int res, offset, idx;

	cmdRes = MEM_PoolAlloc(sizeof(TCMD_DEC_Results));		//it has to be 32bit aligned!
	if ( ! cmdRes)
	{
		return CMD_DEC_OOMEM;
	}

	offset = 0;
	do
	{
		res = CMD_DEC_decode(cmd + offset, cmdRes);
		offset += res;

		if (cmdRes->cmd)
			if (*cmdRes->cmd != '\0')	/* ignore empty line, just spaces */
			{
				idx = CMD_DEC_findCmd(cmdRes->cmd, cmdRes->cmdLen);
				if (idx != (unsigned int)-1)
				{
					ECMD_DEC_Status err;

					err = Commands[idx].func(cmdRes, out);

					/* decode the err code */
					CMD_DEC_DecodeError(err, out);
				}
				else
				{
					UART_Send((uint8_t *)"*E: unknown >", 13, out);
					UART_Send((uint8_t *)cmdRes->cmd, cmdRes->cmdLen, out);
					UART_Send((uint8_t *)"<\r\n", 3, out);

					SYS_SetError(SYS_ERR_INVALID_CMD);
				}
			}

		if (cmdRes->ctl == 1)
			break;

	} while (res);

	MEM_PoolFree(cmdRes);
	return CMD_DEC_OK;
}

void __attribute__((section(".itcmram"))) SendPrompt(void)
{
	/* send prompt with End of Text EOT */
	UART_Send((uint8_t *)"> \003", 3, UART_OUT);
}

void __attribute__((section(".itcmram"))) SendPromptHTTPD(void)
{
	/* send prompt with End of Text EOT */
	UART_Send((uint8_t *)"\003", 1, HTTPD_OUT);
}

/* ---------------------------------------------------------------------- */

/* helper function for help - keyword length */
static unsigned int CMD_keywordLen(const char *str)
{
	unsigned int l = 0;

	while (*str)
	{
		if ((*str == ';') || (*str == ' ') || (*str == '\r') || (*str == '\n'))
			break;
		str++;
		l++;
	}

	return l;
}

/* helper function for print - string length up to ';' or '#' */
static unsigned int CMD_lineLen(const char *str)
{
	unsigned int l = 0;

	while (*str)
	{
		if ((*str == ';') || (*str == '#') || (*str == '\r') || (*str == '\n'))
			break;
		str++;
		l++;
	}

	return l;
}

#if 0
/* helper function to find string <filename> after a value <IDX> */
static const char * CMD_nextStr(const char *str)
{
	const char *resStr = NULL;		/* set to invalid result */

	/* find space after <IDX> */
	while (*str)
	{
		if ((*str == ' ') || (*str == '\r') || (*str == '\n'))
			break;
		str++;
	}

	/* find start of <filename> as not a space */
	while (*str)
	{
		if (*str != ' ')
		{
			resStr = str;
			break;
		}
		str++;
	}

	return resStr;
}
#endif

/* other helper functions */
static void hex_dumpASCII(EResultOut out, uint8_t *ptr)
{
	int i;
	print_log(out, (const char *)" | ");
	for (i = 0; i < 16; i++)
	{
		if ((*ptr < 0x20) || (*ptr > 0x7E))
			print_log(out, (const char *)".");
		else
			print_log(out, (const char *)"%c", *ptr);

		ptr++;
	}
}

void hex_dump(uint8_t *ptr, uint16_t len, int mode, EResultOut out)
{
	int i = 0;
	int xLen = (int)len;

#if 0
	if (ptr == NULL)
		return;
#endif
	if (len == 0)
		return;

	SCB_CleanDCache_by_Addr((uint32_t*)ptr, ((len + 31)/32) * 32);

	{
		while (xLen > 0)
		{
			if (mode == 3)
			{
				if ((i % 15) == 0)
					print_log(out, "%04x | ", (i*2) / mode);
			}
			else
			{
				if ((i % 16) == 0)
					print_log(out, "%04x | ", i / mode);
			}

			if ((mode == 1) || (mode == 0))
			{
				//bytes
				print_log(out, (const char *)"%02X ", (int)*ptr);
				ptr++;
				xLen--;
				i++;
			}
			else
			if (mode == 2)
			{
				//short words, little endian
				print_log(out, (const char *)"%04X ", (int)(*ptr | (*(ptr + 1) << 8)));
				ptr += 2;
				xLen -= 2;
				i += 2;
			}
			else
			if (mode == 3)
			{
				//12bit words, little endian - tricky: 2 values from 3 bytes
				print_log(out, (const char *)"%03X ", (int)(*ptr | ((*(ptr + 1) & 0xF) << 8)));
				print_log(out, (const char *)"%03X ", (int)(((*(ptr + 1) >> 4) & 0xF) | (*(ptr + 2) << 4)));
				ptr += 3;
				xLen -= 3;
				i += 3;
			}
			else
			if (mode == 4)
			{
				//32bit words, little endian
				print_log(out, (const char *)"%08lX ", (unsigned long)(*ptr |
																	  (*(ptr + 1) << 8) |
																	  (*(ptr + 2) << 16) |
																	  (*(ptr + 3) << 24)
																	 ));
				ptr += 4;
				xLen -= 4;
				i += 4;
			}
			else
			if (mode == 12)
			{
				i = 1;
				////print_log(out, (const char *)"len: %d\r\n", xLen);
				//CLATCH FIFO display, 12 bytes (6 words) - with endian flip and just valid entries
				if ((*(ptr + 11) & 0x80) == 0x80)
				{
					print_log(out, (const char *)"0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						*(ptr + 11), *(ptr + 10), *(ptr + 9), *(ptr + 8), *(ptr + 7), *(ptr + 6),
						*(ptr + 5), *(ptr + 4), *(ptr + 3), *(ptr + 2), *(ptr + 1), *(ptr + 0));
					i = 16;	//force a newline
				}
				ptr += 12;
				xLen -= 12;
				if (xLen < 12)
					xLen = 0;			//break the loop
			}
			else
				break;		//just for now to make it safe

			if (mode == 3)
			{
				if ((i % 15) == 0)
				{
					//we cannot print 16 characters here - different length!
					UART_Send((uint8_t *)"\r\n", 2, out);
				}
			}
			else
			{
				if ((i % 16) == 0)
				{
					hex_dumpASCII(out, ptr - 16);
					UART_Send((uint8_t *)"\r\n", 2, out);
				}
			}
		}

		if (mode == 3)
		{
			if ((i % 15) != 0)
				UART_Send((uint8_t *)"\r\n", 2, out);
		}
		else
		{
			if ((i % 16) != 0)
				UART_Send((uint8_t *)"\r\n", 2, out);
		}
	}
}

/*
 * convert URL into ASCII string for command interpreter:
 * SPACE -> + or %20
 * ; -> %3B
 * + -> %2B
 * # -> %23
 * " -> %22
 * LF -> %0A
 */
int convertURL(char *out, const char *in, int maxLenOut)
{
	int state = 0;
	int i = 0;
	char hexVal[3];

	while ((i < maxLenOut) && (*in))
	{
		if (state == 0)
		{
			if (*in == '+')
			{
				*out++ = ' ';
				in++;
				i++;
				continue;
			}
			if (*in == '%')
			{
				in++;
				state = 1;
				continue;
			}

			/* all other characters take it as it is */
			*out++ = *in++;
			i++;
			continue;
		}

		if (state == 1)
		{
			hexVal[0] = *in++;
			state = 2;
			continue;
		}

		if (state == 2)
		{
			hexVal[1] = *in++;
			hexVal[2] = '\0';
			/* convert hex value (in ASCII, w/o 0x) into ASCII character value */
			*out++ = (char)strtol(hexVal, NULL, 16);
			i++;
			state = 0;
		}
	}

	/* NUL for end of string in out */
	*out = '\0';

	/* return the length of out string */
	return i;
}

/* verify the option and get the SPI interface number - starting at 1 */
int CMD_getSPIoption(char *str)
{
#ifndef CHIP2
	if (strncmp(str, "-T", 2) == 0)
		return 1;
#else
	if (strncmp(str, "-G", 2) == 0)
		return 4;
	if (strncmp(str, "-g", 2) == 0)
		return 5;
#endif
	if (strncmp(str, "-A", 2) == 0)
		return 3;
	//dummy SPI
	if (strncmp(str, "-D", 2) == 0)
		return 6;

	return 2;		//-P is default
}

/* ------------ command handlers ----------------------------*/

ECMD_DEC_Status CMD_help(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int idx;

	if (res->str)
	{
		/* we have 'help <CMD>' - print just the single line help */
		idx = CMD_DEC_findCmd(res->str, CMD_keywordLen(res->str));
		if (idx != (unsigned int)-1)
			print_log(out, (const char *)"%-10s: %s\r\n", Commands[idx].cmd, Commands[idx].help);
		else
		{
			UART_Send((uint8_t *)"*E: unknown\r\n", 13, out);
			return CMD_DEC_UNKNOWN;
		}
	}
	else
	{
		/* just 'help' - print entire list */
		for (idx = 0; (size_t)idx < (sizeof(Commands)/sizeof(TCMD_DEC_Command)); idx++)
		{
			if (Commands[idx].sepLine)
			{
				print_log(out, (const char *)"=========== " VT_BOLD "%s" VT_END " ==================================\r\n", Commands[idx].help);
			}
			else
			{
				print_log(out, (const char *) VT_BLUEBOLD "%-10s" VT_END ": %s\r\n", Commands[idx].cmd, Commands[idx].help);
			}
		}
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_man(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int idx;

	if (res->str)
	{
		/* we have 'man <CMD>' - print the manual page from QSPI */
		idx = CMD_DEC_findCmd(res->str, CMD_keywordLen(res->str));
		if (idx != (unsigned int)-1)
		{
#ifdef VT100
			print_log(out, VT_BG_WHITE);
#endif
			QSPI_ManPage(out, Commands[idx].manPage);
#ifdef VT100
			print_log(out, VT_END);
#endif
		}
		else
		{
			UART_Send((uint8_t *)"*E: unknown\r\n", 13, out);
			return CMD_DEC_UNKNOWN;
		}
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_print(TCMD_DEC_Results *res, EResultOut out)
{
	if (strncmp(res->opt, "-u", 2) == 0)
	{
		int l;
		char *resStr = MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
		if (resStr)
		{
			l = convertURL(resStr, res->str, MEM_POOL_SEG_SIZE);
			UART_Send((uint8_t *)resStr, l, out);
			MEM_PoolFree(resStr);

			if (out == UART_OUT)
				UART_Send((uint8_t *)"\r\n", 2, UART_OUT);
		}
		else
			return CMD_DEC_OOMEM;

		return CMD_DEC_OK;
	}

	if (res->str)
		UART_Send((uint8_t *)res->str, CMD_lineLen(res->str), out);
	if (res->opt)
	{
		if (strncmp(res->opt, "-n", 2) == 0)
		{
			/* no newline to add */
		}
		else
			if (out == UART_OUT)
				UART_Send((uint8_t *)"\r\n", 2, UART_OUT);
	}
	else
		if (out == UART_OUT)
			UART_Send((uint8_t *)"\r\n", 2, UART_OUT);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_fwreset(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;
	(void)out;

#if 0
	/* send the response/command prompt first */
	if (out == UART_OUT)
		SendPrompt();
#endif

	/* block all interrupts and UART reception */

	//just to make sure - reset the UART peripheral
	////__HAL_RCC_USART3_FORCE_RESET();

	__disable_irq();
	NVIC_SystemReset();

	/* we should not come back to here */
	__enable_irq();
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_diag(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;
	extern unsigned long g_pfnVectors;
	unsigned long devID[3];

	devID[0] = *((unsigned long *)0x1FF1E800);
	devID[1] = *((unsigned long *)0x1FF1E804);
	devID[2] = *((unsigned long *)0x1FF1E808);

#if 1
	print_log(out, (const char *)"Project     : %s\r\n", PROJECT_NAME);
	print_log(out, (const char *)"FW version  : %s\r\n", STR_FW_VERSION);
	print_log(out, (const char *)"CHIP        : %s\r\n", STR_CHIP);
#endif
	print_log(out, (const char *)"CPU ID      : 0x%08lx\r\n", (long)SCB->CPUID);
	print_log(out, (const char *)"REV ID      : 0x%08lx\r\n", *((long *)DBGMCU_BASE));
	print_log(out, (const char *)"DEV ID      : 0x%08lx%04lx\r\n", devID[0] + devID[2], devID[1] >> 16);	//the same as USB "Device instance path"
	//print_log(out, (const char *)"DEV ID1     : 0x%08lx\r\n", devID[1]);
	//print_log(out, (const char *)"DEV ID2     : 0x%08lx\r\n", devID[2]);
	print_log(out, (const char *)"CPU Clock   : %ld\r\n", SystemCoreClock);
	print_log(out, (const char *)"Vector base : 0x%lx\r\n", (long)&g_pfnVectors);
	print_log(out, (const char *)"MEMPool size: %ld\r\n", (long)MEM_POOL_SEG_SIZE);
	print_log(out, (const char *)"MEMPool num : %ld\r\n", (long)MEM_POOL_SEGMENTS);
	print_log(out, (const char *)"MEMPool stat: %ld | %ld\r\n", (long)MEM_PoolWatermark(), (long)MEM_PoolAvailable());

	/* display voltage and temperature */
	ADC3_RunOnce(out, 1);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_debug(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	GDebug = res->val[0];
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_dumpm(TCMD_DEC_Results *res, EResultOut out)
{
	hex_dump((uint8_t *)res->val[0], (uint16_t)res->val[1], (int)res->val[2], out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_bootld(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;
	(void)res;

	RTC->BKP0R = 0x0000DF59;
	RTC->BKP7R = 0x00000001;
	NVIC_SystemReset();

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_usr(TCMD_DEC_Results *res, EResultOut out)
{
	if (strncmp(res->opt, "-d", 2) == 0)
	{
		/* define the user command */
		if (res->str)
			strncpy(usrCmdBuf, res->str, sizeof(usrCmdBuf)-1);
		else
			usrCmdBuf[0] = '\0';

		res->ctl = 1;			//break the outer command interpreter, we can have ';', done here

		return CMD_DEC_OK;
	}

	if (strncmp(res->opt, "-p", 2) == 0)
	{
		/* print the defined the user command */
		UART_Send((uint8_t *)usrCmdBuf, (int)strlen(usrCmdBuf), out);
		UART_Send((uint8_t *)"\r\n", 2, out);

		return CMD_DEC_OK;
	}

	if (usrCmdBuf[0] != '\0')
		return CMD_DEC_execute(usrCmdBuf, out);

	/* empty command, if not defined it is OK */
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_usr2(TCMD_DEC_Results *res, EResultOut out)
{
	if (strncmp(res->opt, "-d", 2) == 0)
	{
		/* define the user command */
		if (res->str)
			strncpy(usrCmdBuf2, res->str, sizeof(usrCmdBuf2)-1);
		else
			usrCmdBuf2[0] = '\0';

		res->ctl = 1;			//break the outer command interpreter, we can have ';', done here

		return CMD_DEC_OK;
	}

	if (strncmp(res->opt, "-p", 2) == 0)
	{
		/* print the defined the user command */
		UART_Send((uint8_t *)usrCmdBuf2, (int)strlen(usrCmdBuf2), out);
		UART_Send((uint8_t *)"\r\n", 2, out);

		return CMD_DEC_OK;
	}

	if (usrCmdBuf2[0] != '\0')
		return CMD_DEC_execute(usrCmdBuf2, out);

	/* empty command, if not defined it is OK */
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_var(TCMD_DEC_Results *res, EResultOut out)
{
	if (strncmp(res->opt, "-d", 2) == 0)
	{
		/* set a user variable, using '$' */
		usrVar = res->val[0];

		return CMD_DEC_OK;
	}

	//print user variable if entered as command '$' without -d
	print_log(out, (const char *)"user var : 0x%08lx\r\n", usrVar);
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_led(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	switch (res->val[0])
	{
	case 0 :
		if (res->val[1])
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_SET);
		break;
	case 1 :
		if (res->val[1])
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);
		break;
	case 2 :
		if (res->val[1])
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_RESET);
		else
			HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_SET);
		break;
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_ipaddr(TCMD_DEC_Results *res, EResultOut out)
{
	extern unsigned long gIPAddr;
	(void)res;

	print_log(out, (const char *)"IP address : %d.%d.%d.%d\r\n", (int)(gIPAddr & 0xFF), (int)((gIPAddr >> 8) & 0xFF), (int)((gIPAddr >> 16) & 0xFF), (int)((gIPAddr >> 24) & 0xFF));
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_syscfg(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;

	if (res->opt)
	{
		if (strncmp(res->opt, "-d", 2) == 0)
		{
			SYSCFG_Default();
		}

		return CMD_DEC_OK;
	}

	SYSCFG_print(out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_syserr(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;
	unsigned long err;

	if (res->opt)
	{
		if (strncmp(res->opt, "-d", 2) == 0)
		{
			SYS_DisplayError(out);
			SYS_ClrError();
		}
	}
	else
	{
		err = SYS_GetError();
		SYS_ClrError();

		print_log(out, "0x%08lx\r\n", err);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_setcfg(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	SYSCFG_setCfg(res->val[0], res->val[1]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_pmicr(TCMD_DEC_Results *res, EResultOut out)
{
	uint8_t data[1];

	I2C_PMIC_MemRead(PMIC_I2C_SLAVE_ADDR, res->val[0], data, 1);
	print_log(out, (const char *)"PMIC 0x%02x | 0x%02x\r\n", (int)res->val[0], data[0]);
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_pmicw(TCMD_DEC_Results *res, EResultOut out)
{
	uint8_t data[2];

	data[0] = (uint8_t)res->val[0];
	data[1] = (uint8_t)res->val[1];
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_epmiccfg(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;
	(void)res;

	PMIC_Recover();
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_rawspi(TCMD_DEC_Results *res, EResultOut out)
{
	int i;
	for (i = 0; i < res->num; i++)
		SPIbuf.spiTx[i] = (uint8_t)res->val[i];

	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, res->num);
	for (i = 0; i < res->num; i++)
		print_log(out, "%02x ", SPIbuf.spiRx[i]);
	print_log(out, "\r\n");
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_spiclk(TCMD_DEC_Results *res, EResultOut out)
{
	if (res->val[0])
		SPI_SetConfig(res->val[0]);
	else
	{
		int freq;
		freq = SPI_GetClock();
		print_log(out, "SPI clock: %d\r\n", freq);
	}
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2cwr(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;
	int i;
	if (res->num < 1)			//at least a register address
		return CMD_DEC_INVPARAM;

	if (res->num == 1)
		res->num = 2;			//we write a value 0
	if (res->num > sizeof(I2CBuf))
		res->num = sizeof(I2CBuf);

	for (i = 0; i < res->num; i++)
		I2CBuf[i] = (uint8_t)res->val[i];

	I2CUser_MemWrite(I2CBuf, res->num);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2crr(TCMD_DEC_Results *res, EResultOut out)
{
	int i;
	if (res->num < 1)
		return CMD_DEC_INVPARAM;

	if (res->val[1] > (sizeof(I2CBuf) -1))
		res->val[1] = sizeof(I2CBuf) -1;
	if (res->val[1] == 0)
		res->val[1] = 1;

	if ( ! I2CUser_MemRead((uint16_t)res->val[0], I2CBuf, res->val[1]))
	{
		for (i = 0; i < res->val[1]; i++)
		{
			print_log(out, "%02x ", I2CBuf[i]);
		}
		print_log(out, "\r\n");
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2c2wr(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;
	int i;
	if (res->num < 1)			//at least a register address
		return CMD_DEC_INVPARAM;

	if (res->num == 1)
		res->num = 2;			//we write a value 0
	if (res->num > sizeof(I2CBuf))
		res->num = sizeof(I2CBuf);

	for (i = 0; i < res->num; i++)
		I2CBuf[i] = (uint8_t)res->val[i];

	I2CUser_MemWrite2(I2CBuf, res->num);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2c2rr(TCMD_DEC_Results *res, EResultOut out)
{
	int i;
	if (res->num < 1)
		return CMD_DEC_INVPARAM;

	if (res->val[1] > (sizeof(I2CBuf) -1))
		res->val[1] = sizeof(I2CBuf) -1;
	if (res->val[1] == 0)
		res->val[1] = 1;

	if ( ! I2CUser_MemRead2((uint16_t)res->val[0], I2CBuf, res->val[1]))
	{
		for (i = 0; i < res->val[1]; i++)
		{
			print_log(out, "%02x ", I2CBuf[i]);
		}
		print_log(out, "\r\n");
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2cclk(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	I2CUser_ClockSelect(res->val[0]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_test(TCMD_DEC_Results *res, EResultOut out)
{
#if 0
	//SPI test
	int i;
	if (res->val[0])
		res->num = 4*1024 -1;					//1: smaller as 64K
	else
		res->num = SPI_MAX_LEN;					//0: larger 64K
	for (i = 0; i < res->num; i++)
		SPIbuf.spiTx[i] = (uint8_t)i;

	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, res->num);
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, res->num);
	for (i = res->num - 32; i < res->num; i++)
		print_log(out, "%02x ", SPIbuf.spiRx[i]);
	print_log(out, "\r\n");
#endif

#if 0
	//Backup RAM test: ATTENTION: MCU Clock <= 450 MHz - otherwise random crashes
	//Remark: Backup RAM is cleared with fwreset - how to avoid?
	unsigned long *bram = (unsigned long *)0x38800000;
	print_log(UART_OUT, "Backup RAM: %lx\r\n", *bram);
	*bram = 0x12345678;
	print_log(UART_OUT, "Backup RAM: %lx\r\n", *bram);
#endif
#if 0
	//D3 RAM test
	unsigned long *bram = (unsigned long *)0x38000000;
	print_log(UART_OUT, "Backup RAM: %lx\r\n", *bram);
	*bram = 0x12345678;
	print_log(UART_OUT, "Backup RAM: %lx\r\n", *bram);
#endif
#if 0
	print_log(UART_OUT, "RTC BKP0R: %lx\r\n", RTC->BKP0R);
	RTC->BKP0R = 0x12345678;
	print_log(UART_OUT, "RTC BKP0R: %lx\r\n", RTC->BKP0R);
#endif

#if 0
	I2C_PMIC_ReadOTP();
#endif

#if 0
	//set PMIC STANDBY pin, PJ0: low is RUN, high is STANDBY
	//see via reading PMIC register 0x67 (for STATE info)
	if (res->val[0])
		HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_0, GPIO_PIN_SET);		//high active: STANDBY PMIC
	else
		HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_0, GPIO_PIN_RESET);	//high active: RUN PMIC
#endif

#if 0
	{
		unsigned long *addr = (unsigned long *)res->val[0];
		int i;
		if (res->num < 2)
			return CMD_DEC_OK;
		for (i = 1; i < res->num; i++)
			*addr++ = res->val[i];
	}
#endif

#if 0
	{
		volatile unsigned long val = 0;
		volatile unsigned long off = 0;
		volatile unsigned long *addr;

		while (1)
		{
			//write SDRAM
			val = off;
			addr = (unsigned long *)SDRAM_START_ADDRESS;
			while (addr < (unsigned long *)(SDRAM_START_ADDRESS + SDRAM_SIZE_BYTES))
				*addr++ = val++;

			//read back and compare
			val = off;
			addr = (unsigned long *)SDRAM_START_ADDRESS;
			while (addr < (unsigned long *)(SDRAM_START_ADDRESS + SDRAM_SIZE_BYTES))
			{
				if (*addr != val)
				{
					print_log(UART_OUT, "*E: SDRAM error: %08lx: %08lx | %08lx\r\n", (unsigned long)addr, *addr, val);
					hex_dump((uint8_t *)addr, 32, 1, UART_OUT);
					osDelay(10);
					break;
					//return CMD_DEC_OK;
				}
				addr++;
				val++;
			}

			if (addr >= (unsigned long *)(SDRAM_START_ADDRESS + SDRAM_SIZE_BYTES))
			{
				print_log(UART_OUT, "*I: SDRAM OK: %08lx\r\n", off);
				osDelay(1);
			}

			off++;
		}
	}
#endif

	return CMD_DEC_OK;
}

#if 0
//--------------------------------------------------------------
int GVar = 1;
int *sAddr = (int *)4;			//initialized: .data, not initialized (implizit 0 or 0): .bss

//if very simple and static code - it is expanded like an inline function!
void FuncWithStatic(void)
{
	static int lStaticVar = 2;	//compiler optimizes to do on startup code
	sAddr = &lStaticVar;

	lStaticVar = 3;

	print_log(UART_OUT, "    FuncWithStatic() was called\r\n");
}
#endif

ECMD_DEC_Status CMD_test2(TCMD_DEC_Results *res, EResultOut out)
{
#if 0
    print_log(UART_OUT, "&GVar=0x%08lx | &sAddr=0x%08lx | sAddr=0x%08lx\r\n", (uint32_t)&GVar, (uint32_t)&sAddr, (uint32_t)sAddr);

#if 1
    //run first time w/o this code!
    //for 2nd compile & run use the address spit out
    print_log(UART_OUT, "--> *sAddr=0x%08x\r\n", *((int *)0x24000014));	//figure out the address of lStaticVar
#endif
    FuncWithStatic();
    //watch the address spit out and modify above
    print_log(UART_OUT, "    sAddr=0x%08lx\r\n", (uint32_t)sAddr);
    print_log(UART_OUT, "--> *sAddr=0x%08x\r\n", *sAddr);
#else
#if 0
    extern void testOOP_CCall(void);

    testOOP_CCall();
#endif
#endif

#if 0
    ADC3_RunOnce(out, 1);
    ADC3_Voltage(out);
#endif

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_usb(TCMD_DEC_Results *res, EResultOut out)
{
	////extern PCD_HandleTypeDef hpcd;
	////HAL_PCD_Stop(&hpcd);
	/* it does not help to solve the INCOMPISOOUT issue! */

	USB_Interface(res->val[0]);	//1 is USB Audio to PC, 4 is VCP, later add 2 for SD Card
	//USB_Interface(4);			//VCP works! Rx from PC is OK, for Tx to PC we need separate function and call
	//it mirrors echo and prints on USB-C UART

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_mic(TCMD_DEC_Results *res, EResultOut out)
{
	//set the gain, 0 is off
	PDM_MIC_Init(res->val[0], res->val[1], res->val[2]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_mics(TCMD_DEC_Results *res, EResultOut out)
{
	//sample one PDM MIC buffer
	PDM_MIC_Sample();

	return CMD_DEC_OK;
}

//--------------------------------------------------------------
ECMD_DEC_Status CMD_cgpio(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	GPIO_Config(res->val[0], res->val[1]);
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_pgpio(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	GPIO_Set(res->val[0], res->val[1]);
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_ggpio(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;

	unsigned long val;
	val = GPIO_Get();
	print_log(out, "0x%08lx\r\n", val);
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_syshd(TCMD_DEC_Results *res, EResultOut out)
{
	hex_dump((uint8_t *)res->val[0], (uint16_t)res->val[1], res->val[2], out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdinit(TCMD_DEC_Results *res, EResultOut out)
{
	if (res->val[0] != 0)
	{
		if (strncmp(res->opt, "-f", 2) == 0)
			SDCard_Init(2);			//format !!!
		else
			SDCard_Init(1);
		/* do also an "sddir" - otherwise USB MSC will not work! */
		if (strncmp(res->opt, "-s", 2) == 0)
			SDCard_ScanFiles("/", SILENT);
		else
			SDCard_ScanFiles("/", out);
	}
	else
		SDCard_Init(0);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results *res, EResultOut out)
{
	if (res->str)
		SDCard_ScanFiles(res->str, out);
	else
		SDCard_ScanFiles("", out);

	return CMD_DEC_OK;
}

#define FILE_LINE_SIZE	MEM_POOL_SEG_SIZE

ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results *res, EResultOut out)
{
	FIL *MyFile;     	/* File object */
	char *fileLine;		/* buffer for single line */
	int hexMode = 0;
	unsigned int numRd;

	if (strncmp(res->opt, "-h", 2) == 0)
		hexMode = 1;
	if (strncmp(res->opt, "-H", 2) == 0)
		hexMode = 4;

	if(SDCard_GetStatus() && res->str)
	{
		MyFile = MEM_PoolAlloc(sizeof(FIL));
		if ( ! MyFile)
			return CMD_DEC_OOMEM;
		fileLine = MEM_PoolAlloc(FILE_LINE_SIZE);
		if ( ! fileLine)
		{
			MEM_PoolFree(MyFile);
			return CMD_DEC_OOMEM;
		}

		if(f_open(MyFile, res->str, FA_READ) != FR_OK)
		{
			MEM_PoolFree(MyFile);
			MEM_PoolFree(fileLine);
			return CMD_DEC_ERROR;			//error
		}
		else
		{
			if ( ! hexMode)
			{
				while (f_gets(fileLine, FILE_LINE_SIZE -1, MyFile) != 0)
				{
					UART_Send((uint8_t *)fileLine, (int)strlen(fileLine), out);
					//in case there is just '\n' in file - send '\r'
					if (out != HTTPD_OUT)
						UART_Send((uint8_t *)"\r", 1, out);
				}
			}
			else
			{
				//read one 512byte sector or max. allocated size and print in hex
				f_read(MyFile, fileLine, FILE_LINE_SIZE -1, &numRd);
				if (numRd > 0)
				{
					hex_dump((uint8_t *)fileLine, numRd, hexMode, out);
					if (out != HTTPD_OUT)
						UART_Send((uint8_t *)"\r", 1, out);
				}
			}

		    f_close(MyFile);
		    MEM_PoolFree(MyFile);
		    MEM_PoolFree(fileLine);

			//print a newline - just in case the file ends without it
			UART_Send((uint8_t *)"\n", 1, out);
			//send '\r'
			if (out != HTTPD_OUT)
				UART_Send((uint8_t *)"\r", 1, out);

		    return CMD_DEC_OK;
		}
	}
	else
		return CMD_DEC_ERROR;
}

ECMD_DEC_Status CMD_sddel(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	if(SDCard_GetStatus() && res->str)
		f_unlink(res->str);

	return CMD_DEC_ERROR;
}

ECMD_DEC_Status CMD_sdwrite(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	FIL *MyFile;     	/* File object */
	char *fileName;
	char *restOfLine;
	int l;
	char *multiLineBuf;
	unsigned int bufIdx;
	int stop;
	UINT bw;
	/* ATTENTION: we overwrite the same command line buffer !!! */
	/* open file first as long as command line is still correct */

	if(SDCard_GetStatus() && res->str)
	{
		/* SDCard open and a filename as str needed ! */
		if (strncmp(res->opt, "-m", 2) == 0)
		{
			/* open file first with res->str ! afterwards all free on same buffer! */
			MyFile = MEM_PoolAlloc(sizeof(FIL));
			if ( ! MyFile)
			{
				return CMD_DEC_OOMEM;
			}

			if (f_open(MyFile, res->str, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
			{
				MEM_PoolFree(MyFile);
				return CMD_DEC_OOMEM;
			}
			multiLineBuf = MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
			if (!multiLineBuf)
			{
				f_close(MyFile);
				MEM_PoolFree(MyFile);
				return CMD_DEC_OOMEM;
			}

			bufIdx = 0;
			stop = 0;

			//multi-line write to SD card
			while(1)
			{
				UART_Send((uint8_t *)": ", 2, UART_OUT);
				do {
					osThreadYield();
					l = UART_getString(strRxBuf, sizeof(strRxBuf));
				} while (!l);
				if (l > 1)			//it ignores empty line, just a newline entered is ignored
				{
					//should we stop with CTRL-Z
					if (strRxBuf[l - 2] == 0x1A)	//finish with CTRL-Z
					{
						stop = 1;
					}
					//check if we would overflow the buffer
					if ((bufIdx + l) > (MEM_POOL_SEG_SIZE-3))			//with CR, NEWLINE and NUL at the end!
					{
						l = (MEM_POOL_SEG_SIZE-3) - bufIdx;
						stop = 1;
					}

					if (stop)
					{
						if (l > 2)
						{
							//take the last without CTRL-Z
							memcpy(multiLineBuf + bufIdx, strRxBuf, l - 2);
							bufIdx += l - 2;		//ignore NEWLINE, we append CR + NEWLINE
							strcpy(multiLineBuf + bufIdx, "\r\n");	//with a NUL at the end!
							bufIdx += 2;
						}

						//write all to disk, close and release all
						f_write(MyFile, multiLineBuf, bufIdx, &bw);
						f_close(MyFile);

						/* TODO: ATTETNION: print_log on a buffer we release in between - it FAILS! */

						MEM_PoolFree(MyFile);
						MEM_PoolFree(multiLineBuf);
						break;
					}
					//copy into buffer
					memcpy(multiLineBuf + bufIdx, strRxBuf, l - 1);
					bufIdx += l - 1;		//ignore NEWLINE, we append CR + NEWLINE
					strcpy(multiLineBuf + bufIdx, "\r\n");	//with a NUL at the end!
					bufIdx += 2;
				}
			}

			//kill the last string as a valid command to repeat - it is garbage!
			////UART_killLastStr();
			return CMD_DEC_OK;
		}
		else //without any option - single line write to file
		{
			res->ctl = 1; //break rest of line as command!

			MyFile = MEM_PoolAlloc(sizeof(FIL));
			if ( ! MyFile)
				return CMD_DEC_OOMEM;

			/* get pointer to file name - ATT: we write a NUL to space location ! */
			fileName = res->str;
			/* find the rest of line, set NUL at space delimiter ! - if not there, rest of line is empty! */
			restOfLine = res->str;

			while (*restOfLine > ' ')
				restOfLine++;
			if (*restOfLine != '\0')
			{
				*restOfLine++ = '\0';
				if (*restOfLine == '\0')
				{
					MEM_PoolFree(MyFile);
					return CMD_DEC_OK;			//there is nothing afterwards
				}
			}
			else
			{
				MEM_PoolFree(MyFile);
				return CMD_DEC_OK;				//there is nothing afterwards
			}

			if (f_open(MyFile, fileName, FA_CREATE_ALWAYS | FA_WRITE) == FR_OK)
			{
				f_write(MyFile, restOfLine, strlen(restOfLine), &bw);
				f_write(MyFile, "\r\n", 2, &bw);	//append a newline, otherwise not there from command line
				f_close(MyFile);
			}

			MEM_PoolFree(MyFile);
		}
		return CMD_DEC_OK;
	}

	return CMD_DEC_ERROR;
}

ECMD_DEC_Status CMD_sdexec(TCMD_DEC_Results *res, EResultOut out)
{
	FIL *MyFile;     	/* File object */
	char *fileLine;		/* buffer for single line */
	int silent = 0;

	if(SDCard_GetStatus() && res->str)
	{
		MyFile = MEM_PoolAlloc(sizeof(FIL));
		if ( ! MyFile)
			return CMD_DEC_OOMEM;
		fileLine = MEM_PoolAlloc(FILE_LINE_SIZE);
		if ( ! fileLine)
		{
			MEM_PoolFree(MyFile);
			return CMD_DEC_OOMEM;
		}

		if (strncmp(res->opt, "-s", 2) == 0)
			silent = 1;

		if(f_open(MyFile, res->str, FA_READ) != FR_OK)
		{
			MEM_PoolFree(MyFile);
			MEM_PoolFree(fileLine);
			return CMD_DEC_ERROR;			//error
		}
		else
		{
		    while (f_gets(fileLine, FILE_LINE_SIZE -1, MyFile) != 0)
		    {
		    	if ( ! silent)
		    	{
#if 0
		    		/* print to see as log what came from file, '$' is 'file prompt' */
		    		UART_Send((uint8_t *)"$ ", 2, out);
		    		/* log entire line - even with comments */
		    		UART_Send((uint8_t *)fileLine, (int)strlen(fileLine), out);
		    		if (out != HTTPD_OUT)
		    			/* just in case there is not '\r' in file, just Linux '\n' */
		    			UART_Send((uint8_t *)"\r", 1, out);
#else
		    		/* strip of the comments */
		    		{
		    			char *strComment;
		    			int strLenLine;
		    			int appendNL = 0;
		    			strComment = strchr(fileLine, '#');
		    			if (strComment)
		    			{
		    				strLenLine = strComment - fileLine;
		    				if (strLenLine)				/* avoid empty line if just commented line */
		    					appendNL = 2;
		    			}
		    			else
		    			{
		    				strLenLine = (int)strlen(fileLine);
		    				/* is there a \r from file? we assume there is always a \n from Unix file */
		    				if (strchr(fileLine, '\r') == NULL)
		    					appendNL = 1;
		    			}
		    			if (strLenLine)
		    			{
		    				UART_Send((uint8_t *)"$ ", 2, out);
		    				UART_Send((uint8_t *)fileLine, strLenLine, out);
		    			}
		    			else
		    				continue;
		    			if (appendNL == 2)
		    				UART_Send((uint8_t *)"\r\n", 2, out);
		    			if (appendNL == 1)
		    				UART_Send((uint8_t *)"\r", 1, out);		//it becomes \n\r
		    		}
#endif
		    	}
		    	/* now execute command */
		    	CMD_DEC_execute(fileLine, out);

		    	/* Q: should we stop on first CMD error, e.g. wrong command? */
		    }

		    f_close(MyFile);
		    MEM_PoolFree(MyFile);
		    MEM_PoolFree(fileLine);
		    return CMD_DEC_OK;
		}
	}
	else
		return CMD_DEC_ERROR;
}

ECMD_DEC_Status CMD_sd2qspi(TCMD_DEC_Results *res, EResultOut out)
{
	FIL *MyFile;     	/* File object */
	uint8_t *data;		/* buffer for single line */
	UINT len;			/* length of file we have read */

	if(SDCard_GetStatus() && res->str)
	{
		MyFile = MEM_PoolAlloc(sizeof(FIL));
		if ( ! MyFile)
			return CMD_DEC_OOMEM;
		data = MEM_PoolAlloc(FILE_LINE_SIZE);
		if ( ! data)
		{
			MEM_PoolFree(MyFile);
			return CMD_DEC_OOMEM;
		}

		/* terminate str before the parameters */
		{
			char *s;
			s = strstr(res->str, " ");
			if (s)
				*s = '\0';
		}

		if(f_open(MyFile, res->str, FA_READ) != FR_OK)
		{
			MEM_PoolFree(MyFile);
			MEM_PoolFree(data);
			return CMD_DEC_ERROR;			//error
		}
		else
		{
			memset(data, 0xFF, 4096);
			len = 0;
			f_read(MyFile, data, 4096, &len);		//read max. one page

			print_log(UART_OUT, (const char*)"*D: len: %d\n\r", len);

			if (BSP_QSPI_Write(0, data, res->val[0] * MT25TL01G_SUBSECTOR_SIZE, len) != 0)
			{
			     print_log(UART_OUT, (const char*)"*E: QSPI write failed\n\r");
			}
			else
			{
			     print_log(UART_OUT, (const char*)"*I: QSPI write OK\n\r");
			}

		    f_close(MyFile);
		    MEM_PoolFree(MyFile);
		    MEM_PoolFree(data);
		    return CMD_DEC_OK;
		}
	}
	else
		return CMD_DEC_ERROR;
}

ECMD_DEC_Status CMD_picoc(TCMD_DEC_Results *res, EResultOut out)
{
#if 0
	osPriority_t prio;
	osThreadId_t id;

	id = osThreadGetId();
	prio = osThreadGetPriority(id);
	osThreadSetPriority(id, osPriorityBelowNormal);
#endif

	pico_c_main_interactive(0, NULL);

#if 0
	osThreadSetPriority(id, prio);
#endif

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_res(TCMD_DEC_Results* res, EResultOut out)
{
	(void)out;

	GPIO_resLevel(res->val[0]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_bggpio(TCMD_DEC_Results* res, EResultOut out)
{
	unsigned long l;

	//TODO: add -d for table with pin names
	l = GPIO_BitGet(res->val[0]);
	print_log(out, "GPIO: %1lx\r\n", l);	//just a single bit

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_bpgpio(TCMD_DEC_Results* res, EResultOut out)
{
	(void)out;

	GPIO_BitSet(res->val[0], res->val[1]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_spitr(TCMD_DEC_Results* res, EResultOut out)
{
	int i;

	if (((GSysCfg.SPImode & CFG_SPI_WORD) >> 8) == 4)
	{
		for (i = 0; i < res->num; i++)
		{
			//little endian 32bit words
			SPIbuf.spiTx[4 * i + 0] = (char)(res->val[i] >>  0);
			SPIbuf.spiTx[4 * i + 1] = (char)(res->val[i] >>  8);
			SPIbuf.spiTx[4 * i + 2] = (char)(res->val[i] >> 16);
			SPIbuf.spiTx[4 * i + 3] = (char)(res->val[i] >> 24);
		}
		CHIP_SwapEndian(SPIbuf.spiTx, res->num * 4);
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, res->num * 4);
		CHIP_SwapEndian(SPIbuf.spiRx, res->num * 4);

		hex_dump(SPIbuf.spiRx, res->num * 4, 4, out);		//len in number of bytes
	}

	if (((GSysCfg.SPImode & CFG_SPI_WORD) >> 8) == 2)
	{
		for (i = 0; i < res->num; i++)
		{
			//little endian 16bit words
			SPIbuf.spiTx[2 * i + 0] = (char)(res->val[i] >>  0);
			SPIbuf.spiTx[2 * i + 1] = (char)(res->val[i] >>  8);
		}
		CHIP_SwapEndian(SPIbuf.spiTx, res->num * 2);
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, res->num * 2);
		CHIP_SwapEndian(SPIbuf.spiRx, res->num * 2);

		hex_dump(SPIbuf.spiRx, res->num * 2, 2, out);		//len in number of bytes
	}

	if (((GSysCfg.SPImode & CFG_SPI_WORD) >> 8) == 1)
	{
		for (i = 0; i < res->num; i++)
		{
			//8bit words
			SPIbuf.spiTx[i] = (char)(res->val[i]);
		}
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, res->num);

		hex_dump(SPIbuf.spiRx, res->num, 1, out);			//len in number of bytes
	}

	return CMD_DEC_OK;
}

#if 0
/* the SPI commands require the allocation of buffers, SPItr and SPIrx can be the same buffer
 * convert the uint32_t in TCMD_DEC_Results into the right format in the buffer
 */

ECMD_DEC_Status CMD_cid(TCMD_DEC_Results* res, EResultOut out)
{
	uint32_t cid;

	CHIP_cid(&cid);

	if (GSysCfg.ChipNo == 0)
	{
		print_log(out, "%08lx\r\n", cid);
	}
	if (GSysCfg.ChipNo == 1)
	{
		print_log(out, "%04lx\r\n", cid);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_rreg(TCMD_DEC_Results* res, EResultOut out)
{
	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (GSysCfg.ChipNo == 1)
	{
		int num;
		uint16_t *values;

		values = (uint16_t *)&res->val[0];
		//TODO: place check for num > CMD_DEC_NUM_VAL here
		num = CHIP_rreg(res->val[0], (uint32_t *)values, res->val[1]);

		while (num)
		{
			print_log(out, "%04x ", *values++);
			num--;
		}
		print_log(out, "\r\n");
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_wreg(TCMD_DEC_Results* res, EResultOut out)
{
	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (GSysCfg.ChipNo == 1)
	{
		CHIP_wreg(res->val[0], &res->val[1], res->num - 1);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_rblk(TCMD_DEC_Results* res, EResultOut out)
{
	int option = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (strncmp(res->opt, "-s", 2) == 0)
		option = 2;
	if (strncmp(res->opt, "-b", 2) == 0)
		option = 1;

	if (GSysCfg.ChipNo == 1)
	{
		int num;
		uint16_t *values;
		uint8_t *valuesB;

		values  = (uint16_t *)&res->val[0];
		valuesB = (uint8_t *) &res->val[0];
		//TODO: place check for num > CMD_DEC_NUM_VAL here
		num = CHIP_rblk(res->val[0], (uint32_t *)values, res->val[1], option);

		while (num > 0)
		{
			if (option == 0)
				print_log(out, "%04x ", *values++);
			if (option == 1)
				print_log(out, "%02x ", *valuesB++);
			if (option == 2)
			{
				//as 12bit values
				print_log(out, (const char *)"%03X ", (int)(*valuesB | ((*(valuesB + 1) & 0xF) << 8)));
				print_log(out, (const char *)"%03X ", (int)(((*(valuesB + 1) >> 4) & 0xF) | (*(valuesB + 2) << 4)));
				valuesB += 3;
				num -= 2;
			}
			num--;
		}
		print_log(out, "\r\n");
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_wblk(TCMD_DEC_Results* res, EResultOut out)
{
	int option = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (strncmp(res->opt, "-s", 2) == 0)
		option = 2;
	if (strncmp(res->opt, "-b", 2) == 0)
		option = 1;

	if (GSysCfg.ChipNo == 1)
	{
		CHIP_wblk(res->val[0], &res->val[1], res->num - 1, option);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_rslice(TCMD_DEC_Results* res, EResultOut out)
{
	int option = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (strncmp(res->opt, "-s", 2) == 0)
		option = 2;
	if (strncmp(res->opt, "-b", 2) == 0)
		option = 1;

	if (GSysCfg.ChipNo == 1)
	{
		/* it prints inside the called function */
		CHIP_rslice(res->val[0], res->val[1], option);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_wslice(TCMD_DEC_Results* res, EResultOut out)
{
	int option = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (strncmp(res->opt, "-s", 2) == 0)
		option = 2;
	if (strncmp(res->opt, "-b", 2) == 0)
		option = 1;

	if (GSysCfg.ChipNo == 1)
	{
		CHIP_wslice(res->val[0], &res->val[1], res->num - 1, option);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_fslice(TCMD_DEC_Results* res, EResultOut out)
{
	int option = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;

	if (strncmp(res->opt, "-s", 2) == 0)
		option = 2;
	if (strncmp(res->opt, "-b", 2) == 0)
		option = 1;

	if (GSysCfg.ChipNo == 1)
	{
		CHIP_fslice(res->val[0], res->val[1], res->val[2], option);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_noop(TCMD_DEC_Results* res, EResultOut out)
{
	if (GSysCfg.ChipNo == 1)
	{
		CHIP_noop(res->val[0], res->val[1]);
	}

	return CMD_DEC_OK;
}
#endif

ECMD_DEC_Status CMD_cint(TCMD_DEC_Results* res, EResultOut out)
{
	(void)out;

	GPIO_INTConfig((int)res->val[0], (int)res->val[1]);
	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_aint(TCMD_DEC_Results *res, EResultOut out)
{
	if (strncmp(res->opt, "-d", 2) == 0)
	{
		/* delete the INT action */
		intCmdBuf[0] = '\0';

		return CMD_DEC_OK;
	}

	if (strncmp(res->opt, "-p", 2) == 0)
	{
		/* print the defined the user command */
		UART_Send((uint8_t *)intCmdBuf, (int)strlen(intCmdBuf), out);
		UART_Send((uint8_t *)"\r\n", 2, out);

		return CMD_DEC_OK;
	}

	/* define INT action */
	if (res->str)
		strncpy(intCmdBuf, res->str, sizeof(intCmdBuf)-1);
	else
		intCmdBuf[0] = '\0';

	return CMD_DEC_OK;
}

/* helper function: invoke the aint Action INT command */
int CMD_invokeAINT(void)
{
	if (intCmdBuf[0] != '\0')
	{
		CMD_DEC_execute(intCmdBuf, UART_OUT);
		return 1;
	}

	return 0;
}

ECMD_DEC_Status CMD_qspi(TCMD_DEC_Results *res, EResultOut out)
{
	QSPI_demo((int)res->val[0], res->val[1]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_msdelay(TCMD_DEC_Results *res, EResultOut out)
{
	osDelay(res->val[0]);

	return CMD_DEC_OK;
}

#ifdef UART_TEST
ECMD_DEC_Status CMD_uspeedtest(TCMD_DEC_Results *res, EResultOut out)
{
	uint32_t startTick;
	static uint8_t b[256];
	int i, c;

	for (i = 0; i < 256; i++)
		b[i] = 0x20 + i;

	startTick = HAL_GetTick();
	c = 0;
	while (1)
	{
		UART_Send(b, 256, UART_OUT);
		c++;
#if 0
		if (UART_WaitForChar())
			break;
#else
		if (c >= 10000)
			break;
#endif
	}

	print_log(UART_OUT, "\r\nUART speed: %d bytes in %ld ticks\r\n", c * 256, HAL_GetTick() - startTick);

	return CMD_DEC_OK;
}
#endif

ECMD_DEC_Status CMD_volt(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;

	ADC3_RunOnce(out, 0);	//silent
	ADC3_Voltage(out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_udpip(TCMD_DEC_Results *res, EResultOut out)
{
	extern void SetUDPDest(unsigned long ipAddr);

	(void)out;
	unsigned long ip[4];
	unsigned long ipAddr;

	sscanf(res->str, "%lu.%lu.%lu.%lu", &ip[0], &ip[1], &ip[2], &ip[3]);
	ipAddr  = ip[0];
	ipAddr |= ip[1] << 8;
	ipAddr |= ip[2] << 16;
	ipAddr |= ip[3] << 24;

	SetUDPDest(ipAddr);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_cservo(TCMD_DEC_Results *res, EResultOut out)
{
	PWM_ServoInit();

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sservo(TCMD_DEC_Results *res, EResultOut out)
{
	PWM_ServoSet((int)res->val[0], (int)res->val[1]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_pwr(TCMD_DEC_Results *res, EResultOut out)
{
	PWR_Set((int)res->val[0]);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_tofc(TCMD_DEC_Results *res, EResultOut out)
{
	MX_TOF_Init();

	if (strncmp(res->opt, "-i", 2) == 0)
	{
		/* endless on UART */
		MX_TOF_Process();
	}
	else
	{
		/* release TOF thread, sending via network */
		extern void ReleaseTOFTask(void);
		ReleaseTOFTask();
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_tofp(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;
	extern void toggle_resolution(void);
	extern void toggle_signal_and_ambient(void);

	if (res->val[0])
		toggle_resolution();
	if (res->val[1])
		toggle_signal_and_ambient();

	return CMD_DEC_OK;
}
