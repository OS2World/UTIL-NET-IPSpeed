#if !(defined(_UTILITY_INCLUDED)) /* Include it only once */
#define _UTILITY_INCLUDED
/*---------------------------------------------------------------------------\
| Filename:     UTILITY.H                                                    |
| Filedate:     19.10.93                                                     |
| Version:      7.0                                                          |
| Author:       Michael Bock                                                 |
| Copyright:    (Public domain)                                              |
| Description:  This include file provides a lot of useful macros and types. |
|               It's purpose is to make C-programming easier and faster.     |
|               It's designed to work with all version of Microsoft-C from   |
|               4.0 to 7.0 (Borland-C requires only little changes).         |
| Comments:     Already defined constants or macros are not redefined by this|
|               include file. The only exceptions are FP_OFF and FP_SEG,     |
|               which are defined in <dos.h> but appear to be buggy. They    |
|               are redefined here with nonbuggy definitions.                |
|               Macros which evaluate their arguments more than once, and    |
|               can cause unexpected side-effects are marked with (SE!) in   |
|               the comment.                                                 |
| History:      With version 5.5, if the constant FORCE_NEAR is defined be-  |
|               fore including utility.h all pointer types become near by    |
|               default.                                                     |
|               Version 6.0 was prepared for 32-bit Compilers                |
|               Version 7.0 was rewritten to work with IBM C++ and GNU C++   |
\---------------------------------------------------------------------------*/

#if defined(__IBMC__)
/* Disable all warnings for this include file when using IBM's C/Set */
#pragma info(none)
#endif /* defined(__IBMC__) */

#if defined(__GNUC__)
#define __32BIT__
#endif /* defined(__GNUC__) */

#if (_MSC_VER >= 700)
/* Disable boring non-ANSI warning. The non-ANSI warning is trashing your
   screen if you use the macros defined here and compile with warninglevel 4.
*/

/* Disable boring conversion warnings
*/
#pragma warning(disable:4001)

/* Disable '__fastcall with stack-checking ...' warning. After reading once
   we don't need to see it again
*/
#pragma warning(disable:4135)

/* Disable local-function-not-called warning, produces many useless warnings
   with inline-functions defined in header-files
*/
#pragma warning(disable:4124)

#pragma warning(disable:4505)

#endif /* (_MSC_VER >= 700) */

/* Define NULL Pointer if necessary
*/
#if !(defined(NULL))

#if defined(__cplusplus) || defined(__32BIT__)
#define NULL    0
#elif (_MSC_VER >= 600)
#define NULL    ((void *)0)
#elif (defined(M_I86SM) || defined(M_I86MM))
#define NULL    0
#else
#define NULL    0L
#endif

#endif /* !(defined(NULL)) */


#if !(defined(__32BIT__))
/* Define _FAR and _NEAR
   Microsoft seems to add one underscore per release to all non-ANSI
   keywords and functions. So the constants _FAR and _NEAR are good
   replacements, because they maps to the version-specific keyword in
   all cases. For further releases, please add the required underscores.
*/

#if (_MSC_VER >= 700)
#define _FAR    __far
#define _NEAR   __near
#elif (_MSC_VER >= 600)
#define _FAR    _far
#define _NEAR   _near
#else
#define _FAR    far
#define _NEAR   near
#endif

#endif /* !(defined(__32BIT__)) */


/* Utility macros:
   plural(a)    - creates 's' char if a is greater than one and a zero char
                  otherwise.
   beep()       - beeps!
   wait()       - waits, until a key is pressed, but preserves key for later
                  reading. Does not clear the keybuffer before!
   isblanc(a)   - if a is a blanc, returns TRUE, else FALSE.
   symstr(a)    - returns a string contains symbol a. symstr makes no eva-
                  luations on it's argument. For examples a+3 with value of
                  a is 4 becomes "a+3" instead of "7" or "4+3".

   The use of this macros is obsolete and their definitions are here
   for compatibility purposes only.
*/
#define plural(a)   (((a)>1)*'s')
#define beep()      putch(7)
#define wait()      while ( ! kbhit() )
#define isblanc(a)  ((a) == ' ')
#define symstr(a)   #a


