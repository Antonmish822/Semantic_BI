/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         RAPTOR_PARSEDATE_STYPE
/* Substitute the variable and function names.  */
#define yyparse         raptor_parsedate_parse
#define yylex           raptor_parsedate_lex
#define yyerror         raptor_parsedate_error
#define yydebug         raptor_parsedate_debug
#define yynerrs         raptor_parsedate_nerrs

/* First part of user prologue.  */
#line 1 "./parsedate.y"

/*
 * Imported from the public domain source in PHP 4.4
 *   Fri May 20 07:14:01 2005
 *   https://github.com/php/php-src/blob/379c4af44aa6456fbdbc69dba3ead166ba7ff67d/ext/standard/parsedate.y
 *
 * and patched from there
 *
 * Later versions removed this from PHP and replaced it with entirely
 * new code written under the PHP license. That code is not used here
 * and cannot be used.
 *
 */


/*
**  Originally written by Steven M. Bellovin <smb@research.att.com> while
**  at the University of North Carolina at Chapel Hill.  Later tweaked by
**  a couple of people on Usenet.  Completely overhauled by Rich $alz
**  <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990.
**
**  This code is in the public domain and has no copyright.
*/


#ifdef HAVE_CONFIG_H
#include <raptor_config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if defined(_HPUX_SOURCE)
#include <alloca.h>
#endif

#include "raptor2.h"
#include "raptor_internal.h"


#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#if !defined(__GNUC__) || __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
# define __attribute__(x)
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

/* Some old versions of bison generate parsers that use bcopy.
   That loses on systems that don't provide the function, so we have
   to redefine it here.  */
#if !defined (HAVE_BCOPY) && defined (HAVE_MEMCPY) && !defined (bcopy)
# define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif

/* Prototypes */ 
static int raptor_parsedate_error(void* parm, const char *msg);


#define EPOCH		1970
#define HOUR(x)		((x) * 60)

#define MAX_BUFF_LEN    128   /* size of buffer to read the date into */

/*
**  An entry in the lexical lookup table.
*/
typedef struct _TABLE {
    const char	*name;
    int		type;
    int		value;
} TABLE;


/*
**  Meridian:  am, pm, or 24-hour style.
*/
typedef enum _MERIDIAN {
    MERam, MERpm, MER24
} MERIDIAN;

struct date_yy {
	const char	*yyInput;
	int	yyDayOrdinal;
	int	yyDayNumber;
	int	yyHaveDate;
	int	yyHaveDay;
	int	yyHaveRel;
	int	yyHaveTime;
	int	yyHaveZone;
	int	yyTimezone;
	int	yyDay;
	int	yyHour;
	int	yyMinutes;
	int	yyMonth;
	int	yySeconds;
	int	yyYear;
	MERIDIAN	yyMeridian;
	int	yyRelDay;
	int	yyRelHour;
	int	yyRelMinutes;
	int	yyRelMonth;
	int	yyRelSeconds;
	int	yyRelYear;
};

typedef union _date_ll {
    int			Number;
    enum _MERIDIAN	Meridian;
} date_ll;

#define YYPARSE_PARAM parm
#define YYLEX_PARAM parm
#define YYSTYPE date_ll
#define YYLTYPE void

static int yylex (YYSTYPE *lvalp, void *parm);

static int ToHour (int Hours, MERIDIAN Meridian);
static int ToYear (int Year);
static int LookupWord (YYSTYPE *lvalp, char *buff);


#line 219 "parsedate.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parsedate.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_tAGO = 3,                       /* tAGO  */
  YYSYMBOL_tDAY = 4,                       /* tDAY  */
  YYSYMBOL_tDAY_UNIT = 5,                  /* tDAY_UNIT  */
  YYSYMBOL_tDAYZONE = 6,                   /* tDAYZONE  */
  YYSYMBOL_tDST = 7,                       /* tDST  */
  YYSYMBOL_tHOUR_UNIT = 8,                 /* tHOUR_UNIT  */
  YYSYMBOL_tID = 9,                        /* tID  */
  YYSYMBOL_tTZONE = 10,                    /* tTZONE  */
  YYSYMBOL_tWZONE = 11,                    /* tWZONE  */
  YYSYMBOL_tZZONE = 12,                    /* tZZONE  */
  YYSYMBOL_tMERIDIAN = 13,                 /* tMERIDIAN  */
  YYSYMBOL_tMINUTE_UNIT = 14,              /* tMINUTE_UNIT  */
  YYSYMBOL_tMONTH = 15,                    /* tMONTH  */
  YYSYMBOL_tMONTH_UNIT = 16,               /* tMONTH_UNIT  */
  YYSYMBOL_tSEC_UNIT = 17,                 /* tSEC_UNIT  */
  YYSYMBOL_tSNUMBER = 18,                  /* tSNUMBER  */
  YYSYMBOL_tUNUMBER = 19,                  /* tUNUMBER  */
  YYSYMBOL_tYEAR_UNIT = 20,                /* tYEAR_UNIT  */
  YYSYMBOL_tZONE = 21,                     /* tZONE  */
  YYSYMBOL_22_ = 22,                       /* '.'  */
  YYSYMBOL_23_ = 23,                       /* ':'  */
  YYSYMBOL_24_ = 24,                       /* ','  */
  YYSYMBOL_25_ = 25,                       /* '/'  */
  YYSYMBOL_YYACCEPT = 26,                  /* $accept  */
  YYSYMBOL_spec = 27,                      /* spec  */
  YYSYMBOL_item = 28,                      /* item  */
  YYSYMBOL_time = 29,                      /* time  */
  YYSYMBOL_iso8601time_colon = 30,         /* iso8601time_colon  */
  YYSYMBOL_iso8601zonepart = 31,           /* iso8601zonepart  */
  YYSYMBOL_sec_fraction_part = 32,         /* sec_fraction_part  */
  YYSYMBOL_zonepart_numeric_without_colon = 33, /* zonepart_numeric_without_colon  */
  YYSYMBOL_zonepart_numeric_with_colon = 34, /* zonepart_numeric_with_colon  */
  YYSYMBOL_HMStime_with_colon = 35,        /* HMStime_with_colon  */
  YYSYMBOL_HMtime_with_colon = 36,         /* HMtime_with_colon  */
  YYSYMBOL_zone = 37,                      /* zone  */
  YYSYMBOL_day = 38,                       /* day  */
  YYSYMBOL_date = 39,                      /* date  */
  YYSYMBOL_iso8601datetime = 40,           /* iso8601datetime  */
  YYSYMBOL_iso8601date = 41,               /* iso8601date  */
  YYSYMBOL_iso8601weekspec = 42,           /* iso8601weekspec  */
  YYSYMBOL_iso8601time = 43,               /* iso8601time  */
  YYSYMBOL_rel = 44,                       /* rel  */
  YYSYMBOL_relunit = 45,                   /* relunit  */
  YYSYMBOL_number = 46,                    /* number  */
  YYSYMBOL_o_merid = 47                    /* o_merid  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined RAPTOR_PARSEDATE_STYPE_IS_TRIVIAL && RAPTOR_PARSEDATE_STYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   110

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  26
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  22
/* YYNRULES -- Number of rules.  */
#define YYNRULES  77
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  100

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   276


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    24,     2,    22,    25,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    23,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21
};

