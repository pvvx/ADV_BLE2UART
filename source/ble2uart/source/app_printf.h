#pragma once

#include "stdarg.h"  // used for uart_printf()

int  u_printf(const char *fmt, ...);
int  u_sprintf(char* s, const char *fmt, ...);
void u_array_printf(unsigned char*data, unsigned int len);
int print(char **out, const char *format, va_list args);
int u_sprintf(char *out, const char *format, ...);  // see uprintf.mk
void uart_printf(const char *format, ...);
int raw_printf(const char *format, ...);
int p_printf(const char *format, ...);