/* Mathematical macros:
   abs(a)   - absolute value of a ( all types )         // (SE!)
   max(a,b) - maximum of a and b ( all types )          // (SE!)
   min(a,b) - minimum of a and b ( all types )          // (SE!)
   trunc(a) - integer part of a, result type is int     // Obsolete
   intval(a)- integer part of a, result type is double  // Obsolete
   frac(a)  - fractional part of a, same sign           // Obsolete
   ln(a)    - natural logarithm ( for pascal compatibility )
   pred(a)  - a-1 ( for pascal compatibility )
   succ(a)  - a+1 ( for pascal compatibility )
   chr(a)   - a ( for pascal compatibility )
   ord(a)   - a ( for pascal compatibility )
   round(a) - nearest integer to a                      // Obsolete
   sgn(a)   - signum of a                               // (SE!)
   sqr(a)   - square of a                               // (SE!)
   cub(a)   - cubic of a                                // (SE!)
   odd(a)   - returns TRUE if a is odd and FALSE if a is even.
   even(a)  - returns TRUE if a is even and FALSE if a is odd.
   idiv(a,b)- returns the result of the unsigned integer division a/b. Instead
              of truncating, the result is rounded. Requires that a*2
              doesn't cause an overflow.
   sidiv(a,b)- Same as idiv, but works on signed integers // (SE!)
   PI       - Value of pi
   EULER_C  - Value of Euler's constant
   EXP1     - Value of e

   Arguments to macros should not produce side effects, because they may
   be avaluated more than once.
   abs, max and min are not defined when using C++, because they may
   interfere with some member functions.
   You can prevent the definition of all floating-point related macros and
   constants by defining the constant NOFLOAT before including this file.
*/

#if !(defined(__cplusplus))
#if !(defined(abs)) /* May be already defined */
#define abs(a)      (((a) < 0) ? (-(a)) : (a))
#endif
#if !(defined(min)) /* May be already defined */
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif
#if !(defined(max)) /* May be already defined */
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#else
#if (defined(abs)) /* May be already defined */
#undef abs
#endif
template <class _TYPE>
inline _TYPE abs(_TYPE a) {
    return a < 0 ? -a : a;
    }
#if (defined(min)) /* May be already defined */
#undef min
#endif
template <class _TYPE>
inline _TYPE min(_TYPE a, _TYPE b) {
    return a < b ? a : b;
    }
#if (defined(max)) /* May be already defined */
#undef max
#endif
template <class _TYPE>
inline _TYPE max(_TYPE a, _TYPE b) {
    return a > b ? a : b;
    }
#endif /* !(defined(__cplusplus)) */

#define pred(a)     ((a)-1)
#define succ(a)     ((a)+1)
#define chr(a)      (unsigned char)(a)
#define ord(a)      (int)(a)
#define sgn(a)      ((a) > 0) ? 1 : ( ((a) < 0) ? -1 : 0 )
#define sqr(a)      ((a) * (a))
#define cub(a)      ((a) * (a) * (a))
#define odd(a)      (((a) & 1) ? (TRUE) : (FALSE))
#define even(a)     (((a) & 1) ? (FALSE) : (TRUE))
#define idiv(a,b)   ((((a) * 2) / (b) + 1) / 2)
#define sidiv(a,b)  (((a) >= 0) ? ((((a) * 2) / (b) + 1) / 2) : ((((a) * 2) / (b) - 1) / 2))

#if !(defined(NOFLOAT))
#define ln(a)       log(a)
#define arctan(a)   atan(a)
#define PI          (3.14159265358979323851L)
#define EULER_C     (0.577215664901532860L)
#define EXP1        (2.71828182845904523543L)
#endif /* !(defined(NOFLOAT)) */


