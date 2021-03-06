/* Copyright (C) 2000-2010 Analog Devices Inc., All Rights Reserved,
** This contains Analog Devices Background IP and Development IP as
** defined in the ADI/Intel Collaboration Agreement.
*/

#pragma file_attr("libGroup=runtime_checking")
#pragma file_attr("libName=libc")
#pragma file_attr("libFunc=__unassigned_pointer_check")
#pragma file_attr("libFunc=___unassigned_pointer_check")
#pragma file_attr("libFunc=__unassigned_check")
#pragma file_attr("libFunc=___unassigned_check")
#pragma file_attr("libFunc=__pointer_check")
#pragma file_attr("libFunc=___pointer_check")
#pragma file_attr("libFunc=__array_bound_check")
#pragma file_attr("libFunc=___array_bound_check")
#pragma file_attr("libFunc=__shift_check")
#pragma file_attr("libFunc=___shift_check")
#pragma file_attr("libFunc=__idiv_by_zero")
#pragma file_attr("libFunc=___idiv_by_zero")
#pragma file_attr("libFunc=__lldiv_by_zero")
#pragma file_attr("libFunc=___lldiv_by_zero")
#pragma file_attr("libFunc=__fdiv_by_zero")
#pragma file_attr("libFunc=___fdiv_by_zero")
#pragma file_attr("libFunc=__ddiv_by_zero")
#pragma file_attr("libFunc=___ddiv_by_zero")
#pragma file_attr("libFunc=__rt_array_register")
#pragma file_attr("libFunc=___rt_array_register")
#pragma file_attr("libFunc=__rt_array_index_pop")
#pragma file_attr("libFunc=___rt_array_index_pop")
#pragma file_attr("libFunc=__rule_17_1")
#pragma file_attr("libFunc=___rule_17_1")
#pragma file_attr("libFunc=__rule_17_2")
#pragma file_attr("libFunc=___rule_17_2")
#pragma file_attr("libFunc=__rule_17_3")
#pragma file_attr("libFunc=___rule_17_3")
#pragma file_attr("prefersMem=any")
#pragma file_attr("prefersMemNum=50")

#ifdef _MISRA_RULES
#pragma diag(suppress:misra_rule_6_3:"Use basic types")
#pragma diag(suppress:misra_rule_10_1_a:"Checking unassigned pattern clashes unsigned/signed")
#pragma diag(suppress:misra_rule_10_3:"Need to cast different integral types")
#pragma diag(suppress:misra_rule_11_3:"Need to cast pointers and integers")
#pragma diag(suppress:misra_rule_11_4:"Need to cast different pointers")
#pragma diag(suppress:misra_rule_11_5:"Need to cast away constness for unassigned checking")
#pragma diag(suppress:misra_rule_12_12:"Unassigned checking looks at float representation")
#pragma diag(suppress:misra_rule_16_2:"Recursion happens when runtime checks apply to runtime checking functions")
#pragma diag(suppress:misra_rule_17_4:"Pointer arithmetic required")
#pragma diag(suppress:misra_rule_20_4:"Need dynamic memory allocated")
#pragma diag(suppress:misra_rule_20_9:"Allow errors to be printed")
#pragma diag(suppress:misra_rule_20_11:"Allow exit to be used")
#endif /* _MISRA_RULES */

#include <stdio.h>
#include <stdlib.h>

/* specs not generated by compiler */
void __rt_array_register(void *p, int size);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Runtime array bounds checking support."
#if defined(__ADSP21000__)
%regs_clobbered "ALLScratch,LoopDepth1"
#endif
#endif
void __rt_array_index_pop(int sub_index);

#ifdef __SYSCALL_INFO__
%syscall
%notes "Runtime recursion checking support."
#if defined(__ADSP21000__)
%regs_clobbered "ALLScratch,LoopDepth1"
#endif
#endif
void __rt_recursion(const char *func_name,
                    const char *file,
                    int line_number);

