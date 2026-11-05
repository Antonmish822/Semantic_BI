/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_RAPTOR_PARSEDATE_PARSEDATE_H_INCLUDED
# define YY_RAPTOR_PARSEDATE_PARSEDATE_H_INCLUDED
/* Debug traces.  */
#ifndef RAPTOR_PARSEDATE_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define RAPTOR_PARSEDATE_DEBUG 1
#  else
#   define RAPTOR_PARSEDATE_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define RAPTOR_PARSEDATE_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined RAPTOR_PARSEDATE_DEBUG */
#if RAPTOR_PARSEDATE_DEBUG
extern int raptor_parsedate_debug;
#endif

/* Token kinds.  */
#ifndef RAPTOR_PARSEDATE_TOKENTYPE
# define RAPTOR_PARSEDATE_TOKENTYPE
  enum raptor_parsedate_tokentype
  {
    RAPTOR_PARSEDATE_EMPTY = -2,
    RAPTOR_PARSEDATE_EOF = 0,      /* "end of file"  */
    RAPTOR_PARSEDATE_error = 256,  /* error  */
    RAPTOR_PARSEDATE_UNDEF = 257,  /* "invalid token"  */
    tAGO = 258,                    /* tAGO  */
    tDAY = 259,                    /* tDAY  */
    tDAY_UNIT = 260,               /* tDAY_UNIT  */
    tDAYZONE = 261,                /* tDAYZONE  */
    tDST = 262,                    /* tDST  */
    tHOUR_UNIT = 263,              /* tHOUR_UNIT  */
    tID = 264,                     /* tID  */
    tTZONE = 265,                  /* tTZONE  */
    tWZONE = 266,                  /* tWZONE  */
    tZZONE = 267,                  /* tZZONE  */
    tMERIDIAN = 268,               /* tMERIDIAN  */
    tMINUTE_UNIT = 269,            /* tMINUTE_UNIT  */
    tMONTH = 270,                  /* tMONTH  */
    tMONTH_UNIT = 271,             /* tMONTH_UNIT  */
    tSEC_UNIT = 272,               /* tSEC_UNIT  */
    tSNUMBER = 273,                /* tSNUMBER  */
    tUNUMBER = 274,                /* tUNUMBER  */
    tYEAR_UNIT = 275,              /* tYEAR_UNIT  */
    tZONE = 276                    /* tZONE  */
  };
  typedef enum raptor_parsedate_tokentype raptor_parsedate_token_kind_t;
#endif

/* Value type.  */




int raptor_parsedate_parse (struct date_yy *parm);


#endif /* !YY_RAPTOR_PARSEDATE_PARSEDATE_H_INCLUDED  */