/* Additional program control flow statements:
   loop     - provides an infinite loop.
   _loop    - synonymous to loop (see below).

   endloop(c) -
        if c is true, current loop ends. Because the endcondition
        may occur anywhere in a loop-statement, this is an extension
        to the standard loop statements, which test end-conditions
        only at the front or the end of a loop body.
        Designed for use with 'loop', but works proper with any other
        loop statement ( while, do, for ).
        endloop always works on the nearest loop statement, so the
        following will not work as you might believe:

      loop {
              ...
              while ( ... ) {
                  ...
                  endloop ( ... )
                  ...
                  }
              ...
              }

        endloop will not terminate the loop body in this case, but
        rather the body of the while statement.

   stoploop - same as an unconditional endloop.

   repeat ...
   until(c) - reverses interpretation of endcondition in a 'do...while(c)'
        loop. Allowes pascal-style loops.

   elif(c)  - substitution for an 'else if(c)' combination. May clear in-
        terpretation of program. For example:

          if ( a )
          {
              ...
          }
          elif ( b )
          {
              ...
          }
          elif ( c )
          {
              ...
          }

    var_unused(v) - Declares a local variable or formal parameter to be
        unused in a function's body. This is only a trick to prevent the
        compiler from issueing a warning, the variable can still be used.
        (No trick helps us with MSC7, a "statement without affect"-warning
         is produces instead)

    Note that the loop statement causes confusion if loop is used as a
    mnemonic with the inline assembler. In this case add an '#undef loop'
    statement to your source file after including utility.h. You still can
    use _loop in this case.
*/

#define loop            while ( 1 )
#define _loop           while ( 1 )
#define endloop(c)      if (c) break
#define stoploop        break
#define repeat          do
#define until(c)        while ( !(c) )
#define elif(c)         else if (c)
#define var_unused(v)   (void)(v)


/* Three simple macros which define pointer types:

   DEFP(MyType); is equivalent to                // (SE!)
   typedef MyType *PMyType;
   typedef PMyType *PPMyType;

   DEFNP(MyType); is equivalent to               // (SE!)
   typedef MyType _NEAR *PMyType;
   typedef PMyType _NEAR *PPMyType;

   DEFFP(MyType); is equivalent to               // (SE!)
   typedef MyType _FAR *PMyType;
   typedef PMyType _FAR *PPMyType;
*/

#define DEFP(type) \
    typedef type * P##type; \
    typedef P##type * PP##type

#if !(defined(__32BIT__))

#define DEFNP(type) \
    typedef type _NEAR * P##type; \
    typedef P##type _NEAR * PP##type
#define DEFFP(type) \
    typedef type _FAR * P##type; \
    typedef P##type _FAR * PP##type

#endif /* !(defined(__32BIT__)) */

/* Two more complex macros which defines the type and it's pointers:
   DEFTYPE(MyType, {                         // (SE!)
        type1 field1;
        type2 field2;
        ...
        }); is equivalent to

   typedef struct _MyType {
        type1 field1;
        type2 field2;
        ...
        } MyType;
   typedef MyType _FAR *PMyType;
   typedef PMyType _FAR *PPMyType;

   You can use 'struct _MyType' to reference the type within it's
   definition.

   DEFUTYPE(MyType, {                        // (SE!)
        type1 field1;
        type2 field2;
        ...
        }); is equivalent to

   typedef union _MyType {
        type1 field1;
        type2 field2;
        ...
        } MyType;
   typedef MyType _FAR *PMyType;
   typedef PMyType _FAR *PPMyType;

   You can use 'union _MyType' to reference the type within it's
   definition.

   Note! When using FORCE_NEAR the associated pointer types are near
*/

#if defined(__32BIT__)

#define DEFTYPE(type, strucdef) \
    typedef struct _##type strucdef type; \
    DEFP(type)
#define DEFUTYPE(type, uniondef) \
    typedef union _##type uniondef type; \
    DEFP(type)

#elif defined(FORCE_NEAR)

#define DEFTYPE(type, strucdef) \
    typedef struct _##type strucdef type; \
    DEFNP(type)
