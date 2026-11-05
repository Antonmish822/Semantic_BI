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

#ifndef YY_TURTLE_PARSER_TURTLE_PARSER_H_INCLUDED
# define YY_TURTLE_PARSER_TURTLE_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef TURTLE_PARSER_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define TURTLE_PARSER_DEBUG 1
#  else
#   define TURTLE_PARSER_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define TURTLE_PARSER_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined TURTLE_PARSER_DEBUG */
#if TURTLE_PARSER_DEBUG
extern int turtle_parser_debug;
#endif

/* Token kinds.  */
#ifndef TURTLE_PARSER_TOKENTYPE
# define TURTLE_PARSER_TOKENTYPE
  enum turtle_parser_tokentype
  {
    TURTLE_PARSER_EMPTY = -2,
    TURTLE_PARSER_EOF = 0,         /* "end of file"  */
    TURTLE_PARSER_error = 256,     /* error  */
    TURTLE_PARSER_UNDEF = 257,     /* "invalid token"  */
    A = 258,                       /* "a"  */
    HAT = 259,                     /* "^"  */
    DOT = 260,                     /* "."  */
    COMMA = 261,                   /* ","  */
    SEMICOLON = 262,               /* ";"  */
    LEFT_SQUARE = 263,             /* "["  */
    RIGHT_SQUARE = 264,            /* "]"  */
    LEFT_ROUND = 265,              /* "("  */
    RIGHT_ROUND = 266,             /* ")"  */
    LEFT_CURLY = 267,              /* "{"  */
    RIGHT_CURLY = 268,             /* "}"  */
    TRUE_TOKEN = 269,              /* "true"  */
    FALSE_TOKEN = 270,             /* "false"  */
    PREFIX = 271,                  /* "@prefix"  */
    BASE = 272,                    /* "@base"  */
    SPARQL_PREFIX = 273,           /* "PREFIX"  */
    SPARQL_BASE = 274,             /* "BASE"  */
    STRING_LITERAL = 275,          /* "string literal"  */
    IDENTIFIER = 276,              /* "identifier"  */
    LANGTAG = 277,                 /* "langtag"  */
    INTEGER_LITERAL = 278,         /* "integer literal"  */
    FLOATING_LITERAL = 279,        /* "floating point literal"  */
    DECIMAL_LITERAL = 280,         /* "decimal literal"  */
    BLANK_LITERAL = 281,           /* "blank node"  */
    URI_LITERAL = 282,             /* "URI literal"  */
    GRAPH_NAME_LEFT_CURLY = 283,   /* "Graph URI literal {"  */
    QNAME_LITERAL = 284,           /* "QName"  */
    ERROR_TOKEN = 285              /* ERROR_TOKEN  */
  };
  typedef enum turtle_parser_tokentype turtle_parser_token_kind_t;
#endif

/* Value type.  */
#if ! defined TURTLE_PARSER_STYPE && ! defined TURTLE_PARSER_STYPE_IS_DECLARED
union TURTLE_PARSER_STYPE
{
#line 145 "./turtle_parser.y"

  unsigned char *string;
  raptor_term *identifier;
  raptor_sequence *sequence;
  raptor_uri *uri;

#line 109 "turtle_parser.h"

};
typedef union TURTLE_PARSER_STYPE TURTLE_PARSER_STYPE;
# define TURTLE_PARSER_STYPE_IS_TRIVIAL 1
# define TURTLE_PARSER_STYPE_IS_DECLARED 1
#endif




int turtle_parser_parse (raptor_parser* rdf_parser, void* yyscanner);


#endif /* !YY_TURTLE_PARSER_TURTLE_PARSER_H_INCLUDED  */
