/************************************************************************
 *
 * xprint_float.c : $Revision: 5128 $
 *
 * (c) Copyright 2004-2011 Analog Devices, Inc.  All rights reserved.
 *
 ************************************************************************/

#if !defined(__NO_LIBRARY_ATTRIBUTES__)

#pragma file_attr ("libName=libio")
#pragma file_attr ("libGroup=stdio.h")

#if defined(__ADSPBLACKFIN__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr ("libFunc=_print_float_32")
#pragma file_attr ("libFunc=__print_float_32")
#else
#pragma file_attr ("libFunc=__print_f_float_64")
#pragma file_attr ("libFunc=___print_f_float_64")
#endif
#else

#if defined(__ADSP21000__)
#if defined(__DOUBLES_ARE_FLOATS__)
#pragma file_attr ("libFunc=__print_float_32")
#else
#pragma file_attr ("libFunc=__print_f_float_64")
#endif
#endif
#endif

#pragma file_attr ("libFunc=printf")
#pragma file_attr ("libFunc=_printf")
#pragma file_attr ("libFunc=sprintf")
#pragma file_attr ("libFunc=_sprintf")
#pragma file_attr ("libFunc=fprintf")
#pragma file_attr ("libFunc=_fprintf")
#pragma file_attr ("libFunc=fputs")
#pragma file_attr ("libFunc=_fputs")
#pragma file_attr ("libFunc=fputc")
#pragma file_attr ("libFunc=_fputc")
#pragma file_attr ("libFunc=puts")
#pragma file_attr ("libFunc=_puts")
#pragma file_attr ("libFunc=putc")
#pragma file_attr ("libFunc=_putc")
#pragma file_attr ("libFunc=putchar")
#pragma file_attr ("libFunc=_putchar")
#pragma file_attr ("libFunc=vsprintf")
#pragma file_attr ("libFunc=_vsprintf")
#pragma file_attr ("libFunc=vsnprintf")
#pragma file_attr ("libFunc=_vsnprintf")
#pragma file_attr ("prefersMem=external")
#pragma file_attr ("prefersMemNum=70")

#endif

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "xprnt.h"

#if defined(__DOUBLES_ARE_FLOATS__)
#if defined(__ADSP21000__)
#define DIVF
#pragma linkage_name ___divsf3
float divf(float x, float y);
#endif
#endif

#if defined(__DOUBLES_ARE_FLOATS__)
#  define DBL_BIAS 127
/*largest exponent we'll get is 38 so only need to check exponent to the 6th bit */
#  define EXPONENT_BITS 6 
#  define MAX_EXPONENT 38
#else
#  define DBL_BIAS 1023
/*largest exponent we'll get is 308, so only need to check exponent to the 9th bit*/
#  define EXPONENT_BITS 9 
#  define MAX_EXPONENT 308
#endif

/* Powers of 10 lookup tables */
static const double _pvLookupTable[EXPONENT_BITS] = {
    1e1,  1e2 ,  1e4,
    1e8,  1e16,  1e32
#if !defined(__DOUBLES_ARE_FLOATS__)
   ,1e64, 1e128, 1e256
#endif
};

#if defined(__DOUBLES_ARE_FLOATS__)
  typedef union { double d; unsigned long i; } dblUnion;
#elif !defined(__ADSP21000__) /* we are a little endian machine  */
  typedef union { double d; struct {unsigned long low;unsigned long high; } i;} dblUnion;
#else /* we are a big endian machine */
  typedef union { double d; struct {unsigned long high;unsigned long low; } i;} dblUnion;
#endif