#define DEFUTYPE(type, uniondef) \
    typedef union _##type uniondef type; \
    DEFNP(type)

#else

#define DEFTYPE(type, strucdef) \
    typedef struct _##type strucdef type; \
    DEFFP(type)
#define DEFUTYPE(type, uniondef) \
    typedef union _##type uniondef type; \
    DEFFP(type)

#endif


/* Additional types for assembler-like programming.
   They save a lot of keystrokes.
   In addition the new type 'bool' is defined here, which also
   defines TRUE and FALSE.
*/
#if !(defined(NO_SHORT_TYPES))
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
#endif /* !(defined(NO_SHORT_TYPES)) */

#if !(defined(TRUE)) && !(defined(FALSE))
typedef enum {
    FALSE = 0,
    TRUE = 1
    } bool;
#endif

#if !(defined(__32BIT__))
#if (_MSC_VER < 600)
typedef word _segment;
#elif (_MSC_VER >= 700)
#define _segment __segment
#endif
#endif /* !(defined(__32BIT__)) */


/* Additional far pointer types for generic use.
   When using FORCE_NEAR, these become near pointer types.
*/
#if defined(__32BIT__)

typedef byte *pbyte;
typedef char *pchar;
typedef word *pword;
typedef int *pint;
typedef dword *pdword;
typedef long *plong;
#if !(defined(TRUE)) && !(defined(FALSE))
typedef bool *pbool;
#endif
typedef void *pvoid;

typedef pbyte *ppbyte;
typedef pchar *ppchar;
typedef pword *ppword;
typedef pint *ppint;
typedef pdword *ppdword;
typedef plong *pplong;
#if !(defined(TRUE)) && !(defined(FALSE))
typedef pbool *ppbool;
#endif
typedef pvoid *ppvoid;

#elif defined(FORCE_NEAR)

typedef byte _NEAR *pbyte;
typedef char _NEAR *pchar;
typedef word _NEAR *pword;
typedef int _NEAR *pint;
typedef dword _NEAR *pdword;
typedef long _NEAR *plong;
#if !(defined(TRUE)) && !(defined(FALSE))
typedef bool _NEAR *pbool;
#endif
typedef void _NEAR *pvoid;
typedef _segment _NEAR *p_segment;

typedef pbyte _NEAR *ppbyte;
typedef pchar _NEAR *ppchar;
typedef pword _NEAR *ppword;
typedef pint _NEAR *ppint;
typedef pdword _NEAR *ppdword;
typedef plong _NEAR *pplong;
#if !(defined(TRUE)) && !(defined(FALSE))
typedef pbool _NEAR *ppbool;
#endif
typedef pvoid _NEAR *ppvoid;
typedef p_segment _NEAR *pp_segment;

#else

typedef byte _FAR *pbyte;
typedef char _FAR *pchar;
typedef word _FAR *pword;
typedef int _FAR *pint;
typedef dword _FAR *pdword;
typedef long _FAR *plong;
#if !(defined(TRUE)) && !(defined(FALSE))
typedef bool _FAR *pbool;
#endif
typedef void _FAR *pvoid;
typedef _segment _FAR *p_segment;

typedef pbyte _FAR *ppbyte;
typedef pchar _FAR *ppchar;
typedef pword _FAR *ppword;
typedef pint _FAR *ppint;
typedef pdword _FAR *ppdword;
typedef plong _FAR *pplong;
#if !(defined(TRUE)) && !(defined(FALSE))
typedef pbool _FAR *ppbool;
#endif
typedef pvoid _FAR *ppvoid;
typedef p_segment _FAR *pp_segment;

#endif


