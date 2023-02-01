/**
 * fsdata_custom.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include "lwip/apps/fs.h"
#include "lwip/def.h"

#define file_NULL (struct fsdata_file *) NULL

static const unsigned char data__404_html[] __attribute__((aligned(4))) = {
"/404.html\0\
HTTP/1.0 200 OK\r\n\
Server: lwIP/2.0.3 (http://savannah.nongnu.org/projects/lwip)\r\n\
Content-type: text/html\r\n\r\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n\
<html xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:w=\"urn:schemas-microsoft-com:office:word\" xmlns=\"http://www.w3.org/TR/REC-html40\">\r\n\
<head>\r\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\r\n\
<title>Sunstone</title>\r\n\
</head>\r\n\
<body lang=\"EN-US\" link=\"blue\" vlink=\"blue\" bgcolor=\"#336666\">\r\n\
<h1><font color=\"white\">DualSPIder</font></h1>\r\n\
<hr>\r\n\
<p>Page not found: please go to <a href=\"/STM32H7xx.html\">Home Page</a></p>\r\n\
</body></html>\r\n"
};

/* we use TEXTAREA: copy first part, add variable part, append second part */
const unsigned char data__STM32H7xx_html[] __attribute__((aligned(4))) = {
"/STM32H7xx.html\0\
HTTP/1.0 200 OK\r\n\
Server: lwIP/2.0.3 (http://savannah.nongnu.org/projects/lwip)\r\n\
Content-type: text/html\r\n\r\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n\
<html xmlns:v=\"urn:schemas-microsoft-com:vml\" xmlns:o=\"urn:schemas-microsoft-com:office:office\" xmlns:w=\"urn:schemas-microsoft-com:office:word\" xmlns=\"http://www.w3.org/TR/REC-html40\">\r\n\
<head>\r\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\r\n\
<title>P7SPIder</title>\r\n\
<style>\r\n\
.bg1 {background-color: rgb(180, 180, 180);}\r\n\
.bg2 {background-color: rgb(200, 200, 200);}\r\n\
.bg3 {font-family: Verdana; font-weight: bold; font-style: italic; background-color: rgb(230, 230, 230); text-align: center;}\r\n\
.bg4 {font-family: Verdana; font-weight: bold; font-style: italic; text-align: center;}\r\n\
</style>\r\n\
</head>\r\n\
<body lang=\"EN-US\" link=\"blue\" vlink=\"blue\" bgcolor=\"#336666\">\r\n\
<table style=\"width: 860px; height: 30px;\" border=\"0\" cellpadding=\"2\" cellspacing=\"2\">\r\n\
<tbody>\r\n\
<tr>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(40, 40, 255);\"><small><a href=\"/STM32H7xx.html\"><span style=\"color: white;\">Home Page</span></a></small></td>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(40, 40, 255);\"><small><span style=\"color: white;\">P7 SPIder</span></a></small></td>\r\n\
</tr>\r\n\
<tr>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(60, 160, 255);\"><strong><a href=\"/softreset.html\" style=\"text-decoration:none;\"><span style=\"color: white;\">Soft reset</span></a></strong></td>\r\n\
<td class=\"bg4\" style=\"background-color: rgb(200, 100, 255);\"><strong><a href=\"/hardreset.html\" style=\"text-decoration:none;\"><span style=\"color: white;\">Hard reset</span></a></strong></td>\r\n\
</tr>\r\n"
"<tr class=\"bg2\">\r\n\
<td colspan=\"2\">\r\n\
<form action=\"/STM32H7xx.html\">\r\n\
<input type=\"text\" name=\"CMD\" value=\"\" size=\"145\">\r\n\
</form>\r\n\
</td>\r\n\
</tr>\r\n\
<tr class=\"bg1\">\r\n\
<td colspan=\"2\">\r\n\
<form action=\"/STM32H7xx.html\">\r\n\
<textarea name=\"CMD_RES\" rows=\"30\" cols=\"140\">\r\n"
};

const unsigned char data__STM32H7xx_b_html[] __attribute__((aligned(4))) = {
"/STM32H7xx_b.html\0\
</textarea>\r\n\
</form>\r\n\
</td>\r\n\
</tr>\r\n\
</tbody>\r\n\
</table>\r\n\
</body>\r\n\
</html>\r\n"
};

const struct fsdata_file file__404_html[] = { {
file_NULL,
data__404_html,
data__404_html + 10,
sizeof(data__404_html) - 10 -1,		//sizeof is with NUL, lower by one
1,
}};

const struct fsdata_file file__STM32H7xx_html[] = { {
file__404_html,
data__STM32H7xx_html,
data__STM32H7xx_html + 16,
sizeof(data__STM32H7xx_html) - 16 -1,
1,
}};

const struct fsdata_file file__STM32H7xx_b_html[] = { {
file__STM32H7xx_html,
data__STM32H7xx_b_html,
data__STM32H7xx_b_html + 18,
sizeof(data__STM32H7xx_b_html) - 18 -1,
1,
}};

#define FS_ROOT file__STM32H7xx_b_html
#define FS_NUMFILES 3