static __inline double
_normalise(double input, int *return_exponent)
{

/* Normalise a double into the range 1e6<=|result|<1e7 
**                      or sometimes 1e5<=|result|<1e7
**
** input = result*10^return_exponent
*/

  dblUnion in;
  int exponent,first_mult,second_mult=0;
  double power,tempVal;
  long i,lookupPos;

  in.d=input;

  /* First extract the exponent */
#if defined(__DOUBLES_ARE_FLOATS__)
  /* 32-bit version - extract bits 30 to 23 inclusive */
  exponent = (in.i & 0x7f800000) >> 23;
#else
  /* 64-bit version - extract bits 62 to 52 inclusive */
  exponent = (in.i.high & 0x7ff00000) >> 20;
#endif

  exponent = exponent - DBL_BIAS;

  /* if value is zero (exponent = -127 [-1023]) return exponent zero */
  if (exponent==-DBL_BIAS)
  {
    *return_exponent=-5; /* hack to make this function work with _print_float */
    return input;
  }

  exponent = (int)((double)0.30103 * (double) exponent);
  exponent-=6;

  /* we have a guess at an exponent, so go and do the normalisation */

  first_mult=abs(exponent);

  if (first_mult>MAX_EXPONENT){
    second_mult = first_mult >> 1;
    first_mult  = first_mult - second_mult;
  }

  for(i=1,lookupPos=0,power=1.0;lookupPos<EXPONENT_BITS;i<<=1,lookupPos++){
    if (first_mult&i)
      power=power*_pvLookupTable[lookupPos];
  }
    
  /* now we have calculated the power of 10 that we need to multiply the 
     number by to normalise it. */
  if (exponent<=0)
    tempVal=input*power;
  else
    tempVal=input/power;

  /* Do the second multiplication if appropriate */
  if (second_mult){
    for(i=1,lookupPos=0,power=1.0;lookupPos<EXPONENT_BITS;i<<=1,lookupPos++){
      if (second_mult&i)
        power=power*_pvLookupTable[lookupPos];
    }
    
    /* now we have calculated the power of 10 that we need to multiply the 
       number by to normalise it. */
    if  (exponent<=0)
      tempVal=tempVal*power;
    else
      tempVal=tempVal/power;
  }

  if (tempVal < 1e6){
    if (first_mult == 1) {
      tempVal = input;
      exponent--;
          /* some values that are within the range [1e6,1e7) will cause
          ** `power` to be 10.0, and hence will be scaled down into the
          ** range [1e5,1e6), and will lose us precision.
          */
    }
  }
  else if (tempVal >= 1e7){
    exponent++;
    tempVal/= 10.0;
  }

  input=tempVal;
  *return_exponent=exponent;
  return input;
}



/* CONV_DIGITS sets the number of digits that will be converted.  One of the
   digits is used only in rounding, then discarded. I chose to give one extra
   digit than the digits that are guaranteed to be accurate in a DOUBLE. */
#define CONV_DIGITS (4+DBL_DIG)

static const double _pwr10[10]={1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9};