/* Macros to extract parts of a variable:
   _loww(d)     - get lower word of a long or unsigned long variable.
   _highw(d)    - get higher word of a long or unsigned long variable.
   _lowb(d)     - get lower byte of a int or unsigned int variable.
   _highb(d)    - get higher byte of a int or unsigned int variable.
   _bdword(h,l) - builds an unsigned long with h as the higher word an
                  l as the lower word.
   _bulong(h,l) - same as _bdword(h,l).
   _blong(h,l)  - same as _bdword(h,l) but return type is 'long' rather
                  than 'dword'
   _bword(h,l)  - builds an unsigned int with h as the higher byte an
                  l as the lower byte.
   _buint(h,l)  - same as _bword(h,l).
   _bint(h,l)   - same as _bword(h,l) but return type is 'int' rather
                  than 'word'

   These macros do not rely on the internal ordering of bytes and
   therefore should work with every C-Compiler on every target computer.
*/

#define _loww(d)        (word)((dword)(d) & 0xffffL)
#define _highw(d)       (word)((dword)(d) >> 16)
#define _lowb(w)        (byte)((word)(w) & 0xff)
#define _highb(w)       (byte)((word)(w) >> 8)
#define _bdword(h,l)    (((dword)(h) << 16) | (dword)(l))
#define _bulong(h,l)    _bdword(h,l)
#define _blong(h,l)     ((long)_bdword(h,l))
#define _bword(h,l)     (((word)(h) << 8) | (word)(l))
#define _buint(h,l)     _bword(h,l)
#define _bint(h,l)      ((int)_bword(h,l))



/* Macros for additional bitwise operations:
*/
#define _nand(a,b)      (~((a)&(b)))
#define _nor(a,b)       (~((a)|(b)))
#define _nxor(a,b)      (~((a)^(b)))
#define _impl(a,b)      ((~(a))|((a)&(b)))      /* (SE!) */


#if defined(_INCL_UTI_BITS)
/* Macros to perform bit operations:
   _bit(a,n)    - return value ( 0 or 1 ) of n(th) bit in a. a must be a legal
                  lvalue of an integral type or a structure/union type of any
                  size. Does not work with arrays ( but with structures con-
                  taining arrays. Very fast, if n is a constant! // (SE!)
   _setbit(a,n,v) - set n(th) bit to v. a and n have the same limitations as for
                  _bit. A nonzero v(alue) sets the bit, a zero v(alue) clears it.
                  Returns no usable value and must not be used in expressions. // (SE!)
   _lbit(a,n)     - Produces an lvalue representing the n(th) bit of a. Can
                  used in assignments or expressions. a must be a lvalue,
                  n must be a constant containing only digits ( for example,
                  '1+2' would illegal ), and a's size may not be greater than
                  4 bytes. */
#define _bit(a,n) (((pword)&(a))[(n)>>4]&(1<<((n)&0x0f)))
#define _setbit(a,n,v)  ( (v) ? \
  (((pword)&(a))[(n)>>4]|=(1<<((n)&0x0f)))  \
  (((pword)&(a))[(n)>>4]&= ~(1<<((n)&0x0f)))
#pragma pack(1)
struct __bitsn {    /* Structure for use with _lbit */
    unsigned b0 : 1;
    unsigned b1 : 1;
    unsigned b2 : 1;
    unsigned b3 : 1;
    unsigned b4 : 1;
    unsigned b5 : 1;
    unsigned b6 : 1;
    unsigned b7 : 1;
    unsigned b8 : 1;
    unsigned b9 : 1;
    unsigned b10 : 1;
    unsigned b11 : 1;
    unsigned b12 : 1;
    unsigned b13 : 1;
    unsigned b14 : 1;
    unsigned b15 : 1;
    unsigned b16 : 1;
    unsigned b17 : 1;
    unsigned b18 : 1;
    unsigned b19 : 1;
    unsigned b20 : 1;
    unsigned b21 : 1;
    unsigned b22 : 1;
    unsigned b23 : 1;
    unsigned b24 : 1;
    unsigned b25 : 1;
    unsigned b26 : 1;
    unsigned b27 : 1;
    unsigned b28 : 1;
    unsigned b29 : 1;
    unsigned b30 : 1;
    unsigned b31 : 1;
    };
#pragma pack()
#define _lbit(a,n)  (((struct __bitsn _FAR *)(&a))->b##n)
#endif /* defined(_INCL_UTI_BITS) */


