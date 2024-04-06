/*
 * app_printf.c (improved version of u_printf from the BLE SDK)
*/

/*
 Copyright 2001, 2002 Georges Menie (www.menie.org)
 stdarg version contributed by Christian Ettinger

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 putchar is the only external dependency for this file,
 if you have a working putchar, leave it commented out.
 If not, uncomment the define below and
 replace outbyte(c) by your own function call.

 #define putchar(c) outbyte(c)
 */
#if 1
#include <stdarg.h>

#include "drivers.h"

#ifdef PUTCHAR
#include "putchar.h"
#else
#include "drv_uart.h"

#include "scanning.h"
#include "crc.h"

extern my_fifo_t ad_fifo;

int putchar(int c){
    u8 out[] = { (u8) c };
    uart_send(out, 1);
    sleep_us(1000);
    return 1;
}
#endif

static void printchar(char **str, int c) {
	if (str) {
		**str = c;
		++(*str);
	} else
		(void) putchar(c);
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad) {
	register int pc = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr)
			++len;
		if (len >= width)
			width = 0;
		else
			width -= len;
		if (pad & PAD_ZERO)
			padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for (; width > 0; --width) {
			printchar(out, padchar);
			++pc;
		}
	}
	for (; *string; ++string) {
		printchar(out, *string);
		++pc;
	}
	for (; width > 0; --width) {
		printchar(out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad,
		int letbase) {
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints(out, print_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = -i;
	}

	s = print_buf + PRINT_BUF_LEN - 1;
	*s = '\0';

	while (u) {
		t = u % b;
		if (t >= 10)
			t += letbase - '0' - 10;
		*--s = t + '0';
		u /= b;
	}

	if (neg) {
		if (width && (pad & PAD_ZERO)) {
			printchar(out, '-');
			++pc;
			--width;
		} else {
			*--s = '-';
		}
	}

	return pc + prints(out, s, width, pad);
}

int print(char **out, const char *format, va_list args) {
	register int width, pad;
	register int pc = 0;
	char scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0')
				break;
			if (*format == '%')
				goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for (; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if (*format == 's') {
				register char *s = (char *) va_arg( args, int );
				pc += prints(out, s ? s : "(null)", width, pad);
				continue;
			}
			if (*format == 'd') {
				pc += printi(out, va_arg( args, int ), 10, 1, width, pad, 'a');
				continue;
			}
			if (*format == 'x') {
				pc += printi(out, va_arg( args, int ), 16, 0, width, pad, 'a');
				continue;
			}
			if (*format == 'X') {
				pc += printi(out, va_arg( args, int ), 16, 0, width, pad, 'A');
				continue;
			}
			if (*format == 'u') {
				pc += printi(out, va_arg( args, int ), 10, 0, width, pad, 'a');
				continue;
			}
			if (*format == 'c') {
				/* char are converted to int then pushed on the stack */
				scr[0] = (char) va_arg( args, int );
				scr[1] = '\0';
                if (scr[0] == 0x00) {
                    printchar(out, 0);
                    pc++;
                }
                else
                    pc += prints(out, scr, width, pad);
				continue;
			}
		} else {
			out: printchar(out, *format);
			++pc;
		}
	}
	if (out)
		**out = '\0';
	va_end( args );
	return pc;
}

int u_printf(const char *format, ...) {
	va_list args;
	va_start( args, format );
	return print(0, format, args);
}

int u_sprintf(char *out, const char *format, ...) {
	va_list args;
	va_start( args, format );
	return print(&out, format, args);
}

void u_array_printf(unsigned char*data, unsigned int len) {
	u_printf("{");
	for(int i = 0; i < len; ++i){
		u_printf("%X%s", data[i], i<(len)-1? ":":" ");
	}
	u_printf("}\n");
}

// Write a Command.DEBUG_PRINT packet to the USB UART and uses the FIFO.
void uart_printf(const char *format, ...) {
    u8 ret;
	va_list args;
	va_start(args, format);
	u8 *s = my_fifo_wptr(&ad_fifo);
    char * out = (char *) &s[11];
	if(s) {
		memset(s, 0, HEAD_CRC_ADD_LEN);  // MAC (last 6 bytes) is set to 0
        ret = print(&out, format, args);  // message
		s[0] = ret; // length of the message
		s[1] = CMD_ID_PRNT;  // position of the rssi
		s[2] = 0xff;
		s[3] = 0xff;
		s[4] = 0xff;
        // Mac address (6 bytes) is all 0. Total bytes: length 05 ff ff ff 00 00 00 00 00 00
		my_fifo_next(&ad_fifo);
	}
    va_end(args);
}

#define MYFIFO_BLK_SIZE		(EXTADV_RPT_DATA_LEN_MAX + HEAD_CRC_ADD_LEN) // 229+12 = 241 bytes

// Write raw data to the USB UART directly calling uart_send(), without using FIFO/headers/crc.
// The length of the message is returned, or -1 if busy.
int raw_printf(const char *format, ...) {
    u8 out[MYFIFO_BLK_SIZE];
    u8 length;
	va_list args;
    char * p = (char *) out;

	va_start(args, format);
    length = print(&p, format, args);
    va_end(args);
    return(uart_send(out, length));
}

// Write a Command.DEBUG_PRINT packet to the USB UART without using the FIFO.
// The length of the message is returned, or -1 if busy.
int p_printf(const char *format, ...) {
    u8 out[MYFIFO_BLK_SIZE];
    u8 length;
	va_list args;
    crc_t crc;
    int len;
    char * p = (char *) out + HEAD_CRC_ADD_LEN - 2;  // 2 is the CRC

	va_start(args, format);
    length = print(&p, format, args);  // message
    va_end(args);
    memset(out, 0, HEAD_CRC_ADD_LEN);  // MAC (last 6 bytes) is set to 0
    out[0] = length; // length of the message
    out[1] = CMD_ID_PRNT;  // position of the rssi
    out[2] = 0xff;
    out[3] = 0xff;
    out[4] = 0xff;
    len = length + HEAD_CRC_ADD_LEN - 2;
    crc = crcFast(out, len);
    out[len] = crc;
    out[len+1] = crc >> 8;

    return(uart_send(out, length + HEAD_CRC_ADD_LEN));
}

#endif
