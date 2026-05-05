#pragma once

#include "stdarg.h"

int  u_printf(const char *fmt, ...);
void uart_printf(const char *format, ...);
int  print(char **out, const char *format, va_list args);