/* Cast any pointer to a far pointer to a function with return type
   'ret_type' and 'args' as arguments, use with extreme care!
*/
#if !(defined(__32BIT__))
#if !(defined(__cplusplus)) && !(_MSC_VER >= 700)
#define inline(ret_type,func,args)   ((ret_type (_FAR *)args)func)
#define call(ret_type,func,args)     ((ret_type (_FAR *)args)(void _FAR *)func)
#endif
#endif /* !(if defined(__32BIT__)) */


/* Macro to generate a debug breakpoint instruction (int 3)
   works with MSC 6.00 and higher
*/
#if (_MSC_VER >= 700)
#define DEBUGBREAK()    { __asm int 3 }
#elif (_MSC_VER >= 600)
#define DEBUGBREAK()    { _asm int 3 }
#endif


#if !(defined(__32BIT__))

/* Macro to build a far pointer from segment ( seg ) and offset ( ofs ).
   Because the resulting pointer points to void, use type casting to
   get any other pointer type
*/

#define MK_FP(seg,ofs)  ((pvoid)_bdword(seg, ofs))

#if defined(FP_SEG)     /* definition in <dos.h> is buggy!!! */
#undef FP_SEG
#endif
#define FP_SEG(pointer) ((_segment)_highw((pvoid)(pointer)))

#if defined(FP_OFF)     /* definition in <dos.h> is buggy!!! */
#undef  FP_OFF
#endif
#define FP_OFF(pointer) _loww((pvoid)(pointer))


/* Because FP_SEG and FP_OFF don't require their argument to be a lvalue,
   they don't return a lvalue. So expressions like FP_SEG(p) = .. are not
   valid. The following macros SEG_OF ( or SEL_OF ) and OFFSET_OF can be
   used for this purpose, but they require a lvalue, which must be a far(!)
   pointer, as their argument. Using SEG_OF with near pointers produces
   undefined results.
*/

#define SEG_OF(pointer) (((p_segment)&(pointer))[1])
#define SEL_OF(pointer) SEG_OF(pointer)

#define OFFSET_OF(pointer) (((pword)&(pointer))[0])

#endif /* !(if defined(__32BIT__)) */


/* PDIF calculates the difference between two pointers in bytes regardless
   of the type. The segment parts of far pointers are ignored.
*/

#if defined(__32BIT__)
#define PDIF(pointer1,pointer2) ((int)(pointer2)-(int)(pointer1))
#else
#define PDIF(pointer1,pointer2) (FP_OFF(pointer2)-FP_OFF(pointer1))
#endif


/* FIELD_OFFSET calculates a structure field's offset in bytes from the
   beginning of the whole structure. VARFIELD_OFFSET does the same job, but
   takes a structure variable as first argument rather than a type name.
*/

#if defined(__32BIT__)
#define FIELD_OFFSET(type, field) ((dword)&(((type *)0)->field))
#define VARFIELD_OFFSET(var, field) PDIF(&(var), &((var)->field))
#else
#define FIELD_OFFSET(type, field) ((word)&((type _NEAR *0)->field))
#define VARFIELD_OFFSET(var, field) PDIF(&(var), &((var)->field))
#endif


/* ARRAY_SIZE returns the number of elements of an array. For multidimensional
   arrays, the size of the leftmost dimension is returned (a[10][3][2] -> 10).
   The parameter must be the name of an previously declared array.  // (SE!)
*/

#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

/* INCR_P increments a pointer by n. Only the offsetpart of the pointer
   is affected. The incremented pointer is returned.        // (SE!)
*/

#if defined(__32BIT__)
#define INCR_P(pointer, n)  ((int)(pointer) += (n), (pointer))
#else
#define INCR_P(pointer, n)  (OFFSET_OF(pointer) += (n), (pointer))
#endif

/* NORMALIZE_P normalizes a far pointer by adjust the segment and offset
   part of the pointer. After normalisation, the offset part is always
   less than 0x10. Normalized pointer can be easily compared and are useful
   to start large arrays. The normalized pointer is returned.  // (SE!)
*/