static struct arrinfo {void *low; void *high;} *arrs;
#define INDEX_CHUNK 100
static int index;

#define TRUE 1
#define FALSE 0

void __unassigned_check(const void *val,
                        int size,
                        const char *file,
                        int line_number,
                        int is_misra)
/*
  check the contents of a variable at location val and of size size
  to see if it represents the unassigned pattern and raise an error
  if that is the case.
  We only support 32-bit and 64-bit unassigned checking.
  is_misra if true indicates MISRA mode.
*/
{
  int error = FALSE;
  char *rule = is_misra ? "MISRA Rule 21.1 (Req): " : "";

  switch (size) {
#if defined(__ADSPBLACKFIN__)
    case 4:
#elif defined(__ADSP21000__)
    case 1:
#else
#error unimplemented
#endif
       if ((*(int*)val) == 0x81818181) {
         error = TRUE; 
       }
       break;
#if defined(__ADSPBLACKFIN__)
    case 8:
#elif defined(__ADSP21000__)
    case 2:
#else
#error unimplemented
#endif
       if (((*(int*)val) == 0x81818181) &&
                       ((*((int*)val+1)) == 0x81818181)) {
         error = TRUE; 
       }
       break;
   
    default:
       break;
  }
  if (error == TRUE) {
    fprintf(stderr, "\"%s\", line %d: cc4000: error:\n%s"
                    "Unassigned variable\n",
                    file, line_number, rule);
    exit(EXIT_FAILURE);
  }
} /* unassigned_check */

void __unassigned_pointer_check(const void *val,
                                int size,
                                const char *file,
                                int line_number,
                                int is_misra)
/*
  val is the contents of a pointer and the contents are checked to see
  if the pointer is local and unassigned. If it is return an error.
  If not check the location pointed to in order to detect an unassigned
  variable of size size is not being referenced.
  is_misra if true indicates MISRA mode.
*/
{
  char *rule = is_misra ? "MISRA Rule 21.1 (Req): " : "";

  if ((int)val == 0x81818181) {
    fprintf(stderr, "\"%s\", line %d: cc4000: error:\n%s"
                    "Unassigned variable\n",
                    file, line_number, rule);
    exit(EXIT_FAILURE);
  }
  /* now check for location pointed at */
 __unassigned_check(val,size,file,line_number,is_misra);
} /* __unassigned_pointer_check */

void __pointer_check(const void *val,
                     int size,
                     const char *file,
                     int line_number,
                     int is_misra)
/*
   check whether pointer value that is about to be derefernced
   contains the NULL pointer.
   Note: size is redundant but keep for backwards compatibility issue
*/
{
  char *rule = is_misra ? "MISRA Rule 21.1 (Req): " : "";
  if (val == NULL) {
    fprintf(stderr, "\"%s\", line %d: cc4001: error:\n%s"
                    "Attempt to dereference NULL pointer\n",
                    file, line_number, rule);
    exit(EXIT_FAILURE);
  }
} /* __pointer_check */

void __array_bound_check(int val,
                         int size,
                         const char *file,
                         int line_number,
                         int is_misra)
/*
  size indicates the size of an array and val is the current index.
*/
{
  char *rule = is_misra ? "MISRA Rule 21.1 (Req): " : "";

  if ((val < 0) || (val >= size)) {
    fprintf(stderr, "\"%s\", line %d: cc4002: error:\n%s"
                    "Array bound fault: index by %d in array size of %d\n",
                    file, line_number, rule, val, size);
    exit(EXIT_FAILURE);
  }
} /* __array_bound_check */

void __shift_check(int shift_val,
                   int width,
                   const char *file,
                   int line_number,
                   int is_misra)