#if RAPTOR_PARSEDATE_DEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   177,   177,   178,   181,   184,   187,   190,   193,   196,
     197,   200,   206,   210,   213,   217,   220,   226,   229,   232,
     235,   238,   240,   243,   253,   259,   265,   281,   284,   287,
     290,   293,   296,   301,   305,   309,   315,   319,   330,   348,
     349,   352,   358,   363,   371,   376,   384,   391,   392,   411,
     417,   423,   435,   438,   444,   445,   470,   484,   487,   490,
     493,   496,   499,   502,   505,   508,   511,   514,   517,   520,
     523,   526,   529,   532,   535,   538,   543,   578
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if RAPTOR_PARSEDATE_DEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "tAGO", "tDAY",
  "tDAY_UNIT", "tDAYZONE", "tDST", "tHOUR_UNIT", "tID", "tTZONE", "tWZONE",
  "tZZONE", "tMERIDIAN", "tMINUTE_UNIT", "tMONTH", "tMONTH_UNIT",
  "tSEC_UNIT", "tSNUMBER", "tUNUMBER", "tYEAR_UNIT", "tZONE", "'.'", "':'",
  "','", "'/'", "$accept", "spec", "item", "time", "iso8601time_colon",
  "iso8601zonepart", "sec_fraction_part", "zonepart_numeric_without_colon",
  "zonepart_numeric_with_colon", "HMStime_with_colon", "HMtime_with_colon",
  "zone", "day", "date", "iso8601datetime", "iso8601date",
  "iso8601weekspec", "iso8601time", "rel", "relunit", "number", "o_merid", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-60)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
     -60,     2,   -60,   -13,   -60,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,    13,   -60,   -60,    69,    20,   -60,    32,   -60,
     -60,   -60,    29,     4,   -60,   -60,   -60,   -60,    44,   -60,
      58,   -60,   -60,   -60,   -15,   -60,   -60,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,    46,    48,   -60,   -60,    28,   -60,
     -60,    37,   -60,    56,    57,   -60,   -60,    59,    52,    61,
      52,    46,   -60,    64,    62,    26,   -60,   -60,    66,   -60,
     -60,   -60,   -60,    68,   -60,    36,    74,   -60,   -60,   -60,
     -60,   -60,   -60,   -60,   -60,   -60,    76,   -60,    86,   -60,
      80,    81,    78,    79,   -60,   -60,   -60,    84,    87,   -60
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     1,    33,    66,    31,    69,    27,    28,    29,
      77,    72,     0,    63,    75,     0,    76,    60,    30,     3,
       4,    12,    22,    22,     5,     7,     6,    40,    39,     8,
      57,     9,    10,    34,    43,    65,    68,    71,    62,    74,
      59,    35,    64,    67,     0,     0,    11,    70,    45,    61,
      73,    50,    58,     0,     0,    51,    32,     0,    20,     0,
      20,     0,    56,    42,     0,    22,    54,    48,    52,    41,
      46,    49,    26,    36,    21,    23,     0,    15,    17,    18,
      19,    13,    25,    16,    14,    47,     0,    44,    20,    53,
       0,     0,     0,    23,    55,    38,    24,     0,     0,    37
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -60,   -60,   -60,   -60,   104,   -59,   -23,   -60,   -60,   -60,
     -60,   107,   -60,   -60,   -60,   -60,   -60,    49,   -55,   -60,
     -60,   -60
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     1,    19,    20,    66,    77,    58,    78,    79,    22,
      23,    80,    25,    26,    27,    28,    55,    67,    29,    30,
      31,    32
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      60,    83,     2,    81,    63,    84,     3,     4,     5,    64,
       6,    33,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    41,    42,    57,    59,    43,    94,
      44,    45,    34,    46,    47,    48,    49,    50,    51,    56,
      52,    35,    88,    53,    36,    54,    69,    70,    57,    53,
      37,    57,    38,    39,    61,    71,    40,     4,     5,    91,
       6,    62,     7,     8,     9,    65,    11,    68,    13,    14,
      75,    76,    17,    18,    35,    72,    73,    36,    74,    42,
      82,    87,    43,    37,    89,    38,    39,    86,    47,    40,
      49,    50,     5,    90,    52,    92,     7,     8,     9,    95,
      96,    97,    91,    98,    93,    21,    99,    18,    24,     0,
      85
};

static const yytype_int8 yycheck[] =
{
      23,    60,     0,    58,    19,    60,     4,     5,     6,    24,
       8,    24,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,     4,     5,    22,    23,     8,    88,
      10,    11,    19,    13,    14,    15,    16,    17,    18,     7,
      20,     5,    65,    23,     8,    25,    18,    19,    22,    23,
      14,    22,    16,    17,    10,    18,    20,     5,     6,    23,
       8,     3,    10,    11,    12,    19,    14,    19,    16,    17,
      18,    19,    20,    21,     5,    19,    19,     8,    19,     5,
      19,    19,     8,    14,    18,    16,    17,    23,    14,    20,
      16,    17,     6,    25,    20,    19,    10,    11,    12,    19,
      19,    23,    23,    19,    18,     1,    19,    21,     1,    -1,
      61
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    27,     0,     4,     5,     6,     8,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    28,
      29,    30,    35,    36,    37,    38,    39,    40,    41,    44,
      45,    46,    47,    24,    19,     5,     8,    14,    16,    17,
      20,     4,     5,     8,    10,    11,    13,    14,    15,    16,
      17,    18,    20,    23,    25,    42,     7,    22,    32,    23,
      32,    10,     3,    19,    24,    19,    30,    43,    19,    18,
      19,    18,    19,    19,    19,    18,    19,    31,    33,    34,
      37,    44,    19,    31,    44,    43,    23,    19,    32,    18,
      25,    23,    19,    18,    31,    19,    19,    23,    19,    19
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    26,    27,    27,    28,    28,    28,    28,    28,    28,
      28,    29,    29,    30,    30,    30,    30,    31,    31,    31,
      31,    32,    32,    33,    34,    35,    36,    37,    37,    37,
      37,    37,    37,    38,    38,    38,    39,    39,    39,    39,
      39,    39,    39,    39,    39,    39,    39,    40,    40,    41,
      41,    41,    42,    42,    43,    43,    44,    44,    45,    45,
      45,    45,    45,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    45,    45,    46,    47
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     2,     1,     3,     3,     3,     3,     1,     1,     1,
       0,     2,     0,     1,     3,     3,     3,     1,     1,     1,
       1,     1,     2,     1,     2,     2,     3,     8,     5,     1,
       1,     3,     3,     2,     4,     2,     3,     3,     3,     3,
       2,     2,     2,     3,     1,     3,     2,     1,     2,     2,
       1,     2,     2,     1,     2,     2,     1,     2,     2,     1,
       2,     2,     1,     2,     2,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = RAPTOR_PARSEDATE_EMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == RAPTOR_PARSEDATE_EMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (parm, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use RAPTOR_PARSEDATE_error or RAPTOR_PARSEDATE_UNDEF. */
#define YYERRCODE RAPTOR_PARSEDATE_UNDEF


/* Enable debugging if requested.  */
#if RAPTOR_PARSEDATE_DEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, parm); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct date_yy *parm)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (parm);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct date_yy *parm)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, parm);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, struct date_yy *parm)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], parm);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, parm); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !RAPTOR_PARSEDATE_DEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !RAPTOR_PARSEDATE_DEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, struct date_yy *parm)
{
  YY_USE (yyvaluep);
  YY_USE (parm);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct date_yy *parm)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = RAPTOR_PARSEDATE_EMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == RAPTOR_PARSEDATE_EMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, parm);
    }

  if (yychar <= RAPTOR_PARSEDATE_EOF)
    {
      yychar = RAPTOR_PARSEDATE_EOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == RAPTOR_PARSEDATE_error)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = RAPTOR_PARSEDATE_UNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = RAPTOR_PARSEDATE_EMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4: /* item: time  */