/* Prints a float, defined by format.  func and farg are used by
   the routine that prints the resulting string 

   The name gets changed to _print_float_32 or _print_float_64 as appropriate
*/
int
_print_float(FormatT* format, FuncT func, FargT farg)
{
  dblUnion value;
  int norm_exponent;           /*the exponent of val after normalisation */
  int exponent;                /*the exponent of val if it had been normalised
                                 to 1<=|val|<10 */
  int fcode;                   /* copy of format->fcode */
  /* buf contains the (non-exponent) part of the converted number.
     buf size is CONV_DIGITS + 1 (decimal pt) + 1 ('\0' char) */
  char buf[CONV_DIGITS+2];
  /* expBuf contains the exponent.
     size is numchars+sign(1)+e/E(1)+'\0' = 5 or 6 */
#if defined(__DOUBLES_ARE_FLOATS__)
  char expBuf[5];
#else
  char expBuf[6];
#endif

  enum {f=0,e,E} printType=f; /* what is our format specifier to be?
                                 'f' types MUST BE ZERO */
  unsigned char dontDoConversion=0; /* if we decide we're NaN or Inf we skip loads of work */
  unsigned char trimZeroes=0; /* we need to trim the zeroes for %g */

  fcode = format->fcode;

#if defined(__DOUBLES_ARE_FLOATS__)
   if (format->modFlag == ModFlag_L) {
     /* Discard the 64-bit arg and print out the format specifier */
     char mp[3];
     FETCH(format, long double);
     format->buffer = mp;
     mp[0] = 'L';
     mp[1] = format->fcode;
     mp[2] = '\0';
     return( _do_output(format, func, farg) );
   }
#endif

/* FETCH is faster, but can't cope with 64-bit floats */
#if defined(__DOUBLES_ARE_FLOATS__) || defined(__ADSP21000__)
  /* fetch float value */
  value.d = FETCH(format, double);
#else
  /* fetch long double value */
  FETCH_LONG_DOUBLE(value.d,format);
#endif

  /* Print a '+' or a '-' or a ' ' or nothing as per our format flags.
  ** The -, + and space flags have their usual meanings even for Inf and NaN.
  */
#if defined(__DOUBLES_ARE_FLOATS__)
  _set_sign_prefix(format, (value.i & 0x80000000)!=0);
#else
  _set_sign_prefix(format, (value.i.high & 0x80000000)!=0);
#endif

  /* Firstly check to see if our number is +/-inf or +/-NaN, 
  ** and output this if it is 
  */

  /*NaN check.  If exponent is all 1s and mantissa isn't zero then we output NaN. */
  {
#if defined(__DOUBLES_ARE_FLOATS__)
    unsigned long m;
    unsigned long inf=0x7f800000;
    m=value.i & 0x7fffffff;
    if (m > inf)
#else
    unsigned long m=value.i.high&0x7ff00000;
    if ((m==0x7ff00000) && (value.i.low || (value.i.high & 0x000fffff)))
#endif
    {
#ifdef __ADSPBLACKFIN__
      if (fcode=='f' || fcode=='e' || fcode=='g')
        format->buffer="nan";
      else
        format->buffer="NAN";
#else
      format->buffer="NaN";
#endif
      dontDoConversion=1;
      /* The # and 0 flags have no effect on NaN and infinity printing 
      ** but they are not referenced again so the fields aren't explicitly 
      ** cleared.
      */
    }

    /* Not NaN - check for infinity.  Infinity is where all the bits in the exponent are set, and the mantissa is zero */
    else {
      /* Now lets print a '+' or a '-' or a ' ' or nothing as per our format spec*/
#if defined(__DOUBLES_ARE_FLOATS__)
      if (m==inf)
#else
      if (m==0x7ff00000)
#endif
      {
#ifdef __ADSPBLACKFIN__
        if (fcode=='f' || fcode=='e' || fcode=='g')
          format->buffer="inf";
        else
          format->buffer="INF";
#else
        format->buffer="Inf";
#endif
        dontDoConversion=1;
        /* The # and 0 flags have no effect on NaN and infinity printing 
        ** but they are not referenced again so the fields aren't explicitly 
        ** cleared.
        */
      }
    }
  }

  if (!dontDoConversion){
    int decPtPos;           /* the position of the decimal point (after
                            ** character x, i.e. if = 1, then 1.x is the
                            ** format, if=3, 123.x)                        */
    int intDigits;          /* number of digits to the left of the decimal
                            ** point in value.d after scaling              */
    int fractDigits;        /* utility variable - it is used to record the
                            ** number of digits generated that are to the
                            ** right of the decimal point
                            */            
    int numToConvert;
    int workPos;
    unsigned int firstUnusedPos=0; /* the position of the first unused element in the buffer buf */
    unsigned char firstTime;
    unsigned long workingInt,result;
    unsigned int remainder;

    buf[0]=0;
    expBuf[0]=0;

    if (format->precision < 0)
      format->precision = 6;

    /* make the value positive - we've set the output sign earlier. */
#if defined(__DOUBLES_ARE_FLOATS__)
    if (value.i & 0x80000000) value.i = value.i & 0x7fffffff;
#else
    if (value.i.high & 0x80000000) value.i.high = value.i.high & 0x7fffffff;
#endif

    /* Firstly normalise the number to between [1e6,1e7)
    **                        or maybe between [1e5,1e6)
    */
    value.d=_normalise(value.d,&norm_exponent);

    intDigits = (value.d < 1e6) ? 6 : 7;
              /* By default we come out of normalisation with
              ** 7 digits on the LHS of the double, but it may be 6
              */

    exponent=norm_exponent+(intDigits-1);

    /* Now, decide whether %g (or %G) is going to behave as %e or %f -and set
    a local variable to say which function we're using (0=%f, 1=%e 2=%E so we
    can do if (x) for all %e stuff */
    switch (fcode){
      case 'g':
      case 'G':
        /* so if the exponent is <-4 or >= the precision we do %e,
           otherwise %f  */
        if (format->precision == 0)
          format->precision = 1; /* according to the standard */

        if (exponent < -4 || exponent >= format->precision){
          printType=(fcode=='g')?e:E;
          /* For 'g', precision is the maximum number of significant
             digits, including the digit before the '.' */
          --format->precision;
        }
        else { /* we're going to print as 'f' precision */
          printType=f;
          /* For 'g', precision is the num of significant digits */
          format->precision=format->precision-exponent-1;
        }
        if (!format->bSharp)
          trimZeroes=1;
        break;
      case 'f':
      case 'F':
        printType = f;
        break;
      case 'e':
        printType = e;
        break;
      case 'E':
        printType = E;
        break;
    }

    /* Lets decode the number, and then put the decimal point in the
       appropriate place. First calculate numToConvert (the number
       of numbers to convert */

    /*NOTE THAT THE ROUNDING DIGIT IS ADDED AUTOMATICALLY LATER ON*/
    if (printType){ /* 'e' or 'E' */
      /* shrink numToConvert if precision says we don't need that many digits */
      if (CONV_DIGITS > (1 + format->precision) ){
        /* precision+1 for number before '.' */
        numToConvert=1+format->precision;
      }
      else
        numToConvert=CONV_DIGITS;
    } else { /* 'f' */

      /* If we are going to use %f formatting, then the precision specifies
      ** the number of digits required after the decimal point. So when the
      ** value is greater than (or equal to) 1.0 then the number of digits
      ** required is (exponent+1) digits before the decimal point and
      ** (precision) digits after the decimal point, that is:
      **
      **    (exponent+1) + precision
      **
      ** And when the value is less than 1.0, then the number of digits
      ** required is the precision. But suppose the value were 0.0012345
      ** and the precision were 5, then the value should be displayed as
      ** 0.00123 and so we will only need three digits from the value. So
      ** if the value is less than 1.0, then the actual number of digits
      ** required is (bearing in mind that the exponent of 0.1 is -1):
      **
      **    precision + exponent + 1
      **
      ** But we have to bear in mind that the size of the buffer used for
      ** accumulating the digits has a fixed size of CONV_DIGITS. So we
      ** calculate numToConvert as follows:
      */
      numToConvert = min (exponent + format->precision + 1,CONV_DIGITS);
    }

    /* So we have the value, and the number of digits we need. Convert the
       digits into the output buffer. */

    firstTime=1;
    /* remember numToConvert doesn't include the rounding digit (hence >=0) */
    while(numToConvert >= 0){
      /* convert in batches of no more than 7 as this
      ** is the maximum precision that a float can hold. */
      unsigned int convertThisTime;

      convertThisTime = (numToConvert < intDigits)
                      ? numToConvert + 1
                      : intDigits; /* generate an extra digit on the last time
                                   ** round which will be used for rounding */

      if (firstTime){
        if (convertThisTime < intDigits){

          /* scale value.d if we want less than `intDigits` of characters */

#if defined(DIVF)
          value.d = divf(value.d,_pwr10[intDigits-convertThisTime]);
#else
          value.d /= _pwr10[intDigits-convertThisTime];
#endif
        }
        firstTime=0;
        intDigits = 7; /* This is overloading the meaning of `intDigits` - on
                       ** the first time round this loop, it indicates the
                       ** scale of the output value (and will be set to either
                       ** 6 or 7) and hence we should generate no more than
                       ** this number of characters but on subsequent loops we
                       ** can generate up to 7 characters per loop if we
                       ** wanted to.
                       */
      }
      else
        value.d = value.d * _pwr10[convertThisTime];
                       /* will give us `convertThisTime` digits */

      workingInt = (unsigned long) value.d; /* truncates */
      numToConvert -= convertThisTime;
      firstUnusedPos += convertThisTime;
      workPos = firstUnusedPos - 1; /* array starts at zero */

#if defined(__ADSP21000__) && !defined(__DOUBLES_ARE_FLOATS__)
      value.d = value.d - (workingInt & 0xffff0000) - (workingInt & 0x0000ffff);
#else
      value.d = value.d - (double) workingInt;
#endif

      while (convertThisTime >0){
        convertThisTime--;
        result=workingInt/10;
        remainder=workingInt - (result*10);
        workingInt=result;

        /* so we have the remainder which we'll add to the buffer... */
        buf[workPos--] = '0'+remainder;
      }
    }

    /* So now we have a buffer with the numbers in it, and need to round
       the contents of the buffer. not so easy!

       What happens if we have '999'? We want '1000', but we need to chop
       the last digit off and increment exponent.

       Round up until the first character. If the first character rounds up and is over
       then make it one and increment the exponent. 
       
       firstUnusedPos includes the rounding character.
    */

    if (firstUnusedPos>0){
      workPos=firstUnusedPos-1;
      if (buf[workPos]>='5') {
        if (workPos==0){
          buf[workPos]='1';
          exponent++; /* we've rolled over the most significant digit, so increase the exponent */
          firstUnusedPos++; /* we didn't have any digits that were going to get printed before... */
        }
        else
          buf[--workPos]++;
      }

      while(buf[workPos]==('9'+1)){
        if (workPos==0) {
          buf[0]='1';
          exponent++; /* we've rolled over the most significant digit, so increase the exponent */
        }
        else {
          buf[workPos]='0';
          buf[--workPos]++;
        }
      }
      firstUnusedPos--; /* we remove the character we've rounded with */
    }

    /* trim the zeroes leaving at least one character in the buffer. */
    if (trimZeroes){
      while ((firstUnusedPos > 1) && (buf[firstUnusedPos-1] == '0'))
        firstUnusedPos--;
      if ((firstUnusedPos==1 && buf[0]=='0') || firstUnusedPos==0){
        format->buffer="0";
        return (_do_output(format, func, farg));
      }
    }

    /*Print the exponent for e types...*/
    if(printType){ /* 'e' || 'E' */
      unsigned int expBufPos=0;
      expBuf[expBufPos++]=(printType==e)?'e':'E';
      if (exponent>=0)
        expBuf[expBufPos++]='+';
      else
        expBuf[expBufPos++]='-';

      /* now extract the numbers... */
      exponent=abs(exponent);
      {
        int result;
        unsigned int remainder;

 #if !defined(__DOUBLES_ARE_FLOATS__)
        if (exponent>=100)
          expBufPos+=2;
        else
 #endif
        {
          expBuf[expBufPos]='0'; /* always have 2 zeroes */
          expBufPos+=1;
        }
        expBuf[expBufPos+1]='\0';
        if (exponent==0){
          expBuf[expBufPos] = '0';
        }
        else
          while(exponent>0){
            result=exponent/10;
            remainder=exponent-(result*10);
            exponent=result;
            /* so we have the remainder which we'll add to the buffer... */
            expBuf[expBufPos--] = '0'+remainder;
          }
      }
      format->suffix=expBuf;
    }

    /* Now add the decimal point in the appropriate place and also setup
       format to print zeroes in appropriate places */

    /* Calculate the position of the decimal point. The decimal point
    ** will go after the character at buf[decPtPos]
    */

    if (printType) { /* 'e' or 'E' */
        decPtPos = 1;
    } else {         /* 'f'        */

        decPtPos = 1 + exponent;

        /* decPtPos has the following significance: if it is zero, then
        ** the decimal point appears just in front of the digits just
        ** generated. (That is, the absolute value of the number output
        ** will be less than 1.0).
        **
        ** If decPtPos is -ve, then its absolute value indicates the
        ** number of zeroes required between the decimal point and the
        ** digits just generated. Again the absolute value of the number
        ** output will be less than 1.0.
        **
        ** Otherwise decPtPos will be +ve and counts the number of digits
        ** in front of the decimal point. It may be less than firstUnusedPos,
        ** in which case the decimal point appears somewhere in the middle of
        ** the digits generated; it may be exactly equal to firstUnusedPos,
        ** in which case the decimal point appears right at the end of the
        ** generated digits; or it may be greater than firstUnusedPos, in
        ** which case the expression (decPtPos - firstUnusedPos) will indicate
        ** how many trailing zeroes should be appended to the generated digits
        ** before adding the decimal point.
        **
        ** Examples:
        **
        **    Assume that the generated digits are "1234", then firstUnusedPos
        **    will be set to 4. Given this, then
        **
        **    decPtPos = -2 => the number was 0.001234
        **    decPtPos = -1 => the number was 0.01234
        **    decPtPos =  0 => the number was 0.1234
        **    decPtPos =  1 => the number was 1.234
        **    decPtPos =  4 => the number was 1234.
        **    decPtPos =  5 => the number was 12340.
        **    decPtPos =  6 => the number was 123400.
        */
    }

    if (((format->precision == 0) && (!format->bSharp)) ||
        ((trimZeroes) && (decPtPos >= (int) firstUnusedPos))) {

        /* If the precision is set to zero and the # flag has not been
        ** set (equivalent to !format->bSharp) then not only are no digits
        ** output after the decimal point but the decimal point itself is
        ** not output. Alternatively, for %g, the decimal point is only
        ** output if it is followed by a digit.
        **
        ** Note that the precision will never be set to zero for %g (or %G)
        ** because the Standard says that this is equivalent to setting the
        ** precision to 1.
        **
        ** We have the following three possible cases:
        **
        **    [1] 1234500. (a number with trailing zeroes + no fractional part)
        **    [2] 0.12345  (a number with no integral part)
        **    [3] 123.45   (a number with integral and fractional part)
        */

        if (decPtPos <= 0)
            format->buffer = "0";
        else {

            if (decPtPos <= firstUnusedPos)
                buf[decPtPos] = '\0';
            else {
                buf[firstUnusedPos] = '\0';
                format->rzero = decPtPos - firstUnusedPos;
            }
            format->buffer = buf;
        }
        return _do_output(format, func, farg);
    }

    /* The final number will now definitely require a decimal point - we
    ** will try to have it inserted implicitly but if the character appears
    ** in the middle of the digits we have generated, then we shall have to
    ** insert it explicitly:
    */

    if ((decPtPos > 0) && (decPtPos < firstUnusedPos))
    {

        /* Explicitly insert a decimal point into the digits generated
        **
        ** The output to be generated will be of the form:
        **     1.234
        **  or 123.4
        **
        ** but not: .1234 ('.' at the beginning)
        **      or: 1234. ('.' at the end)
        */

        fractDigits = firstUnusedPos - decPtPos;
  
        workPos = firstUnusedPos;
        do {
            buf[workPos] = buf[workPos-1];
            workPos--;
        } while (workPos != decPtPos);

        buf[decPtPos] = '.';
        firstUnusedPos++;

    } else {

        /* Implicitly insert a decimal point into the digits generated
        **
        ** The output to be generated will of the form:
        **     1234.    (no fractional part)
        **     1234000. (trailing zeroes and no fractional part)
        **     .1234    (no integral part)
        **     .001234  (leading zeroes in the fractional part)
        */

        if (decPtPos <= 0)
        {

            /* We are dealing with a fraction (no integral part) */

            format->lzero = 1;       /* a '.' is always preceded by a digit */
            format->odoFlag = kLeft; /* zeroes are reqd after decimal point */
            format->odo = -decPtPos; /* number of zeroes required           */

            if (format->odo >= format->precision)
                format->odo = format->precision;

            fractDigits = format->odo + firstUnusedPos;

        } else {

            /* We are dealing with a number with no fraction */

            format->odoFlag = kRight;
            format->odo = decPtPos - firstUnusedPos;
            fractDigits = 0;
        }
    }

    if (!trimZeroes)
        format->rzero = format->precision - fractDigits;

    buf[firstUnusedPos] = '\0';
    format->buffer = buf;
  }

  return _do_output(format, func, farg);

}
