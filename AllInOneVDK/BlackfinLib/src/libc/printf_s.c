/* Copyright (C) 2001 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/

/*
** A very limited printf replacement, for programs which need to
** print out strings and numbers, but which do not wish to pay the
** cost of a full stdio printf.
** To use this, use something like -Dprintf=printf_s
** This only supports %s, %x, %p, %c. %d and %i are treated as %x.
** No field widths, padding, or anything like that. No %g or %f.
*/
#pragma file_attr(  "libName=libsmall")
#pragma file_attr(  "libFunc=printf_s")
#pragma file_attr(  "libFunc=_printf_s")
#pragma file_attr(  "libFunc=fprintf_s")
#pragma file_attr(  "libFunc=_fprintf_s")
#pragma file_attr(  "prefersMem=external")
#pragma file_attr(  "prefersMemNum=70")

#include <stdarg.h>

extern int _Write(int, const char *, int);

static int prn(int num)
{
	char str[20];
	char *p = str+19;
	int i = 0,  neg = 0;
	*p = 0;

	if (num==0) {
		_Write(1,"0",1);
		return 1;
	}
	if (num<0) {
		neg = 1;
		num = -num;
	}


	while (num>0) {
		const char *digits = "0123456789abcdef";
		int n = num&0xF;
		*--p = digits[n];
		num >>= 4;
		i++;
	}
	if (neg) {
		*--p = '-';
		i++;
	}
	_Write(1,p,i);
	return i;
}



static int doprint(const char *fmt, va_list args)
{
	const char *p = fmt;
	const char *p2 = p;
	char *str, *s, c;
	int l = 0, i;

	while (*p) {
		for (p2 = p+1; *p2 && *p2 != '%'; p2++) ;
		_Write(1, p, p2-p);
		p = p2;
		l += p2-p;
		if (*p2 == '%') {
			switch (p2[1]) {
				case '%':
					_Write(1,p2,1);
					l++;
					break;
				case 's':
					s = str = va_arg(args, char *);
					for (i = 0; s[i]; i++) ;
					_Write(1,str,i);
					l += i;
					break;
				case 'd':
				case 'D':
				case 'x':
				case 'X':
				case 'p':
				case 'i':
					i = va_arg(args, int);
					l += prn(i);
					break;
				case 'c':
					c = (char)va_arg(args, int);
					_Write(1,&c,1);
					l++;
					break;
				default:
					_Write(1,p2,2);
					l += 2;
					break;
			}
			p = p2+2;
		}
	}
	return l;
}

int printf_s(const char *fmt, ...)
{
	int n;
	va_list args;
	va_start(args, fmt);
	n = doprint(fmt, args);
	va_end(args);
	return n;
}

int fprintf_s(void *ptr, const char *fmt, ...)
{
	int n;
	va_list args;
	va_start(args, fmt);
	n = doprint(fmt, args);
	va_end(args);
	return n;
}