#line 181 "./parsedate.y"
               {
	    ((struct date_yy *)parm)->yyHaveTime++;
	}
#line 1328 "parsedate.c"
    break;

  case 5: /* item: zone  */
#line 184 "./parsedate.y"
               {
	        ((struct date_yy *)parm)->yyHaveZone++;
	}
#line 1336 "parsedate.c"
    break;

  case 6: /* item: date  */
#line 187 "./parsedate.y"
               {
	    ((struct date_yy *)parm)->yyHaveDate++;
	}
#line 1344 "parsedate.c"
    break;

  case 7: /* item: day  */
#line 190 "./parsedate.y"
              {
	    ((struct date_yy *)parm)->yyHaveDay++;
	}
#line 1352 "parsedate.c"
    break;

  case 8: /* item: rel  */
#line 193 "./parsedate.y"
              {
	    ((struct date_yy *)parm)->yyHaveRel++;
	}
#line 1360 "parsedate.c"
    break;

  case 11: /* time: tUNUMBER tMERIDIAN  */
#line 200 "./parsedate.y"
                             {
	    ((struct date_yy *)parm)->yyHour = (yyvsp[-1].Number);
	    ((struct date_yy *)parm)->yyMinutes = 0;
	    ((struct date_yy *)parm)->yySeconds = 0;
	    ((struct date_yy *)parm)->yyMeridian = (yyvsp[0].Meridian);
	}
#line 1371 "parsedate.c"
    break;

  case 13: /* iso8601time_colon: HMStime_with_colon sec_fraction_part rel  */
#line 210 "./parsedate.y"
                                                            {
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	}
#line 1379 "parsedate.c"
    break;

  case 14: /* iso8601time_colon: HMtime_with_colon sec_fraction_part rel  */
#line 213 "./parsedate.y"
                                                  {
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	    ((struct date_yy *)parm)->yySeconds = 0;
	}
#line 1388 "parsedate.c"
    break;

  case 15: /* iso8601time_colon: HMStime_with_colon sec_fraction_part iso8601zonepart  */
#line 217 "./parsedate.y"
                                                               {
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	}
#line 1396 "parsedate.c"
    break;

  case 16: /* iso8601time_colon: HMtime_with_colon sec_fraction_part iso8601zonepart  */
#line 220 "./parsedate.y"
                                                              {
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	    ((struct date_yy *)parm)->yySeconds = 0;
	}
#line 1405 "parsedate.c"
    break;

  case 17: /* iso8601zonepart: zonepart_numeric_without_colon  */
#line 226 "./parsedate.y"
                                                {
		((struct date_yy *)parm)->yyHaveZone++;
	}
#line 1413 "parsedate.c"
    break;

  case 18: /* iso8601zonepart: zonepart_numeric_with_colon  */
#line 229 "./parsedate.y"
                                      {
		((struct date_yy *)parm)->yyHaveZone++;
	}
#line 1421 "parsedate.c"
    break;

  case 19: /* iso8601zonepart: zone  */
#line 232 "./parsedate.y"
               {
		((struct date_yy *)parm)->yyHaveZone++;
	}
#line 1429 "parsedate.c"
    break;

  case 21: /* sec_fraction_part: '.' tUNUMBER  */
#line 238 "./parsedate.y"
                                {
	}
#line 1436 "parsedate.c"
    break;

  case 23: /* zonepart_numeric_without_colon: tSNUMBER  */
#line 243 "./parsedate.y"
                                         {
		/* format: [+-]hhmm */
		if((yyvsp[0].Number) <= -100 || (yyvsp[0].Number) >= 100) {
			((struct date_yy *)parm)->yyTimezone = (-(yyvsp[0].Number) / 100) * 60 + (-(yyvsp[0].Number) % 100);
		} else if((yyvsp[0].Number) >= -99 && (yyvsp[0].Number) <= 99) {
			((struct date_yy *)parm)->yyTimezone = -(yyvsp[0].Number) * 60;
		}
	}
#line 1449 "parsedate.c"
    break;

  case 24: /* zonepart_numeric_with_colon: tSNUMBER ':' tUNUMBER  */
#line 253 "./parsedate.y"
                                                   {
		/* format: [+-]hh:mm */
		((struct date_yy *)parm)->yyTimezone = -(yyvsp[-2].Number) * 60 + ((yyvsp[-2].Number) > 0 ? -(yyvsp[0].Number): (yyvsp[0].Number));
	}
#line 1458 "parsedate.c"
    break;

  case 25: /* HMStime_with_colon: HMtime_with_colon ':' tUNUMBER  */
#line 259 "./parsedate.y"
                                                   {
		/* format: hh:mm:ss */
	    ((struct date_yy *)parm)->yySeconds = (yyvsp[0].Number);
	}
#line 1467 "parsedate.c"
    break;

  case 26: /* HMtime_with_colon: tUNUMBER ':' tUNUMBER  */
#line 265 "./parsedate.y"
                                         {
		/* format: hh:mm */
	    ((struct date_yy *)parm)->yyHour = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyMinutes = (yyvsp[0].Number);
	}
#line 1477 "parsedate.c"
    break;

  case 27: /* zone: tTZONE  */
#line 281 "./parsedate.y"
                 {
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[0].Number);
	}
#line 1485 "parsedate.c"
    break;

  case 28: /* zone: tWZONE  */
#line 284 "./parsedate.y"
                 {
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[0].Number);
	}
#line 1493 "parsedate.c"
    break;

  case 29: /* zone: tZZONE  */
#line 287 "./parsedate.y"
                 {
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[0].Number);
	}
#line 1501 "parsedate.c"
    break;

  case 30: /* zone: tZONE  */
#line 290 "./parsedate.y"
                {
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[0].Number);
	}
#line 1509 "parsedate.c"
    break;

  case 31: /* zone: tDAYZONE  */
#line 293 "./parsedate.y"
                   {
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[0].Number) - 60;
	}
#line 1517 "parsedate.c"
    break;

  case 32: /* zone: tZONE tDST  */
#line 296 "./parsedate.y"
                     {
	    ((struct date_yy *)parm)->yyTimezone = (yyvsp[-1].Number) - 60;
	}
#line 1525 "parsedate.c"
    break;

  case 33: /* day: tDAY  */
#line 301 "./parsedate.y"
               {
	    ((struct date_yy *)parm)->yyDayOrdinal = 1;
	    ((struct date_yy *)parm)->yyDayNumber = (yyvsp[0].Number);
	}
#line 1534 "parsedate.c"
    break;

  case 34: /* day: tDAY ','  */
#line 305 "./parsedate.y"
                   {
	    ((struct date_yy *)parm)->yyDayOrdinal = 1;
	    ((struct date_yy *)parm)->yyDayNumber = (yyvsp[-1].Number);
	}
#line 1543 "parsedate.c"
    break;

  case 35: /* day: tUNUMBER tDAY  */
#line 309 "./parsedate.y"
                        {
	    ((struct date_yy *)parm)->yyDayOrdinal = (yyvsp[-1].Number);
	    ((struct date_yy *)parm)->yyDayNumber = (yyvsp[0].Number);
	}
#line 1552 "parsedate.c"
    break;

  case 36: /* date: tUNUMBER '/' tUNUMBER  */
#line 315 "./parsedate.y"
                                {
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyDay = (yyvsp[0].Number);
	}
#line 1561 "parsedate.c"
    break;

  case 37: /* date: tMONTH tUNUMBER tUNUMBER ':' tUNUMBER ':' tUNUMBER tUNUMBER  */