/*
   width indicates the size of the object being shifted and val the current
   shift value.
*/
{
  char *rule = is_misra ? "MISRA Rule 12.8 (Req): " : "";

  if ((shift_val >= width) || (shift_val < 0)) {
    fprintf(stderr,"\"%s\", line %d: cc4003: error:\n"
                   "%sThe right-hand operand of a shift "
                   "operator shall lie between zero and one less than the width"
                   " of bits of the underlying type of the left-hand operand.\n"
                   "Shift of %d on operand of width %d bits.\n",
                   file, line_number, rule, shift_val, width);
    exit(EXIT_FAILURE);
  }
} /* __shift_check */


void __idiv_by_zero(int val,
                    const char *file,
                    int line_number,
                    int is_misra)
/*
   val contains the value of an integer divisor
*/
{
  char *rule = is_misra ? " MISRA Rule 21.1 (Req): " : "";

  if (val == 0) {
    fprintf(stderr, "\"%s\", line %d: cc4004: error:\n%s"
                    "Integer division by zero\n",
                    file, line_number, rule);
    exit(EXIT_FAILURE);
  }
} /* __idiv_by_zero */

void __lldiv_by_zero(long long val,
                    const char *file,
                    int line_number,
                    int is_misra)
{
  char *rule = is_misra ? " MISRA Rule 21.1 (Req): " : "";

  if (val == 0) {
    fprintf(stderr,"\"%s\", line %d: cc4004: error:\n%s"
                   "Integer division by zero\n",
                   file,line_number, rule);
    exit(EXIT_FAILURE);
  }
}

void __fdiv_by_zero(float val,
                    const char *file,
                    int line_number,
                    int is_misra)
/*
   val contains the value of a floating-point (32-bit) divisor
*/
{
  char *rule = is_misra ? "MISRA Rule 21.1 (Req): " : "";
  int *p = (int*)&val;

  if (p[0] == 0) {
    fprintf(stderr, "\"%s\", line %d: cc4005: error:\n%s"
                    "Floating-point division by zero\n",
                    file, line_number, rule);
    exit(EXIT_FAILURE);
  }
} /* __fdiv_by_zero */

void __ddiv_by_zero(long double val,
                    const char *file,
                    int line_number,
                    int is_misra)
/*
   val contains the value of a floating-point (64-bit) divisor
*/
{
  char *rule = is_misra ? "MISRA Rule 21.1 (Req): " : "";
  int *p = (int*)&val;

  if ((p[0] == 0) && (p[1] == 0)) {
    fprintf(stderr, "\"%s\", line %d: cc4006: error:\n%s"
                    "Floating-point division by zero\n",
                    file, line_number, rule);
    exit(EXIT_FAILURE);
  }
} /* __ddiv_by_zero */


void __rt_array_register(void *p, int size)
/*
  When arrays are declared this function notes its start address
  and end address using p and size.
  index contains the location of a specific array in file scope or
  current block.
*/
{
  static int max_index = INDEX_CHUNK;

  if (arrs == NULL) {
    arrs = (struct arrinfo*)malloc(max_index * sizeof(struct arrinfo));
  } else if (index == max_index) {
    max_index += INDEX_CHUNK;
    arrs = (struct arrinfo*)realloc(arrs, max_index * sizeof(struct arrinfo));
  } else {
    ; /* do nothing */
  }
  if (arrs == NULL) {
    fprintf(stderr, "No space to do runtime checking "
                    "on rules 17.1, 17.2 and 17.3 (suppress checking!)\n");
    abort();
  }
  if (p < (void*)0) {
    arrs[index].high = p;
    arrs[index].low = (void *)((int)p + size);
  } else {
    arrs[index].low = p;
    arrs[index].high = (void *)((int)p + size);
  }
  index++;
} /* __rt_array_register */

void __rt_array_index_pop(int sub_index)
/*
   as we exit scope throw away information about local arrays
   that are no longer needed to check.
*/
{
  index -= sub_index;
} /* __rt_array_index_pop */