#if defined(__32BIT__)
#define NORMALIZE_P(pointer) (pointer)
#else
#define NORMALIZE_P(pointer) \
    (MK_FP((word)FP_SEG(pointer) + (FP_OFF(pointer) >> 4), \
          FP_OFF(pointer) & 0x000F), (pointer))
#endif


/* Macro to get the linear address of a far pointer for comparisions.
   Doesn't work in protected mode!          // (SE!)
*/
#if !defined(__32BIT__)
#define ADRV(pointer)   (((dword)FP_SEG(pointer) << 4) + FP_OFF(pointer))
#endif


/* Macro to get the whole amount of program's memory in paragraphs. May be
   used for '_dos_keep' to make a program full resident. The '_psp' variable
   must be declared for this macro.
*/

/* Obsolete:
extern unsigned int _psp;
#define MEM_PARA    (*(unsigned int _FAR *)MK_FP(_psp,2) - _psp)
*/


/* Macros to read or store at any addresses in memory:
   _peek(s,o)   - returns the byte the address s(egment):o(ffset).
   _peekb(s,o)  - same as _peek(s,o).
   _peekw(s,o)  - returns the word the address s(egment):o(ffset).
   _peekd(s,o)  - returns the double word the address s(egment):o(ffset).
   _poke(s,o,v) - sets the byte the address s(egment):o(ffset) to value v.
   _pokeb(s,o,v)- same as _poke(s,o,v).
   _pokew(s,o,v)- sets the word the address s(egment):o(ffset) to value v.
   _pokev(s,o,v)- sets the double word the address s(egment):o(ffset) to
                  value v.
   _storage(t,s,o)- produces an lvalue with address s(egment):o(ffset) and
                  t(ype). May be used in any assignments or expressions.
   the _poke?-macros return the value v if used in expressions */
#if !defined(__32BIT__)

#define _peek(s,o)      (*((pbyte)MK_FP(s,o)))
#define _peekb(s,o)     _peek(s,o)
#define _peekw(s,o)     (*((pword)MK_FP(s,o)))
#define _peekd(s,o)     (*((pdword)MK_FP(s,o)))
#define _poke(s,o,v)    ((*((pbyte)MK_FP(s,o))) = (byte)(v))
#define _pokeb(s,o,v)   _poke(s,o,v)
#define _pokew(s,o,v)   ((*((pword)MK_FP(s,o))) = (word)(v))
#define _poked(s,o,v)   ((*((pdword)MK_FP(s,o))) = (dword)(v))
#define _storage(t,s,o) (*((t _FAR *)MK_FP(s,o)))

#endif

#if !defined(__32BIT__)
#if defined(__cplusplus)

/* Now some C++ specific adjustments:
   The ostream operator<< does not accept far strings and far void pointers
   in small or medium data models and the _segment type in any model,
   so let us define them here:
*/

#if defined(_INC_IOSTREAM)
inline ostream& operator<<(ostream& os, _segment seg)
{
    os << (unsigned short)seg;

    return os;
}

#if (defined(M_I86SM) || defined(M_I86MM))
inline ostream& operator<<(ostream& os, char __far *str)
{
    while ( *str )
        os << *str++;

    return os;
}

inline ostream& operator<<(ostream& os, void __far *p)
{
    long previous_flags = os.flags(ios::hex);
    os << '0' << 'x' << FP_SEG(p) << ':' << FP_OFF(p);
    os.flags(previous_flags);
    return os;
}
#endif
#endif
#endif /* __cplusplus */
#endif /* !defined(__32BIT__) */

#if defined(FORCE_NEAR)
#undef FORCE_NEAR   /* We don't need it anymore */
#endif

#if defined(__IBMC__)
/* Restore warnings when using IBM's C/Set */
#pragma info(restore)
#endif /* defined(__IBMC__) */

#endif /* _UTILITY_INCLUDED */ 