#line 319 "./parsedate.y"
                                                                      {
		((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
		((struct date_yy *)parm)->yyMonth = (yyvsp[-7].Number);
		((struct date_yy *)parm)->yyDay = (yyvsp[-6].Number);

		((struct date_yy *)parm)->yyHour = (yyvsp[-5].Number);
		((struct date_yy *)parm)->yyMinutes = (yyvsp[-3].Number);
		((struct date_yy *)parm)->yySeconds = (yyvsp[-1].Number);

		((struct date_yy *)parm)->yyHaveTime = 1;
	}
#line 1577 "parsedate.c"
    break;

  case 38: /* date: tUNUMBER '/' tUNUMBER '/' tUNUMBER  */
#line 330 "./parsedate.y"
                                             {
	  /* Interpret as YYYY/MM/DD if $1 >= 1000, otherwise as MM/DD/YY.
	     The goal in recognizing YYYY/MM/DD is solely to support legacy
	     machine-generated dates like those in an RCS log listing.  If
	     you want portability, use the ISO 8601 format.  */
	  if((yyvsp[-4].Number) >= 1000)
	    {
	      ((struct date_yy *)parm)->yyYear = (yyvsp[-4].Number);
	      ((struct date_yy *)parm)->yyMonth = (yyvsp[-2].Number);
	      ((struct date_yy *)parm)->yyDay = (yyvsp[0].Number);
	    }
	  else
	    {
	      ((struct date_yy *)parm)->yyMonth = (yyvsp[-4].Number);
	      ((struct date_yy *)parm)->yyDay = (yyvsp[-2].Number);
	      ((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
	    }
	}
#line 1600 "parsedate.c"
    break;

  case 40: /* date: iso8601datetime  */
#line 349 "./parsedate.y"
                          {
			((struct date_yy *)parm)->yyHaveTime++;
    }
#line 1608 "parsedate.c"
    break;

  case 41: /* date: tUNUMBER tMONTH tSNUMBER  */
#line 352 "./parsedate.y"
                                   {
	    /* e.g. 17-JUN-1992.  */
	    ((struct date_yy *)parm)->yyDay = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[-1].Number);
	    ((struct date_yy *)parm)->yyYear = -(yyvsp[0].Number);
	}
#line 1619 "parsedate.c"
    break;

  case 42: /* date: tMONTH tUNUMBER tUNUMBER  */
#line 358 "./parsedate.y"
                                   {
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyDay = (yyvsp[-1].Number);
		((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
	}
#line 1629 "parsedate.c"
    break;

  case 43: /* date: tMONTH tUNUMBER  */
#line 363 "./parsedate.y"
                          {
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[-1].Number);
	    if((yyvsp[0].Number) > 1000) {
		((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
	    } else {
		((struct date_yy *)parm)->yyDay = (yyvsp[0].Number);
	    }
	}
#line 1642 "parsedate.c"
    break;

  case 44: /* date: tMONTH tUNUMBER ',' tUNUMBER  */
#line 371 "./parsedate.y"
                                       {
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[-3].Number);
	    ((struct date_yy *)parm)->yyDay = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
	}
#line 1652 "parsedate.c"
    break;

  case 45: /* date: tUNUMBER tMONTH  */
#line 376 "./parsedate.y"
                          {
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[0].Number);
	    if((yyvsp[-1].Number) > 1000) {
		((struct date_yy *)parm)->yyYear = (yyvsp[-1].Number);
	    } else {
		((struct date_yy *)parm)->yyDay = (yyvsp[-1].Number);
	    }
	}
#line 1665 "parsedate.c"
    break;

  case 46: /* date: tUNUMBER tMONTH tUNUMBER  */
#line 384 "./parsedate.y"
                                   {
	    ((struct date_yy *)parm)->yyMonth = (yyvsp[-1].Number);
	    ((struct date_yy *)parm)->yyDay = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
	}
#line 1675 "parsedate.c"
    break;

  case 48: /* iso8601datetime: tUNUMBER tTZONE iso8601time  */
#line 392 "./parsedate.y"
                                      {
		int i = (yyvsp[-2].Number);

		if(i >= 10000) {
			/* format: yyyymmdd */
			((struct date_yy *)parm)->yyYear = i / 10000;
			i %= 10000;
			((struct date_yy *)parm)->yyMonth = i / 100;
			i %= 100;
			((struct date_yy *)parm)->yyDay = i;
		} else if(i >= 1000 && i <= 9999) {
			/* format: yyyy */
			((struct date_yy *)parm)->yyYear = i;
			((struct date_yy *)parm)->yyDay= 1;
			((struct date_yy *)parm)->yyMonth = 1;
		}
	}
#line 1697 "parsedate.c"
    break;

  case 49: /* iso8601date: tUNUMBER tSNUMBER tSNUMBER  */
#line 411 "./parsedate.y"
                                        {
	    /* ISO 8601 format.  yyyy-mm-dd.  */
	    ((struct date_yy *)parm)->yyYear = (yyvsp[-2].Number);
	    ((struct date_yy *)parm)->yyMonth = -(yyvsp[-1].Number);
	    ((struct date_yy *)parm)->yyDay = -(yyvsp[0].Number);
	}
#line 1708 "parsedate.c"
    break;

  case 50: /* iso8601date: tUNUMBER tSNUMBER  */
#line 417 "./parsedate.y"
                            {
		/* ISO 8601 format   yyyy-mm */
	    ((struct date_yy *)parm)->yyYear = (yyvsp[-1].Number);
	    ((struct date_yy *)parm)->yyMonth = -(yyvsp[0].Number);
	    ((struct date_yy *)parm)->yyDay = 1;
	}
#line 1719 "parsedate.c"
    break;

  case 51: /* iso8601date: tUNUMBER iso8601weekspec  */
#line 423 "./parsedate.y"
                                   {
		const int om = (1 + 9) % 12; /* offset month */
		const int oy = (yyvsp[-1].Number) - 1; /* offset year */

		((struct date_yy *)parm)->yyYear = (yyvsp[-1].Number);
		((struct date_yy *)parm)->yyMonth = 1;
		/* Zeller's formula */
		((struct date_yy *)parm)->yyDay -= ((13 * om + 12) / 5 +
					oy + oy / 4 + oy / 400 - oy / 100) % 7 - 1;
	}
#line 1734 "parsedate.c"
    break;

  case 52: /* iso8601weekspec: tWZONE tUNUMBER  */
#line 435 "./parsedate.y"
                                 {
		((struct date_yy *)parm)->yyDay = ((yyvsp[0].Number) / 10) * 7 + ((yyvsp[0].Number) % 10) - 8;
	}
#line 1742 "parsedate.c"
    break;

  case 53: /* iso8601weekspec: tWZONE tUNUMBER tSNUMBER  */
#line 438 "./parsedate.y"
                                   {
		((struct date_yy *)parm)->yyDay = (yyvsp[-1].Number) * 7 - (yyvsp[0].Number) - 8;
	}
#line 1750 "parsedate.c"
    break;

  case 55: /* iso8601time: tUNUMBER sec_fraction_part iso8601zonepart  */
#line 445 "./parsedate.y"
                                                     {
		int i = (yyvsp[-2].Number);

		if(i <= -100000 || i >= 100000) {
			((struct date_yy *)parm)->yyHour = i / 10000;
			i %= 10000;
			((struct date_yy *)parm)->yyMinutes = i / 100;
			i %= 100;
	    	((struct date_yy *)parm)->yySeconds = i;
		} else if(i <= -1000 || i >= 1000) {
			((struct date_yy *)parm)->yyHour = i / 100;
			i %= 100;
			((struct date_yy *)parm)->yyMinutes = i;
	    	((struct date_yy *)parm)->yySeconds = 0;
		} else if(i >= -99 && i <= 99) {
			((struct date_yy *)parm)->yyHour = i;
			((struct date_yy *)parm)->yyMinutes = 0;
	    	((struct date_yy *)parm)->yySeconds = 0;
		} else {
			((struct date_yy *)parm)->yyHaveTime = 0;
		}
	    ((struct date_yy *)parm)->yyMeridian = MER24;
	}
#line 1778 "parsedate.c"
    break;

  case 56: /* rel: relunit tAGO  */
#line 470 "./parsedate.y"
                       {
	    ((struct date_yy *)parm)->yyRelSeconds =
			-((struct date_yy *)parm)->yyRelSeconds;
	    ((struct date_yy *)parm)->yyRelMinutes =
			-((struct date_yy *)parm)->yyRelMinutes;
	    ((struct date_yy *)parm)->yyRelHour =
			-((struct date_yy *)parm)->yyRelHour;
	    ((struct date_yy *)parm)->yyRelDay =
			-((struct date_yy *)parm)->yyRelDay;
	    ((struct date_yy *)parm)->yyRelMonth =
			-((struct date_yy *)parm)->yyRelMonth;
	    ((struct date_yy *)parm)->yyRelYear =
			-((struct date_yy *)parm)->yyRelYear;
	}
#line 1797 "parsedate.c"
    break;

  case 58: /* relunit: tUNUMBER tYEAR_UNIT  */
#line 487 "./parsedate.y"
                              {
	    ((struct date_yy *)parm)->yyRelYear += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1805 "parsedate.c"
    break;

  case 59: /* relunit: tSNUMBER tYEAR_UNIT  */
#line 490 "./parsedate.y"
                              {
	    ((struct date_yy *)parm)->yyRelYear += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1813 "parsedate.c"
    break;

  case 60: /* relunit: tYEAR_UNIT  */
#line 493 "./parsedate.y"
                     {
	    ((struct date_yy *)parm)->yyRelYear += (yyvsp[0].Number);
	}
#line 1821 "parsedate.c"
    break;

  case 61: /* relunit: tUNUMBER tMONTH_UNIT  */
#line 496 "./parsedate.y"
                               {
	    ((struct date_yy *)parm)->yyRelMonth += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1829 "parsedate.c"
    break;

  case 62: /* relunit: tSNUMBER tMONTH_UNIT  */
#line 499 "./parsedate.y"
                               {
	    ((struct date_yy *)parm)->yyRelMonth += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1837 "parsedate.c"
    break;

  case 63: /* relunit: tMONTH_UNIT  */
#line 502 "./parsedate.y"
                      {
	    ((struct date_yy *)parm)->yyRelMonth += (yyvsp[0].Number);
	}
#line 1845 "parsedate.c"
    break;

  case 64: /* relunit: tUNUMBER tDAY_UNIT  */
#line 505 "./parsedate.y"
                             {
	    ((struct date_yy *)parm)->yyRelDay += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1853 "parsedate.c"
    break;

  case 65: /* relunit: tSNUMBER tDAY_UNIT  */
#line 508 "./parsedate.y"
                             {
	    ((struct date_yy *)parm)->yyRelDay += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1861 "parsedate.c"
    break;

  case 66: /* relunit: tDAY_UNIT  */
#line 511 "./parsedate.y"
                    {
	    ((struct date_yy *)parm)->yyRelDay += (yyvsp[0].Number);
	}
#line 1869 "parsedate.c"
    break;

  case 67: /* relunit: tUNUMBER tHOUR_UNIT  */
#line 514 "./parsedate.y"
                              {
	    ((struct date_yy *)parm)->yyRelHour += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1877 "parsedate.c"
    break;

  case 68: /* relunit: tSNUMBER tHOUR_UNIT  */
#line 517 "./parsedate.y"
                              {
	    ((struct date_yy *)parm)->yyRelHour += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1885 "parsedate.c"
    break;

  case 69: /* relunit: tHOUR_UNIT  */
#line 520 "./parsedate.y"
                     {
	    ((struct date_yy *)parm)->yyRelHour += (yyvsp[0].Number);
	}
#line 1893 "parsedate.c"
    break;

  case 70: /* relunit: tUNUMBER tMINUTE_UNIT  */
#line 523 "./parsedate.y"
                                {
	    ((struct date_yy *)parm)->yyRelMinutes += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1901 "parsedate.c"
    break;

  case 71: /* relunit: tSNUMBER tMINUTE_UNIT  */
#line 526 "./parsedate.y"
                                {
	    ((struct date_yy *)parm)->yyRelMinutes += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1909 "parsedate.c"
    break;

  case 72: /* relunit: tMINUTE_UNIT  */
#line 529 "./parsedate.y"
                       {
	    ((struct date_yy *)parm)->yyRelMinutes += (yyvsp[0].Number);
	}
#line 1917 "parsedate.c"
    break;

  case 73: /* relunit: tUNUMBER tSEC_UNIT  */
#line 532 "./parsedate.y"
                             {
	    ((struct date_yy *)parm)->yyRelSeconds += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1925 "parsedate.c"
    break;

  case 74: /* relunit: tSNUMBER tSEC_UNIT  */
#line 535 "./parsedate.y"
                             {
	    ((struct date_yy *)parm)->yyRelSeconds += (yyvsp[-1].Number) * (yyvsp[0].Number);
	}
#line 1933 "parsedate.c"
    break;

  case 75: /* relunit: tSEC_UNIT  */
#line 538 "./parsedate.y"
                    {
	    ((struct date_yy *)parm)->yyRelSeconds += (yyvsp[0].Number);
	}
#line 1941 "parsedate.c"
    break;

  case 76: /* number: tUNUMBER  */
#line 544 "./parsedate.y"
          {
	    if(((struct date_yy *)parm)->yyHaveTime && 
			((struct date_yy *)parm)->yyHaveDate && 
			!((struct date_yy *)parm)->yyHaveRel)
	      ((struct date_yy *)parm)->yyYear = (yyvsp[0].Number);
	    else
	      {
		if((yyvsp[0].Number) > 10000)
		  {
		    ((struct date_yy *)parm)->yyHaveDate++;
		    ((struct date_yy *)parm)->yyDay= ((yyvsp[0].Number))%100;
		    ((struct date_yy *)parm)->yyMonth= ((yyvsp[0].Number)/100)%100;
		    ((struct date_yy *)parm)->yyYear = (yyvsp[0].Number)/10000;
		  }
		else
		  {
		    ((struct date_yy *)parm)->yyHaveTime++;
		    if((yyvsp[0].Number) < 100)
		      {
			((struct date_yy *)parm)->yyHour = (yyvsp[0].Number);
			((struct date_yy *)parm)->yyMinutes = 0;
		      }
		    else
		      {
		    	((struct date_yy *)parm)->yyHour = (yyvsp[0].Number) / 100;
		    	((struct date_yy *)parm)->yyMinutes = (yyvsp[0].Number) % 100;
		      }
		    ((struct date_yy *)parm)->yySeconds = 0;
		    ((struct date_yy *)parm)->yyMeridian = MER24;
		  }
	      }
	  }
#line 1978 "parsedate.c"
    break;

  case 77: /* o_merid: tMERIDIAN  */
#line 579 "./parsedate.y"
          {
			 ((struct date_yy *)parm)->yyMeridian = (yyvsp[0].Meridian);
		  }
#line 1986 "parsedate.c"
    break;


#line 1990 "parsedate.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == RAPTOR_PARSEDATE_EMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (parm, YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= RAPTOR_PARSEDATE_EOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == RAPTOR_PARSEDATE_EOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, parm);
          yychar = RAPTOR_PARSEDATE_EMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, parm);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (parm, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != RAPTOR_PARSEDATE_EMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, parm);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, parm);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 584 "./parsedate.y"


time_t get_date (char *p, time_t *now);

/* Month and day table. */
static TABLE const MonthDayTable[] = {
    { "january",	tMONTH,  1 },
    { "february",	tMONTH,  2 },
    { "march",		tMONTH,  3 },
    { "april",		tMONTH,  4 },
    { "may",		tMONTH,  5 },
    { "june",		tMONTH,  6 },
    { "july",		tMONTH,  7 },
    { "august",		tMONTH,  8 },
    { "september",	tMONTH,  9 },
    { "sept",		tMONTH,  9 },
    { "october",	tMONTH, 10 },
    { "november",	tMONTH, 11 },
    { "december",	tMONTH, 12 },
    { "sunday",		tDAY, 0 },
    { "monday",		tDAY, 1 },
    { "tuesday",	tDAY, 2 },
    { "tues",		tDAY, 2 },
    { "wednesday",	tDAY, 3 },
    { "wednes",		tDAY, 3 },
    { "thursday",	tDAY, 4 },
    { "thur",		tDAY, 4 },
    { "thurs",		tDAY, 4 },
    { "friday",		tDAY, 5 },
    { "saturday",	tDAY, 6 },
    { NULL, 0, 0 }
};

/* Time units table. */
static TABLE const UnitsTable[] = {
    { "year",		tYEAR_UNIT,	1 },
    { "month",		tMONTH_UNIT,	1 },
    { "fortnight",	tDAY_UNIT,	14 },
    { "week",		tDAY_UNIT,	7 },
    { "day",		tDAY_UNIT,	1 },
    { "hour",		tHOUR_UNIT,	1 },
    { "minute",		tMINUTE_UNIT,	1 },
    { "min",		tMINUTE_UNIT,	1 },
    { "second",		tSEC_UNIT,	1 },
    { "sec",		tSEC_UNIT,	1 },
    { NULL, 0, 0 }
};

/* Assorted relative-time words. */
static TABLE const OtherTable[] = {
    { "tomorrow",	tDAY_UNIT,	1 },
    { "yesterday",	tDAY_UNIT,	-1 },
    { "today",		tDAY_UNIT,	0 },
    { "now",		tDAY_UNIT,	0 },
    { "last",		tUNUMBER,	-1 },
    { "this",		tUNUMBER,	0 },
    { "next",		tUNUMBER,	1 },
    { "first",		tUNUMBER,	1 },
/*  { "second",		tUNUMBER,	2 }, */
    { "third",		tUNUMBER,	3 },
    { "fourth",		tUNUMBER,	4 },
    { "fifth",		tUNUMBER,	5 },
    { "sixth",		tUNUMBER,	6 },
    { "seventh",	tUNUMBER,	7 },
    { "eighth",		tUNUMBER,	8 },
    { "ninth",		tUNUMBER,	9 },
    { "tenth",		tUNUMBER,	10 },
    { "eleventh",	tUNUMBER,	11 },
    { "twelfth",	tUNUMBER,	12 },
    { "ago",		tAGO,	1 },
    { NULL, 0, 0 }
};

/* The timezone table. */
static TABLE const TimezoneTable[] = {
    { "gmt",	tZONE,     HOUR ( 0) },	/* Greenwich Mean */
    { "ut",	tZONE,     HOUR ( 0) },	/* Universal (Coordinated) */
    { "utc",	tZONE,     HOUR ( 0) },
    { "wet",	tZONE,     HOUR ( 0) },	/* Western European */
    { "bst",	tDAYZONE,  HOUR ( 0) },	/* British Summer */
    { "wat",	tZONE,     HOUR ( 1) },	/* West Africa */
    { "at",	tZONE,     HOUR ( 2) },	/* Azores */
#if	0
    /* For completeness.  BST is also British Summer, and GST is
     * also Guam Standard. */
    { "bst",	tZONE,     HOUR ( 3) },	/* Brazil Standard */
    { "gst",	tZONE,     HOUR ( 3) },	/* Greenland Standard */
#endif
#if 0
    { "nft",	tZONE,     HOUR (3.5) },	/* Newfoundland */
    { "nst",	tZONE,     HOUR (3.5) },	/* Newfoundland Standard */
    { "ndt",	tDAYZONE,  HOUR (3.5) },	/* Newfoundland Daylight */
#endif
    { "ast",	tZONE,     HOUR ( 4) },	/* Atlantic Standard */
    { "adt",	tDAYZONE,  HOUR ( 4) },	/* Atlantic Daylight */
    { "est",	tZONE,     HOUR ( 5) },	/* Eastern Standard */
    { "edt",	tDAYZONE,  HOUR ( 5) },	/* Eastern Daylight */
    { "cst",	tZONE,     HOUR ( 6) },	/* Central Standard */
    { "cdt",	tDAYZONE,  HOUR ( 6) },	/* Central Daylight */
    { "mst",	tZONE,     HOUR ( 7) },	/* Mountain Standard */
    { "mdt",	tDAYZONE,  HOUR ( 7) },	/* Mountain Daylight */
    { "pst",	tZONE,     HOUR ( 8) },	/* Pacific Standard */
    { "pdt",	tDAYZONE,  HOUR ( 8) },	/* Pacific Daylight */
    { "yst",	tZONE,     HOUR ( 9) },	/* Yukon Standard */
    { "ydt",	tDAYZONE,  HOUR ( 9) },	/* Yukon Daylight */
    { "hst",	tZONE,     HOUR (10) },	/* Hawaii Standard */
    { "hdt",	tDAYZONE,  HOUR (10) },	/* Hawaii Daylight */
    { "cat",	tZONE,     HOUR (10) },	/* Central Alaska */
    { "akst",	tZONE,     HOUR (10) }, /* Alaska Standard */
    { "akdt",	tZONE,     HOUR (10) }, /* Alaska Daylight */
    { "ahst",	tZONE,     HOUR (10) },	/* Alaska-Hawaii Standard */
    { "nt",	tZONE,     HOUR (11) },	/* Nome */
    { "idlw",	tZONE,     HOUR (12) },	/* International Date Line West */
    { "cet",	tZONE,     -HOUR (1) },	/* Central European */
    { "cest",	tDAYZONE,  -HOUR (1) },	/* Central European Summer */
    { "met",	tZONE,     -HOUR (1) },	/* Middle European */
    { "mewt",	tZONE,     -HOUR (1) },	/* Middle European Winter */
    { "mest",	tDAYZONE,  -HOUR (1) },	/* Middle European Summer */
    { "mesz",	tDAYZONE,  -HOUR (1) },	/* Middle European Summer */
    { "swt",	tZONE,     -HOUR (1) },	/* Swedish Winter */
    { "sst",	tDAYZONE,  -HOUR (1) },	/* Swedish Summer */
    { "fwt",	tZONE,     -HOUR (1) },	/* French Winter */
    { "fst",	tDAYZONE,  -HOUR (1) },	/* French Summer */
    { "eet",	tZONE,     -HOUR (2) },	/* Eastern Europe, USSR Zone 1 */
    { "bt",	tZONE,     -HOUR (3) },	/* Baghdad, USSR Zone 2 */
#if 0
    { "it",	tZONE,     -HOUR (3.5) },/* Iran */
#endif
    { "zp4",	tZONE,     -HOUR (4) },	/* USSR Zone 3 */
    { "zp5",	tZONE,     -HOUR (5) },	/* USSR Zone 4 */
#if 0
    { "ist",	tZONE,     -HOUR (5.5) },/* Indian Standard */
#endif
    { "zp6",	tZONE,     -HOUR (6) },	/* USSR Zone 5 */
#if	0
    /* For completeness.  NST is also Newfoundland Standard, and SST is
     * also Swedish Summer. */
    { "nst",	tZONE,     -HOUR (6.5) },/* North Sumatra */
    { "sst",	tZONE,     -HOUR (7) },	/* South Sumatra, USSR Zone 6 */
#endif	/* 0 */
    { "wast",	tZONE,     -HOUR (7) },	/* West Australian Standard */
    { "wadt",	tDAYZONE,  -HOUR (7) },	/* West Australian Daylight */
#if 0
    { "jt",	tZONE,     -HOUR (7.5) },/* Java (3pm in Cronusland!) */
#endif
    { "cct",	tZONE,     -HOUR (8) },	/* China Coast, USSR Zone 7 */
    { "jst",	tZONE,     -HOUR (9) },	/* Japan Standard, USSR Zone 8 */
#if 0
    { "cast",	tZONE,     -HOUR (9.5) },/* Central Australian Standard */
    { "cadt",	tDAYZONE,  -HOUR (9.5) },/* Central Australian Daylight */
#endif
    { "east",	tZONE,     -HOUR (10) },	/* Eastern Australian Standard */
    { "eadt",	tDAYZONE,  -HOUR (10) },	/* Eastern Australian Daylight */
    { "gst",	tZONE,     -HOUR (10) },	/* Guam Standard, USSR Zone 9 */
    { "nzt",	tZONE,     -HOUR (12) },	/* New Zealand */
    { "nzst",	tZONE,     -HOUR (12) },	/* New Zealand Standard */
    { "nzdt",	tDAYZONE,  -HOUR (12) },	/* New Zealand Daylight */
    { "idle",	tZONE,     -HOUR (12) },	/* International Date Line East */
    {  NULL, 0, 0  }
};

/* Military timezone table. */
static TABLE const MilitaryTable[] = {
    { "a",	tZONE,	HOUR (- 1) },
    { "b",	tZONE,	HOUR (- 2) },
    { "c",	tZONE,	HOUR (- 3) },
    { "d",	tZONE,	HOUR (- 4) },
    { "e",	tZONE,	HOUR (- 5) },
    { "f",	tZONE,	HOUR (- 6) },
    { "g",	tZONE,	HOUR (- 7) },
    { "h",	tZONE,	HOUR (- 8) },
    { "i",	tZONE,	HOUR (- 9) },
    { "k",	tZONE,	HOUR (-10) },
    { "l",	tZONE,	HOUR (-11) },
    { "m",	tZONE,	HOUR (-12) },
    { "n",	tZONE,	HOUR (  1) },
    { "o",	tZONE,	HOUR (  2) },
    { "p",	tZONE,	HOUR (  3) },
    { "q",	tZONE,	HOUR (  4) },
    { "r",	tZONE,	HOUR (  5) },
    { "s",	tZONE,	HOUR (  6) },
    { "t",	tTZONE,	HOUR (  7) },
    { "u",	tZONE,	HOUR (  8) },
    { "v",	tZONE,	HOUR (  9) },
    { "w",	tWZONE,	HOUR ( 10) },
    { "x",	tZONE,	HOUR ( 11) },
    { "y",	tZONE,	HOUR ( 12) },
    { "z",	tZZONE,	HOUR (  0) },
    { NULL, 0, 0 }
};




/* ARGSUSED */
static int
yyerror(void* parm, const char *s)
{
  return 0;
}

static int
ToHour(int Hours, MERIDIAN Meridian)
{
  switch (Meridian)
    {
    case MER24:
      if(Hours < 0 || Hours > 23)
	return -1;
      return Hours;
    case MERam:
      if(Hours < 1 || Hours > 12)
	return -1;
      if(Hours == 12)
	Hours = 0;
      return Hours;
    case MERpm:
      if(Hours < 1 || Hours > 12)
	return -1;
      if(Hours == 12)
	Hours = 0;
      return Hours + 12;
    default:
#ifdef RAPTOR_DEBUG
      fprintf(stderr, "%s:%d:%s: UNKNOWN Meridian %d - add a new case", 
              __FILE__, __LINE__, __FUNCTION__, (int)Meridian);
#endif
      return -1;
    }
  /* NOTREACHED */
}

static int
ToYear(int Year)
{
  if(Year < 0)
    Year = -Year;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  if(Year < 69)
    Year += 2000;
  else if(Year < 100)
    Year += 1900;

  return Year;
}

static int
LookupWord (YYSTYPE *lvalp, char *buff)
{
  char *p;
  char *q;
  const TABLE *tp;
  int i;
  int abbrev;

  /* Make it lowercase. */
  for(p = buff; *p; p++)
    if(isupper ((unsigned char) *p))
      *p = tolower (*p);

  if(strcmp (buff, "am") == 0 || strcmp (buff, "a.m.") == 0)
    {
      lvalp->Meridian = MERam;
      return tMERIDIAN;
    }
  if(strcmp (buff, "pm") == 0 || strcmp (buff, "p.m.") == 0)
    {
      lvalp->Meridian = MERpm;
      return tMERIDIAN;
    }

  /* See if we have an abbreviation for a month. */
  if(strlen (buff) == 3)
    abbrev = 1;
  else if(strlen (buff) == 4 && buff[3] == '.')
    {
      abbrev = 1;
      buff[3] = '\0';
    }
  else
    abbrev = 0;

  for(tp = MonthDayTable; tp->name; tp++)
    {
      if(abbrev)
	{
	  if(strncmp (buff, tp->name, 3) == 0)
	    {
	      lvalp->Number = tp->value;
	      return tp->type;
	    }
	}
      else if(strcmp (buff, tp->name) == 0)
	{
	  lvalp->Number = tp->value;
	  return tp->type;
	}
    }

  for(tp = TimezoneTable; tp->name; tp++)
    if(strcmp (buff, tp->name) == 0)
      {
	lvalp->Number = tp->value;
	return tp->type;
      }

  if(strcmp (buff, "dst") == 0)
    return tDST;

  for(tp = UnitsTable; tp->name; tp++)
    if(strcmp (buff, tp->name) == 0)
      {
	lvalp->Number = tp->value;
	return tp->type;
      }

  /* Strip off any plural and try the units table again. */
  i = RAPTOR_BAD_CAST(int, strlen(buff)) - 1;
  if(buff[i] == 's')
    {
      buff[i] = '\0';
      for(tp = UnitsTable; tp->name; tp++)
	if(strcmp (buff, tp->name) == 0)
	  {
	    lvalp->Number = tp->value;
	    return tp->type;
	  }
      buff[i] = 's';		/* Put back for "this" in OtherTable. */
    }

  for(tp = OtherTable; tp->name; tp++)
    if(strcmp (buff, tp->name) == 0)
      {
	lvalp->Number = tp->value;
	return tp->type;
      }

  /* Military timezones. */
  if(buff[1] == '\0' && isalpha ((unsigned char) *buff))
    {
      for(tp = MilitaryTable; tp->name; tp++)
	if(strcmp (buff, tp->name) == 0)
	  {
	    lvalp->Number = tp->value;
	    return tp->type;
	  }
    }

  /* Drop out any periods and try the timezone table again. */
  for(i = 0, p = q = buff; *q; q++)
    if(*q != '.')
      *p++ = *q;
    else
      i++;
  *p = '\0';
  if(i)
    for(tp = TimezoneTable; tp->name; tp++)
      if(strcmp (buff, tp->name) == 0)
	{
	  lvalp->Number = tp->value;
	  return tp->type;
	}

  return tID;
}

int yylex(YYSTYPE *lvalp, void *parm)
{
  unsigned char c;
  char *p;
  char buff[20];
  int Count;
  int sign;
  struct date_yy * date = (struct date_yy *)parm;

  for(;;)
    {
      while(isspace ((unsigned char) *date->yyInput))
	date->yyInput++;

      if(isdigit (c = *date->yyInput) || c == '-' || c == '+')
	{
	  if(c == '-' || c == '+')
	    {
	      sign = c == '-' ? -1 : 1;
	      if(!ISDIGIT (*++date->yyInput))
		/* skip the '-' sign */
		continue;
	    }
	  else
	    sign = 0;
	  for(lvalp->Number = 0; ISDIGIT (c = *date->yyInput++);)
	    lvalp->Number = 10 * lvalp->Number + c - '0';
	  date->yyInput--;
	  if(sign < 0)
	    lvalp->Number = -lvalp->Number;
	  /* Ignore ordinal suffixes on numbers */
	  c = *date->yyInput;
	  if(c == 's' || c == 'n' || c == 'r' || c == 't') {
	    c = *++date->yyInput;
	    if(c == 't' || c == 'd' || c == 'h') {
	      date->yyInput++;
	    } else {
	      date->yyInput--;
	    }
	  }
	  return sign ? tSNUMBER : tUNUMBER;
	}
      if(isalpha (c))
	{
	  for(p = buff; (c = *date->yyInput++, isalpha (c)) || c == '.';)
	    if(p < &buff[sizeof buff - 1])
	      *p++ = c;
	  *p = '\0';
	  date->yyInput--;
	  return LookupWord (lvalp, buff);
	}
      if(c != '(')
	return *date->yyInput++;
      Count = 0;
      do
	{
	  c = *date->yyInput++;
	  if(c == '\0')
	    return c;
	  if(c == '(')
	    Count++;
	  else if(c == ')')
	    Count--;
	}
      while(Count > 0);
    }
}

#define TM_YEAR_ORIGIN 1900

/* Yield A - B, measured in seconds.  */
static long
difftm (struct tm *a, struct tm *b)
{
  int ay = a->tm_year + (TM_YEAR_ORIGIN - 1);
  int by = b->tm_year + (TM_YEAR_ORIGIN - 1);
  long days = (
  /* difference in day of year */
		a->tm_yday - b->tm_yday
  /* + intervening leap days */
		+ ((ay >> 2) - (by >> 2))
		- (ay / 100 - by / 100)
		+ ((ay / 100 >> 2) - (by / 100 >> 2))
  /* + difference in years * 365 */
		+ (long) (ay - by) * 365
  );
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
		+ (a->tm_min - b->tm_min))
	  + (a->tm_sec - b->tm_sec));
}

time_t raptor_parse_date(const char *p, time_t *now)
{
  struct tm tm, tm0, *tmp;
  time_t Start;
  struct date_yy date;

  date.yyInput = p;
  Start = now ? *now : time ((time_t *) NULL);
  tmp = localtime (&Start);
  if(!tmp)
    return -1;
  date.yyYear = tmp->tm_year + TM_YEAR_ORIGIN;
  date.yyMonth = tmp->tm_mon + 1;
  date.yyDay = tmp->tm_mday;
  date.yyHour = tmp->tm_hour;
  date.yyMinutes = tmp->tm_min;
  date.yySeconds = tmp->tm_sec;
  tm.tm_isdst = tmp->tm_isdst;
  date.yyMeridian = MER24;
  date.yyRelSeconds = 0;
  date.yyRelMinutes = 0;
  date.yyRelHour = 0;
  date.yyRelDay = 0;
  date.yyRelMonth = 0;
  date.yyRelYear = 0;
  date.yyHaveDate = 0;
  date.yyHaveDay = 0;
  date.yyHaveRel = 0;
  date.yyHaveTime = 0;
  date.yyHaveZone = 0;

  if(yyparse (&date)
      || date.yyHaveTime > 1 || date.yyHaveZone > 1 
	  || date.yyHaveDate > 1 || date.yyHaveDay > 1) {
    return -1;
  }
  tm.tm_year = ToYear (date.yyYear) - TM_YEAR_ORIGIN + date.yyRelYear;
  tm.tm_mon = date.yyMonth - 1 + date.yyRelMonth;
  tm.tm_mday = date.yyDay + date.yyRelDay;
  if(date.yyHaveTime || (date.yyHaveRel && !date.yyHaveDate && !date.yyHaveDay))
    {
      tm.tm_hour = ToHour (date.yyHour, date.yyMeridian);
      if(tm.tm_hour < 0)
	return -1;
      tm.tm_min = date.yyMinutes;
      tm.tm_sec = date.yySeconds;
    }
  else
    {
      tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    }
  tm.tm_hour += date.yyRelHour;
  tm.tm_min += date.yyRelMinutes;
  tm.tm_sec += date.yyRelSeconds;

  /* Let mktime deduce tm_isdst if we have an absolute timestamp,
     or if the relative timestamp mentions days, months, or years.  */
  if(date.yyHaveDate | date.yyHaveDay | date.yyHaveTime | date.yyRelDay | date.yyRelMonth | date.yyRelYear)
    tm.tm_isdst = -1;

  tm0 = tm;

  Start = mktime (&tm);

  if(Start == (time_t) -1)
    {

      /* Guard against falsely reporting errors near the time_t boundaries
         when parsing times in other time zones.  For example, if the min
         time_t value is 1970-01-01 00:00:00 UTC and we are 8 hours ahead
         of UTC, then the min localtime value is 1970-01-01 08:00:00; if
         we apply mktime to 1970-01-01 00:00:00 we will get an error, so
         we apply mktime to 1970-01-02 08:00:00 instead and adjust the time
         zone by 24 hours to compensate.  This algorithm assumes that
         there is no DST transition within a day of the time_t boundaries.  */
      if(date.yyHaveZone)
	{
	  tm = tm0;
	  if(tm.tm_year <= EPOCH - TM_YEAR_ORIGIN)
	    {
	      tm.tm_mday++;
	      date.yyTimezone -= 24 * 60;
	    }
	  else
	    {
	      tm.tm_mday--;
	      date.yyTimezone += 24 * 60;
	    }
	  Start = mktime (&tm);
	}

      if(Start == (time_t) -1)
	return Start;
    }

  if(date.yyHaveDay && !date.yyHaveDate)
    {
      tm.tm_mday += ((date.yyDayNumber - tm.tm_wday + 7) % 7
		     + 7 * (date.yyDayOrdinal - (0 < date.yyDayOrdinal)));
      Start = mktime (&tm);
      if(Start == (time_t) -1)
	return Start;
    }

  if(date.yyHaveZone)
    {
      long delta;
      struct tm *gmt = gmtime (&Start);
      if(!gmt)
	return -1;
      delta = date.yyTimezone * 60L + difftm (&tm, gmt);

      if((Start + delta < Start) != (delta < 0))
	return -1;		/* time_t overflow */
      Start += delta;
    }

  return Start;
}
