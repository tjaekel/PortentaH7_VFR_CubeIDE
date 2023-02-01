/**
 * cmd_dec.h
 *
 *  Created on: Oct 18, 2022
 *  Author: Torsten Jaekel
 */

/* define VT100 ESC strings to set color, attributes on VT100 terminal receiver */

////#define VT100							//add VT100 attributes or not
/* ATT: if we use VT100 attributes - the HTML view is messed up! */

#ifdef VT100
#define VT_END		"\033[0m"
#define	VT_BOLD		"\033[1m"
#define VT_BLUEBOLD	"\033[34;1m"
#define VT_BLUE		"\033[34m"
#define VT_GREEN	"\033[32m"
#define VT_RED		"\033[31m"
#define VT_CYAN		"\033[36m"

#define VT_BG_WHITE	"\033[30;47m"

#else
#define VT_END
#define	VT_BOLD
#define VT_BLUEBOLD
#define VT_BLUE
#define VT_GREEN
#define VT_RED
#define VT_CYAN

#define VT_BG_WHITE ""
#endif