void __rule_17_1(void *p, const char *file, int line_number, int is_misra) 
/*
  uses information from __rt_array_register setup to determine if
  pointer p refers to a declared array.
*/
{
  int i;
  char *rule = is_misra ? "MISRA Rule 17.1 (Req): " : "";
  int fail = 1;

   if (index != 0) {
     for (i = index-1; i>=0; i--) {
       if ((p >= arrs[i].low) && (p < arrs[i].high)) {
         fail = 0;
         break;
       }
     }
   }
   if (fail == 1) {
     fprintf(stderr, "\"%s\", line %d: cc4007: error:\n%s"
                     "Pointer arithmetic shall only be "
                     "applied to pointers\nthat address an array or "
                     "array_element.\nPointer value 0X%X\n",
                     file, line_number, rule, p);
   }
} /* __rule_17_1 */

void __rule_17_2(void *p1,
                 void *p2,
                 const char *file,
                 int line_number,
                 int is_misra) 
/*
  uses information from __rt_array_register setup to determine if
  pointer p1 and p2 refer to the same declared array before performing
  a pointer diff.
*/
{
  int i;
  char *rule = is_misra ? "MISRA Rule 17.2 (Req): " : "";
  int fail = 1;

   if (p1 != p2) {
     if (index != 0) {
       for (i = index-1; i>=0; i--) {
         if (((p1 >= arrs[i].low) && (p1 <= arrs[i].high)) &&
             ((p2 >= arrs[i].low) && (p2 <= arrs[i].high))) {
            fail = 0;
            break;
         }
       }
     }
     if (fail == 1) {
       fprintf(stderr, "\"%s\", line %d: cc4008: error:\n%s"
                       "Pointer subtraction shall only be "
                       "applied to pointers that address elements of the same " 
                       "array.\nPointer values 0X%X and 0X%X\n",
                       file, line_number, rule, p1, p2);
     }
   }
} /* __rule_17_2 */

void __rule_17_3(void *p1,
                 void *p2,
                 const char *file,
                 int line_number,
                 int is_misra) 
/*
  uses information from __rt_array_register setup to determine if
  pointer p1 and p2 refer to the same declared array before performing
  a compare.
*/
{
  int i;
  char *rule = is_misra ? "MISRA Rule 17.3 (Req): " : "";
  int fail = 1;

   if (p1 != p2) {
     if (index != 0) {
       for (i = index-1; i>=0; i--) {
         if (((p1 >= arrs[i].low) && (p1 <= arrs[i].high)) &&
             ((p2 >= arrs[i].low) && (p2 <= arrs[i].high))) {
           fail = 0;
           break;
         }
       }
     }
     if (fail == 1) {
       fprintf(stderr, "\"%s\", line %d: cc4009: error:\n%s"
                       ">, >=, <, <= shall not be applied "
                       "to pointer types except where they point to the same "
                       "array.\nPointer Values 0X%X and 0X%X\n",
                       file, line_number, rule, p1, p2);
     }
   }
} /* __rule_17_3 */

void __rt_recursion(const char *func_name,
                    const char *file,
                    int line_number)
/*
   Checks to see if function func_name already on call-stack.
   Note this mechanism does not work when threads are used.
*/
{
  static const char *seen_file = NULL;
  static int seen_line_number = 0;
  char *rule = "MISRA Rule 16.2 (Req): ";

   if (seen_line_number == 0) {
     seen_line_number = line_number;
     seen_file = file;
   } else if ((seen_line_number == line_number) && (seen_file == file)) {
     exit(EXIT_FAILURE); /* recursion already reported,
                         ** avoid possible infinite running */
   } else {
     ; /* do nothing */
   }
   fprintf(stderr, "\"%s\", line %d: cc4010: error:\n%s"
                   "Functions shall not call themselves, "
                   "either directly or indirectly.\n"
                   "Function %s called recursively.\n",
                   file, line_number, rule, func_name);
     
} /* __rt_recursion */

