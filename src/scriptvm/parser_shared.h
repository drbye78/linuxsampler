/*
 * Copyright (c) 2014-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// types shared between auto generated lexer and parser ...

#ifndef LS_INSTRSCRIPTSPARSER_SHARED_H
#define LS_INSTRSCRIPTSPARSER_SHARED_H

#include <stdio.h>
#include "tree.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include "parser.tab.h"
#else
# include "parser.h"
#endif

#include "../common/global_private.h"
    
struct _YYSTYPE {
    union {
        LinuxSampler::vmint iValue;
        LinuxSampler::vmfloat fValue;
        char* sValue;
        struct {
            LinuxSampler::vmint iValue;
            LinuxSampler::MetricPrefix_t prefix[2];
            LinuxSampler::StdUnit_t unit;
        } iUnitValue;
        struct {
            LinuxSampler::vmfloat fValue;
            LinuxSampler::MetricPrefix_t prefix[2];
            LinuxSampler::StdUnit_t unit;
        } fUnitValue;
    };
    LinuxSampler::EventHandlersRef nEventHandlers;
    LinuxSampler::EventHandlerRef nEventHandler;
    LinuxSampler::StatementsRef nStatements;
    LinuxSampler::StatementRef nStatement;
    LinuxSampler::FunctionCallRef nFunctionCall;
    LinuxSampler::ArgsRef nArgs;
    LinuxSampler::ExpressionRef nExpression;
    LinuxSampler::CaseBranch nCaseBranch;
    LinuxSampler::CaseBranches nCaseBranches;
    LinuxSampler::Qualifier_t varQualifier;
};
#define YYSTYPE _YYSTYPE
#define yystype YYSTYPE     ///< For backward compatibility.
#ifndef YYSTYPE_IS_DECLARED
# define YYSTYPE_IS_DECLARED ///< We tell the lexer / parser that we use our own data structure as defined above.
#endif

// custom Bison location type to support raw byte positions
struct _YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    int first_byte;
    int length_bytes;
};
#define YYLTYPE _YYLTYPE
#define YYLTYPE_IS_DECLARED 1

// override Bison's default location passing to support raw byte positions
#define YYLLOC_DEFAULT(Cur, Rhs, N)                         \
do                                                          \
  if (N)                                                    \
    {                                                       \
      (Cur).first_line   = YYRHSLOC(Rhs, 1).first_line;     \
      (Cur).first_column = YYRHSLOC(Rhs, 1).first_column;   \
      (Cur).last_line    = YYRHSLOC(Rhs, N).last_line;      \
      (Cur).last_column  = YYRHSLOC(Rhs, N).last_column;    \
      (Cur).first_byte   = YYRHSLOC(Rhs, 1).first_byte;     \
      (Cur).length_bytes = (YYRHSLOC(Rhs, N).first_byte  -  \
                            YYRHSLOC(Rhs, 1).first_byte) +  \
                            YYRHSLOC(Rhs, N).length_bytes;  \
    }                                                       \
  else                                                      \
    {                                                       \
      (Cur).first_line   = (Cur).last_line   =              \
        YYRHSLOC(Rhs, 0).last_line;                         \
      (Cur).first_column = (Cur).last_column =              \
        YYRHSLOC(Rhs, 0).last_column;                       \
      (Cur).first_byte   = YYRHSLOC(Rhs, 0).first_byte;     \
      (Cur).length_bytes = YYRHSLOC(Rhs, 0).length_bytes;   \
    }                                                       \
while (0)

#endif // LS_INSTRSCRIPTSPARSER_SHARED_H
