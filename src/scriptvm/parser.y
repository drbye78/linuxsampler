/*
 * Copyright (c) 2014-2017 Christian Schoenebeck and Andreas Persson
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */
 
/* Parser for NKSP real-time instrument script language. */

%{
    #define YYERROR_VERBOSE 1
    #include "parser_shared.h"
    #include <string>
    #include <map>
    using namespace LinuxSampler;

    void InstrScript_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err);
    void InstrScript_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt);
    int InstrScript_tnamerr(char* yyres, const char* yystr);
    int InstrScript_lex(YYSTYPE* lvalp, YYLTYPE* llocp, void* scanner);
    #define scanner context->scanner
    #define PARSE_ERR(loc,txt)  yyerror(&loc, context, txt)
    #define PARSE_WRN(loc,txt)  InstrScript_warning(&loc, context, txt)
    #define PARSE_DROP(loc)     context->addPreprocessorComment(loc.first_line, loc.last_line, loc.first_column+1, loc.last_column+1);
    #define yytnamerr(res,str)  InstrScript_tnamerr(res, str)
%}

// generate reentrant safe parser
%pure-parser
%parse-param { LinuxSampler::ParserContext* context }
%lex-param { void* scanner }
// avoid symbol collision with other (i.e. future) auto generated (f)lex scanners
// (NOTE: "=" is deprecated here with Bison 3.x, however removing it would cause an error with Bison 2.x)
%name-prefix="InstrScript_"
%locations
%defines
%error-verbose

%token <iValue> INTEGER "integer literal"
%token <fValue> REAL "real number literal"
%token <iUnitValue> INTEGER_UNIT "integer literal with unit"
%token <fUnitValue> REAL_UNIT "real number literal with unit"
%token <sValue> STRING "string literal"
%token <sValue> IDENTIFIER "function name"
%token <sValue> VARIABLE "variable name"
%token ON "keyword 'on'"
%token END "keyword 'end'"
%token INIT "keyword 'init'"
%token NOTE "keyword 'note'"
%token RELEASE "keyword 'release'"
%token CONTROLLER "keyword 'controller'"
%token DECLARE "keyword 'declare'"
%token ASSIGNMENT "operator ':='"
%token CONST_ "keyword 'const'"
%token POLYPHONIC "keyword 'polyphonic'"
%token WHILE "keyword 'while'"
%token SYNCHRONIZED "keyword 'synchronized'"
%token IF "keyword 'if'"
%token ELSE "keyword 'else'"
%token SELECT "keyword 'select'"
%token CASE "keyword 'case'"
%token TO "keyword 'to'"
%token OR "operator 'or'"
%token AND "operator 'and'"
%token NOT "operator 'not'"
%token BITWISE_OR "bitwise operator '.or.'"
%token BITWISE_AND "bitwise operator '.and.'"
%token BITWISE_NOT "bitwise operator '.not.'"
%token FUNCTION "keyword 'function'"
%token CALL "keyword 'call'"
%token MOD "operator 'mod'"
%token LE "operator '<='"
%token GE "operator '>='"
%token END_OF_FILE 0 "end of file"
%token UNKNOWN_CHAR "unknown character"

%type <nEventHandlers> script sections
%type <nEventHandler> section eventhandler
%type <nStatements> statements opt_statements userfunctioncall
%type <nStatement> statement assignment
%type <nFunctionCall> functioncall
%type <nArgs> args
%type <nExpression> arg expr logical_or_expr logical_and_expr bitwise_or_expr bitwise_and_expr rel_expr add_expr mul_expr unary_expr concat_expr
%type <nCaseBranch> caseclause
%type <nCaseBranches> caseclauses

%start script

%%

script:
    sections  {
        $$ = context->handlers = $1;
    }

sections:
    section  {
        $$ = new EventHandlers();
        if ($1) $$->add($1);
    }
    | sections section  {
        $$ = $1;
        if ($2) $$->add($2);
    }

section:
    function_declaration  {
        $$ = EventHandlerRef();
    }
    | eventhandler  {
        $$ = $1;
    }

eventhandler:
    ON NOTE opt_statements END ON  {
        if (context->onNote)
            PARSE_ERR(@2, "Redeclaration of 'note' event handler.");
        context->onNote = new OnNote($3);
        $$ = context->onNote;
    }
    | ON INIT opt_statements END ON  {
        if (context->onInit)
            PARSE_ERR(@2, "Redeclaration of 'init' event handler.");
        context->onInit = new OnInit($3);
        $$ = context->onInit;
    }
    | ON RELEASE opt_statements END ON  {
        if (context->onRelease)
            PARSE_ERR(@2, "Redeclaration of 'release' event handler.");
        context->onRelease = new OnRelease($3);
        $$ = context->onRelease;
    }
    | ON CONTROLLER opt_statements END ON  {
        if (context->onController)
            PARSE_ERR(@2, "Redeclaration of 'controller' event handler.");
        context->onController = new OnController($3);
        $$ = context->onController;
    }

function_declaration:
    FUNCTION IDENTIFIER opt_statements END FUNCTION  {
        const char* name = $2;
        if (context->functionProvider->functionByName(name)) {
            PARSE_ERR(@2, (String("There is already a built-in function with name '") + name + "'.").c_str());
        } else if (context->userFunctionByName(name)) {
            PARSE_ERR(@2, (String("There is already a user defined function with name '") + name + "'.").c_str());
        } else {
            context->userFnTable[name] = $3;
        }
    }

opt_statements:
    /* epsilon (empty argument) */  {
        $$ = new Statements();
    }
    | statements  {
        $$ = $1;
    }

statements:
    statement  {
        $$ = new Statements();
        if ($1) {
            if (!isNoOperation($1)) $$->add($1); // filter out NoOperation statements
        } else 
            PARSE_WRN(@1, "Not a statement.");
    }
    | statements statement  {
        $$ = $1;
        if ($2) {
            if (!isNoOperation($2)) $$->add($2); // filter out NoOperation statements
        } else
            PARSE_WRN(@2, "Not a statement.");
    }

statement:
    functioncall  {
        $$ = $1;
    }
    | userfunctioncall  {
        $$ = $1;
    }
    | DECLARE VARIABLE  {
        const char* name = $2;
        //printf("declared var '%s'\n", name);
        if (context->variableByName(name)) {
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
        } else if (name[0] == '@') {
            context->vartable[name] = new StringVariable(context);
        } else if (name[0] == '~') {
            context->vartable[name] = new RealVariable({
                .ctx = context
            });
        } else if (name[0] == '$') {
            context->vartable[name] = new IntVariable({
                .ctx = context
            });
        } else if (name[0] == '?') {
            PARSE_ERR(@2, (String("Real number array variable '") + name + "' declaration requires array size.").c_str());
        } else if (name[0] == '%') {
            PARSE_ERR(@2, (String("Integer array variable '") + name + "' declaration requires array size.").c_str());
        } else {
            PARSE_ERR(@2, (String("Variable '") + name + "' declared with unknown type.").c_str());
        }
        $$ = new NoOperation;
    }
    | DECLARE POLYPHONIC VARIABLE  {
        const char* name = $3;
        //printf("declared polyphonic var '%s'\n", name);
        if (context->variableByName(name)) {
            PARSE_ERR(@3, (String("Redeclaration of variable '") + name + "'.").c_str());
        } else if (name[0] != '$' && name[0] != '~') {
            PARSE_ERR(@3, "Polyphonic variables must only be declared either as integer or real number type.");
        } else if (name[0] == '~') {
            context->vartable[name] = new PolyphonicRealVariable({
                .ctx = context
            });
        } else {
            context->vartable[name] = new PolyphonicIntVariable({
                .ctx = context
            });
        }
        $$ = new NoOperation;
    }
    | DECLARE VARIABLE ASSIGNMENT expr  {
        const char* name = $2;
        //printf("declared assign var '%s'\n", name);
        const ExprType_t declType = exprTypeOfVarName(name);
        if (context->variableByName(name)) {
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
            $$ = new NoOperation;
        } else if ($4->exprType() == STRING_EXPR) {
            if (name[0] != '@')
                PARSE_WRN(@2, (String("Variable '") + name + "' declared as " + typeStr(declType) + ", string expression assigned though.").c_str());
            StringExprRef expr = $4;
            if (expr->isConstExpr()) {
                const String s = expr->evalStr();
                StringVariableRef var = new StringVariable(context);
                context->vartable[name] = var;
                $$ = new Assignment(var, new StringLiteral(s));
            } else {
                StringVariableRef var = new StringVariable(context);
                context->vartable[name] = var;
                $$ = new Assignment(var, expr);
            }
        } else if ($4->exprType() == REAL_EXPR) {
            if (name[0] != '~')
                PARSE_WRN(@2, (String("Variable '") + name + "' declared as " + typeStr(declType) + ", real number expression assigned though.").c_str());
            RealExprRef expr = $4;
            RealVariableRef var = new RealVariable({
                .ctx = context,
                .unitType = expr->unitType(),
                .isFinal = expr->isFinal()
            });
            if (expr->isConstExpr()) {
                $$ = new Assignment(var, new RealLiteral({
                    .value = expr->evalReal(),
                    .unitFactor = expr->unitFactor(),
                    .unitType = expr->unitType(),
                    .isFinal = expr->isFinal()
                }));
            } else {
                $$ = new Assignment(var, expr);
            }
            context->vartable[name] = var;
        } else if ($4->exprType() == INT_EXPR) {
            if (name[0] != '$')
                PARSE_WRN(@2, (String("Variable '") + name + "' declared as " + typeStr(declType) + ", integer expression assigned though.").c_str());
            IntExprRef expr = $4;
            IntVariableRef var = new IntVariable({
                .ctx = context,
                .unitType = expr->unitType(),
                .isFinal = expr->isFinal()
            });
            if (expr->isConstExpr()) {
                $$ = new Assignment(var, new IntLiteral({
                    .value = expr->evalInt(),
                    .unitFactor = expr->unitFactor(),
                    .unitType = expr->unitType(),
                    .isFinal = expr->isFinal()
                }));
            } else {
                $$ = new Assignment(var, expr);
            }
            context->vartable[name] = var;
        } else if ($4->exprType() == EMPTY_EXPR) {
            PARSE_ERR(@4, "Expression does not result in a value.");
            $$ = new NoOperation;
        } else if (isArray($4->exprType())) {
            PARSE_ERR(@2, (String("Variable '") + name + "' declared as scalar type, array expression assigned though.").c_str());
            $$ = new NoOperation;
        }
    }
    | DECLARE VARIABLE '[' expr ']'  {
        //printf("declare array without args\n");
        const char* name = $2;
        if (!$4->isConstExpr()) {
            PARSE_ERR(@4, (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
        } else if ($4->exprType() != INT_EXPR) {
            PARSE_ERR(@4, (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
        } else if (context->variableByName(name)) {
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
        } else {
            IntExprRef sizeExpr = $4;
            if (sizeExpr->unitType() || sizeExpr->hasUnitFactorNow()) {
                PARSE_ERR(@4, "Units are not allowed as array size.");
            } else {
                if (sizeExpr->isFinal())
                    PARSE_WRN(@4, "Final operator '!' is meaningless here.");
                vmint size = sizeExpr->evalInt();
                if (size <= 0) {
                    PARSE_ERR(@4, (String("Array variable '") + name + "' declared with array size " + ToString(size) + ".").c_str());
                } else {
                    if (name[0] == '?') {
                        context->vartable[name] = new RealArrayVariable(context, size);
                    } else if (name[0] == '%') {
                        context->vartable[name] = new IntArrayVariable(context, size);
                    } else {
                        PARSE_ERR(@2, (String("Variable '") + name + "' declared as unknown array type: use either '%' or '?' instead of '" + String(name).substr(0,1) + "'.").c_str());
                    }
                }
            }
        }
        $$ = new NoOperation;
    }
    | DECLARE VARIABLE '[' expr ']' ASSIGNMENT '(' args ')'  {
        const char* name = $2;
        if (!$4->isConstExpr()) {
            PARSE_ERR(@4, (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
        } else if ($4->exprType() != INT_EXPR) {
            PARSE_ERR(@4, (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
        } else if (context->variableByName(name)) {
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
        } else {
            IntExprRef sizeExpr = $4;
            ArgsRef args = $8;
            vmint size = sizeExpr->evalInt();
            if (size <= 0) {
                PARSE_ERR(@4, (String("Array variable '") + name + "' must be declared with positive array size.").c_str());
            } else if (args->argsCount() > size) {
                PARSE_ERR(@8, (String("Array variable '") + name +
                          "' was declared with size " + ToString(size) +
                          " but " + ToString(args->argsCount()) +
                          " values were assigned." ).c_str());
            } else if (sizeExpr->unitType() || sizeExpr->hasUnitFactorNow()) {
                PARSE_ERR(@4, "Units are not allowed as array size.");
            } else {
                if (sizeExpr->isFinal())
                    PARSE_WRN(@4, "Final operator '!' is meaningless here.");
                ExprType_t declType = EMPTY_EXPR;
                if (name[0] == '%') {
                    declType = INT_EXPR;
                } else if (name[0] == '?') {
                    declType = REAL_EXPR;
                } else if (name[0] == '$') {
                    PARSE_ERR(@2, (String("Variable '") + name + "' declaration ambiguous: Use '%' as name prefix for integer arrays instead of '$'.").c_str());
                } else if (name[0] == '~') {
                    PARSE_ERR(@2, (String("Variable '") + name + "' declaration ambiguous: Use '?' as name prefix for real number arrays instead of '~'.").c_str());
                } else {
                    PARSE_ERR(@2, (String("Variable '") + name + "' declared as unknown array type: use either '%' or '?' instead of '" + String(name).substr(0,1) + "'.").c_str());
                }
                bool argsOK = true;
                if (declType == EMPTY_EXPR) {
                    argsOK = false;
                } else {
                    for (vmint i = 0; i < args->argsCount(); ++i) {
                        if (args->arg(i)->exprType() != declType) {
                            PARSE_ERR(
                                @8,
                                (String("Array variable '") + name +
                                "' declared with invalid assignment values. Assigned element " +
                                ToString(i+1) + " is not an " + typeStr(declType) + " expression.").c_str()
                            );
                            argsOK = false;
                            break;
                        } else if (args->arg(i)->asNumber()->unitType()) {
                            PARSE_ERR(
                                @8,
                                (String("Array variable '") + name +
                                "' declared with invalid assignment values. Assigned element " +
                                ToString(i+1) + " contains a unit type, only metric prefixes are allowed for arrays.").c_str()
                            );
                            argsOK = false;
                            break;
                        } else if (args->arg(i)->asNumber()->isFinal()) {
                            PARSE_ERR(
                                @8,
                                (String("Array variable '") + name +
                                "' declared with invalid assignment values. Assigned element " +
                                ToString(i+1) + " declared as 'final' value.").c_str()
                            );
                            argsOK = false;
                            break;
                        }
                    }
                }
                if (argsOK) {
                    if (declType == REAL_EXPR)
                        context->vartable[name] = new RealArrayVariable(context, size, args);
                    else
                        context->vartable[name] = new IntArrayVariable(context, size, args);
                }
            }
        }
        $$ = new NoOperation;
    }
    | DECLARE CONST_ VARIABLE '[' expr ']' ASSIGNMENT '(' args ')'  {
        const char* name = $3;
        if (!$5->isConstExpr()) {
            PARSE_ERR(@5, (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
        } else if ($5->exprType() != INT_EXPR) {
            PARSE_ERR(@5, (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
        } else if (context->variableByName(name)) {
            PARSE_ERR(@3, (String("Redeclaration of variable '") + name + "'.").c_str());
        } else {
            IntExprRef sizeExpr = $5;
            ArgsRef args = $9;
            vmint size = sizeExpr->evalInt();
            if (size <= 0) {
                PARSE_ERR(@5, (String("Array variable '") + name + "' must be declared with positive array size.").c_str());
            } else if (args->argsCount() > size) {
                PARSE_ERR(@9, (String("Array variable '") + name +
                          "' was declared with size " + ToString(size) +
                          " but " + ToString(args->argsCount()) +
                          " values were assigned." ).c_str());
            } else if (sizeExpr->unitType() || sizeExpr->hasUnitFactorNow()) {
                PARSE_ERR(@5, "Units are not allowed as array size.");
            } else {
                if (sizeExpr->isFinal())
                    PARSE_WRN(@5, "Final operator '!' is meaningless here.");
                ExprType_t declType = EMPTY_EXPR;
                if (name[0] == '%') {
                    declType = INT_EXPR;
                } else if (name[0] == '?') {
                    declType = REAL_EXPR;
                } else if (name[0] == '$') {
                    PARSE_ERR(@3, (String("Variable '") + name + "' declaration ambiguous: Use '%' as name prefix for integer arrays instead of '$'.").c_str());
                } else if (name[0] == '~') {
                    PARSE_ERR(@3, (String("Variable '") + name + "' declaration ambiguous: Use '?' as name prefix for real number arrays instead of '~'.").c_str());
                } else {
                    PARSE_ERR(@3, (String("Variable '") + name + "' declared as unknown array type: use either '%' or '?' instead of '" + String(name).substr(0,1) + "'.").c_str());
                }
                bool argsOK = true;
                if (declType == EMPTY_EXPR) {
                    argsOK = false;
                } else {
                    for (vmint i = 0; i < args->argsCount(); ++i) {
                        if (args->arg(i)->exprType() != declType) {
                            PARSE_ERR(
                                @9,
                                (String("const array variable '") + name +
                                "' declared with invalid assignment values. Assigned element " +
                                ToString(i+1) + " is not an " + typeStr(declType) + " expression.").c_str()
                            );
                            argsOK = false;
                            break;
                        }
                        if (!args->arg(i)->isConstExpr()) {
                            PARSE_ERR(
                                @9,
                                (String("const array variable '") + name +
                                "' must be defined with const values. Assigned element " +
                                ToString(i+1) + " is not a const expression though.").c_str()
                            );
                            argsOK = false;
                            break;
                        } else if (args->arg(i)->asNumber()->unitType()) {
                            PARSE_ERR(
                                @9,
                                (String("const array variable '") + name +
                                "' declared with invalid assignment values. Assigned element " +
                                ToString(i+1) + " contains a unit type, only metric prefixes are allowed for arrays.").c_str()
                            );
                            argsOK = false;
                            break;
                        } else if (args->arg(i)->asNumber()->isFinal()) {
                            PARSE_ERR(
                                @9,
                                (String("const array variable '") + name +
                                "' declared with invalid assignment values. Assigned element " +
                                ToString(i+1) + " declared as 'final' value.").c_str()
                            );
                            argsOK = false;
                            break;
                        }
                    }
                }
                if (argsOK) {
                    if (declType == REAL_EXPR)
                        context->vartable[name] = new RealArrayVariable(context, size, args, true);
                    else
                        context->vartable[name] = new IntArrayVariable(context, size, args, true);
                }
            }
        }
        $$ = new NoOperation;
    }
    | DECLARE CONST_ VARIABLE ASSIGNMENT expr  {
        const char* name = $3;
        const ExprType_t declType = exprTypeOfVarName(name);
        if ($5->exprType() == STRING_EXPR) {
            if (name[0] != '@')
                PARSE_WRN(@5, (String("Variable '") + name + "' declared as " + typeStr(declType) + ", string expression assigned though.").c_str());
            String s;
            StringExprRef expr = $5;
            if (expr->isConstExpr())
                s = expr->evalStr();
            else
                PARSE_ERR(@5, (String("Assignment to const string variable '") + name + "' requires const expression.").c_str());
            ConstStringVariableRef var = new ConstStringVariable(context, s);
            context->vartable[name] = var;
            //$$ = new Assignment(var, new StringLiteral(s));
        } else if ($5->exprType() == REAL_EXPR) {
            if (name[0] != '~')
                PARSE_WRN(@5, (String("Variable '") + name + "' declared as " + typeStr(declType) + ", real number expression assigned though.").c_str());
            RealExprRef expr = $5;
            if (!expr->isConstExpr()) {
                PARSE_ERR(@5, (String("Assignment to const real number variable '") + name + "' requires const expression.").c_str());
            }
            ConstRealVariableRef var = new ConstRealVariable(
                #if defined(__GNUC__) && !defined(__clang__)
                (const RealVarDef&) // GCC 8.x requires this cast here (looks like a GCC bug to me); cast would cause an error with clang though
                #endif
            { 
                .value = (expr->isConstExpr()) ? expr->evalReal() : vmfloat(0),
                .unitFactor = (expr->isConstExpr()) ? expr->unitFactor() : VM_NO_FACTOR,
                .unitType = expr->unitType(),
                .isFinal = expr->isFinal()
            });
            context->vartable[name] = var;
            //$$ = new Assignment(var, new IntLiteral(i));
        } else if ($5->exprType() == INT_EXPR) {
            if (name[0] != '$')
                PARSE_WRN(@5, (String("Variable '") + name + "' declared as " + typeStr(declType) + ", integer expression assigned though.").c_str());
            IntExprRef expr = $5;
            if (!expr->isConstExpr()) {
                PARSE_ERR(@5, (String("Assignment to const integer variable '") + name + "' requires const expression.").c_str());
            }
            ConstIntVariableRef var = new ConstIntVariable(
                #if defined(__GNUC__) && !defined(__clang__)
                (const IntVarDef&) // GCC 8.x requires this cast here (looks like a GCC bug to me); cast would cause an error with clang though
                #endif
            {
                .value = (expr->isConstExpr()) ? expr->evalInt() : 0,
                .unitFactor = (expr->isConstExpr()) ? expr->unitFactor() : VM_NO_FACTOR,
                .unitType = expr->unitType(),
                .isFinal = expr->isFinal()
            });
            context->vartable[name] = var;
            //$$ = new Assignment(var, new IntLiteral(i));
        } else if ($5->exprType() == EMPTY_EXPR) {
            PARSE_ERR(@5, "Expression does not result in a value.");
        } else if (isArray($5->exprType())) {
            PARSE_ERR(@5, (String("Variable '") + name + "' declared as scalar type, array expression assigned though.").c_str());
        }
        $$ = new NoOperation();
    }
    | assignment  {
        $$ = $1;
    }
    | WHILE '(' expr ')' opt_statements END WHILE  {
        if ($3->exprType() == INT_EXPR) {
            IntExprRef expr = $3;
            if (expr->asNumber()->unitType() ||
                expr->asNumber()->hasUnitFactorEver())
                PARSE_WRN(@3, "Condition for 'while' loops contains a unit.");
            else if (expr->isFinal() && expr->isConstExpr())
                PARSE_WRN(@3, "Final operator '!' is meaningless here.");
            $$ = new While(expr, $5);
        } else {
            PARSE_ERR(@3, "Condition for 'while' loops must be integer expression.");
            $$ = new While(new IntLiteral({ .value = 0 }), $5);
        }
    }
    | SYNCHRONIZED opt_statements END SYNCHRONIZED  {
        $$ = new SyncBlock($2);
    }
    | IF '(' expr ')' opt_statements ELSE opt_statements END IF  {
        if ($3->exprType() == INT_EXPR) {
            IntExprRef expr = $3;
            if (expr->asNumber()->unitType() ||
                expr->asNumber()->hasUnitFactorEver())
                PARSE_WRN(@3, "Condition for 'if' contains a unit.");
            else if (expr->isFinal() && expr->isConstExpr())
                PARSE_WRN(@3, "Final operator '!' is meaningless here.");
            $$ = new If($3, $5, $7);
        } else {
            PARSE_ERR(@3, "Condition for 'if' must be integer expression.");
            $$ = new If(new IntLiteral({ .value = 0 }), $5, $7);
        }
    }
    | IF '(' expr ')' opt_statements END IF  {
        if ($3->exprType() == INT_EXPR) {
            IntExprRef expr = $3;
            if (expr->asNumber()->unitType() ||
                expr->asNumber()->hasUnitFactorEver())
                PARSE_WRN(@3, "Condition for 'if' contains a unit.");
            else if (expr->isFinal() && expr->isConstExpr())
                PARSE_WRN(@3, "Final operator '!' is meaningless here.");
            $$ = new If($3, $5);
        } else {
            PARSE_ERR(@3, "Condition for 'if' must be integer expression.");
            $$ = new If(new IntLiteral({ .value = 0 }), $5);
        }
    }
    | SELECT expr caseclauses END SELECT  {
        if ($2->exprType() == INT_EXPR) {
            IntExprRef expr = $2;
            if (expr->unitType() || expr->hasUnitFactorEver()) {
                PARSE_ERR(@2, "Units are not allowed here.");
                $$ = new SelectCase(new IntLiteral({ .value = 0 }), $3);
            } else {
                if (expr->isFinal() && expr->isConstExpr())
                    PARSE_WRN(@2, "Final operator '!' is meaningless here.");
                $$ = new SelectCase(expr, $3);
            }
        } else {
            PARSE_ERR(@2, "Statement 'select' can only by applied to integer expressions.");
            $$ = new SelectCase(new IntLiteral({ .value = 0 }), $3);
        }
    }

caseclauses:
    caseclause  {
        $$ = CaseBranches();
        $$.push_back($1);
    }
    | caseclauses caseclause  {
        $$ = $1;
        $$.push_back($2);
    }

caseclause:
    CASE INTEGER opt_statements  {
        $$ = CaseBranch();
        $$.from = new IntLiteral({ .value = $2 });
        $$.statements = $3;
    }
    | CASE INTEGER TO INTEGER opt_statements  {
        $$ = CaseBranch();
        $$.from = new IntLiteral({ .value = $2 });
        $$.to   = new IntLiteral({ .value = $4 });
        $$.statements = $5;
    }

userfunctioncall:
    CALL IDENTIFIER  {
        const char* name = $2;
        StatementsRef fn = context->userFunctionByName(name);
        if (context->functionProvider->functionByName(name)) {
            PARSE_ERR(@1, (String("Keyword 'call' must only be used for user defined functions, not for any built-in function like '") + name + "'.").c_str());
            $$ = StatementsRef();
        } else if (!fn) {
            PARSE_ERR(@2, (String("No user defined function with name '") + name + "'.").c_str());
            $$ = StatementsRef();
        } else {
            $$ = fn;
        }
    }

functioncall:
    IDENTIFIER '(' args ')'  {
        const char* name = $1;
        //printf("function call of '%s' with args\n", name);
        ArgsRef args = $3;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR(@1, (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR(@1, (String("No built-in function with name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP(@$);
            $$ = new NoFunctionCall;
        } else if (args->argsCount() < fn->minRequiredArgs()) {
            PARSE_ERR(@3, (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (args->argsCount() > fn->maxAllowedArgs()) {
            PARSE_ERR(@3, (String("Built-in function '") + name + "' accepts max. " + ToString(fn->maxAllowedArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else {
            bool argsOK = true;
            for (vmint i = 0; i < args->argsCount(); ++i) {
                if (!fn->acceptsArgType(i, args->arg(i)->exprType())) {
                    PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects " + acceptedArgTypesStr(fn, i) + " type, but type " + typeStr(args->arg(i)->exprType()) + " was given instead.").c_str());
                    argsOK = false;
                    break;
                } else if (fn->modifiesArg(i) && !args->arg(i)->isModifyable()) {
                    PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects an assignable variable.").c_str());
                    argsOK = false;
                    break;
                } else if (isNumber(args->arg(i)->exprType()) && !fn->acceptsArgUnitType(i, args->arg(i)->asNumber()->unitType())) {
                    if (args->arg(i)->asNumber()->unitType())
                        PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' does not expect unit " + unitTypeStr(args->arg(i)->asNumber()->unitType()) +  ".").c_str());
                    else
                        PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects a unit.").c_str());
                    argsOK = false;
                    break;
                } else if (isNumber(args->arg(i)->exprType()) && args->arg(i)->asNumber()->hasUnitFactorEver() && !fn->acceptsArgUnitPrefix(i, args->arg(i)->asNumber()->unitType())) {
                    if (args->arg(i)->asNumber()->unitType())
                        PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' does not expect a unit prefix for unit" + unitTypeStr(args->arg(i)->asNumber()->unitType()) + ".").c_str());
                    else
                        PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' does not expect a unit prefix.").c_str());
                    argsOK = false;
                    break;
                } else if (!fn->acceptsArgFinal(i) && isNumber(args->arg(i)->exprType()) && args->arg(i)->asNumber()->isFinal()) {
                    PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' does not expect a \"final\" value.").c_str());
                    argsOK = false;
                    break;
                }
            }
            if (argsOK) {
                // perform built-in function's own, custom arguments checks (if any)
                fn->checkArgs(&*args, [&](String err) {
                    PARSE_ERR(@3, (String("Built-in function '") + name + "()': " + err).c_str());
                    argsOK = false;
                }, [&](String wrn) {
                    PARSE_WRN(@3, (String("Built-in function '") + name + "()': " + wrn).c_str());
                });
            }
            $$ = new FunctionCall(name, args, argsOK ? fn : NULL);
        }
    }
    | IDENTIFIER '(' ')' {
        const char* name = $1;
        //printf("function call of '%s' (with empty args)\n", name);
        ArgsRef args = new Args;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR(@1, (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR(@1, (String("No built-in function with name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP(@$);
            $$ = new NoFunctionCall;
        } else if (fn->minRequiredArgs() > 0) {
            PARSE_ERR(@3, (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else {
            $$ = new FunctionCall(name, args, fn);
        }
    }
    | IDENTIFIER  {
        const char* name = $1;
        //printf("function call of '%s' (without args)\n", name);
        ArgsRef args = new Args;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR(@1, (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR(@1, (String("No built-in function with name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP(@$);
            $$ = new NoFunctionCall;
        } else if (fn->minRequiredArgs() > 0) {
            PARSE_ERR(@1, (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else {
            $$ = new FunctionCall(name, args, fn);
        }
    }

args:
    arg  {
        $$ = new Args();
        $$->add($1);
    }
    | args ',' arg  {
        $$ = $1;
        $$->add($3);
    }

arg:
    expr

assignment:
    VARIABLE ASSIGNMENT expr  {
        //printf("variable lookup with name '%s' as assignment expr\n", $1);
        const char* name = $1;
        VariableRef var = context->variableByName(name);
        if (!var)
            PARSE_ERR(@1, (String("Variable assignment: No variable declared with name '") + name + "'.").c_str());
        else if (var->isConstExpr())
            PARSE_ERR(@2, (String("Variable assignment: Cannot modify const variable '") + name + "'.").c_str());
        else if (!var->isAssignable())
            PARSE_ERR(@2, (String("Variable assignment: Variable '") + name + "' is not assignable.").c_str());
        else if (var->exprType() != $3->exprType())
            PARSE_ERR(@3, (String("Variable assignment: Variable '") + name + "' is of type " + typeStr(var->exprType()) + ", assignment is of type " + typeStr($3->exprType()) + " though.").c_str());
        else if (isNumber(var->exprType())) {
            NumberVariableRef numberVar = var;
            NumberExprRef expr = $3;
            if (numberVar->unitType() != expr->unitType())
                PARSE_ERR(@3, (String("Variable assignment: Variable '") + name + "' has unit type " + unitTypeStr(numberVar->unitType()) + ", assignment has unit type " + unitTypeStr(expr->unitType()) + " though.").c_str());
            else if (numberVar->isFinal() != expr->isFinal())
                PARSE_ERR(@3, (String("Variable assignment: Variable '") + name + "' was declared as " + String(numberVar->isFinal() ? "final" : "not final") + ", assignment is " + String(expr->isFinal() ? "final" : "not final") + " though.").c_str());
        }
        $$ = new Assignment(var, $3);
    }
    | VARIABLE '[' expr ']' ASSIGNMENT expr  {
        const char* name = $1;
        VariableRef var = context->variableByName(name);
        if (!var)
            PARSE_ERR(@1, (String("No variable declared with name '") + name + "'.").c_str());
        else if (!isArray(var->exprType()))
            PARSE_ERR(@2, (String("Variable '") + name + "' is not an array variable.").c_str());
        else if (var->isConstExpr())
            PARSE_ERR(@5, (String("Variable assignment: Cannot modify const array variable '") + name + "'.").c_str());
        else if (!var->isAssignable())
            PARSE_ERR(@5, (String("Variable assignment: Array variable '") + name + "' is not assignable.").c_str());
        else if ($3->exprType() != INT_EXPR)
            PARSE_ERR(@3, (String("Array variable '") + name + "' accessed with non integer expression.").c_str());
        else if ($3->asInt()->unitType())
            PARSE_ERR(@3, "Unit types are not allowed as array index.");
        else if ($6->exprType() != scalarTypeOfArray(var->exprType()))
            PARSE_ERR(@5, (String("Variable '") + name + "' was declared as " + typeStr(var->exprType()) + ", assigned expression is " + typeStr($6->exprType()) + " though.").c_str());
        else if ($6->asNumber()->unitType())
            PARSE_ERR(@6, "Unit types are not allowed for array variables.");
        else if ($6->asNumber()->isFinal())
            PARSE_ERR(@6, "Final operator '!' not allowed for array variables.");
        else if ($3->isConstExpr() && $3->asInt()->evalInt() >= ((ArrayExprRef)var)->arraySize())
            PARSE_WRN(@3, (String("Index ") + ToString($3->asInt()->evalInt()) +
                          " exceeds size of array variable '" + name +
                          "' which was declared with size " +
                          ToString(((ArrayExprRef)var)->arraySize()) + ".").c_str());
        else if ($3->asInt()->isFinal())
            PARSE_WRN(@3, "Final operator '!' is meaningless here.");
        if (var->exprType() == INT_ARR_EXPR) {
            IntArrayElementRef element = new IntArrayElement(var, $3);
            $$ = new Assignment(element, $6);
        } else if (var->exprType() == REAL_ARR_EXPR) {
            RealArrayElementRef element = new RealArrayElement(var, $3);
            $$ = new Assignment(element, $6);
        } else {
            $$ = new NoOperation; // actually not possible to ever get here
        }
    }

unary_expr:
    INTEGER  {
        $$ = new IntLiteral({ .value = $1 });
    }
    | REAL  {
        $$ = new RealLiteral({ .value = $1 });
    }
    | INTEGER_UNIT  {
        IntLiteralRef literal = new IntLiteral({
            .value = $1.iValue,
            .unitFactor = VMUnit::unitFactor($1.prefix),
            .unitType = $1.unit
        });
        $$ = literal;
    }
    | REAL_UNIT  {
        RealLiteralRef literal = new RealLiteral({
            .value = $1.fValue,
            .unitFactor = VMUnit::unitFactor($1.prefix),
            .unitType = $1.unit
        });
        $$ = literal;
    }
    | STRING    {
        $$ = new StringLiteral($1);
    }
    | VARIABLE  {
        //printf("variable lookup with name '%s' as unary expr\n", $1);
        VariableRef var = context->variableByName($1);
        if (var)
            $$ = var;
        else {
            PARSE_ERR(@1, (String("No variable declared with name '") + $1 + "'.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        }
    }
    | VARIABLE '[' expr ']'  {
        const char* name = $1;
        VariableRef var = context->variableByName(name);
        if (!var) {
            PARSE_ERR(@1, (String("No variable declared with name '") + name + "'.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isArray(var->exprType())) {
            PARSE_ERR(@2, (String("Variable '") + name + "' is not an array variable.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if ($3->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Array variable '") + name + "' accessed with non integer expression.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if ($3->asInt()->unitType() || $3->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@3, "Units are not allowed as array index.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if ($3->isConstExpr() && $3->asInt()->evalInt() >= ((ArrayExprRef)var)->arraySize())
                PARSE_WRN(@3, (String("Index ") + ToString($3->asInt()->evalInt()) +
                               " exceeds size of array variable '" + name +
                               "' which was declared with size " +
                               ToString(((ArrayExprRef)var)->arraySize()) + ".").c_str());
            else if ($3->asInt()->isFinal())
                PARSE_WRN(@3, "Final operator '!' is meaningless here.");
            if (var->exprType() == REAL_ARR_EXPR) {
                $$ = new RealArrayElement(var, $3);
            } else {
                $$ = new IntArrayElement(var, $3);
            }
        }
    }
    | '(' expr ')'  {
        $$ = $2;
    }
    | functioncall  {
        $$ = $1;
    }
    | '+' unary_expr  {
        $$ = $2;
    }
    | '-' unary_expr  {
        $$ = new Neg($2);
    }
    | BITWISE_NOT unary_expr  {
        if ($2->exprType() != INT_EXPR) {
            PARSE_ERR(@2, (String("Right operand of bitwise operator '.not.' must be an integer expression, is ") + typeStr($2->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if ($2->asInt()->unitType() || $2->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@2, "Units are not allowed for operands of bitwise operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            $$ = new BitwiseNot($2);
        }
    }
    | NOT unary_expr  {
        if ($2->exprType() != INT_EXPR) {
            PARSE_ERR(@2, (String("Right operand of operator 'not' must be an integer expression, is ") + typeStr($2->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if ($2->asInt()->unitType() || $2->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@2, "Units are not allowed for operands of logical operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            $$ = new Not($2);
        }
    }
    | '!' unary_expr  {
        if (!isNumber($2->exprType())) {
            PARSE_ERR(@2, (String("Right operand of \"final\" operator '!' must be a scalar number expression, is ") + typeStr($2->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            $$ = new Final($2);
        }
    }

expr:
    concat_expr

concat_expr:
    logical_or_expr
    | concat_expr '&' logical_or_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->isConstExpr() && rhs->isConstExpr()) {
            $$ = new StringLiteral(
                lhs->evalCastToStr() + rhs->evalCastToStr()
            );
        } else {
            $$ = new ConcatString(lhs, rhs);
        }
    }

logical_or_expr:
    logical_and_expr
    | logical_or_expr OR logical_and_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator 'or' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator 'or' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asInt()->unitType() || lhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@1, "Units are not allowed for operands of logical operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->asInt()->unitType() || rhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@3, "Units are not allowed for operands of logical operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asInt()->isFinal() && !rhs->asInt()->isFinal())
                PARSE_WRN(@3, "Right operand of 'or' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asInt()->isFinal() && rhs->asInt()->isFinal())
                PARSE_WRN(@1, "Left operand of 'or' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new Or(lhs, rhs);
        }
    }

logical_and_expr:
    bitwise_or_expr  {
        $$ = $1;
    }
    | logical_and_expr AND bitwise_or_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator 'and' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator 'and' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asInt()->unitType() || lhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@1, "Units are not allowed for operands of logical operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->asInt()->unitType() || rhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@3, "Units are not allowed for operands of logical operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asInt()->isFinal() && !rhs->asInt()->isFinal())
                PARSE_WRN(@3, "Right operand of 'and' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asInt()->isFinal() && rhs->asInt()->isFinal())
                PARSE_WRN(@1, "Left operand of 'and' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new And(lhs, rhs);
        }
    }

bitwise_or_expr:
    bitwise_and_expr
    | bitwise_or_expr BITWISE_OR bitwise_and_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of bitwise operator '.or.' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of bitwise operator '.or.' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asInt()->unitType() || lhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@1, "Units are not allowed for operands of bitwise operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->asInt()->unitType() || rhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@3, "Units are not allowed for operands of bitwise operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asInt()->isFinal() && !rhs->asInt()->isFinal())
                PARSE_WRN(@3, "Right operand of '.or.' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asInt()->isFinal() && rhs->asInt()->isFinal())
                PARSE_WRN(@1, "Left operand of '.or.' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new BitwiseOr(lhs, rhs);
        }
    }

bitwise_and_expr:
    rel_expr  {
        $$ = $1;
    }
    | bitwise_and_expr BITWISE_AND rel_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of bitwise operator '.and.' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of bitwise operator '.and.' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asInt()->unitType() || lhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@1, "Units are not allowed for operands of bitwise operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->asInt()->unitType() || rhs->asInt()->hasUnitFactorEver()) {
            PARSE_ERR(@3, "Units are not allowed for operands of bitwise operations.");
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asInt()->isFinal() && !rhs->asInt()->isFinal())
                PARSE_WRN(@3, "Right operand of '.and.' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asInt()->isFinal() && rhs->asInt()->isFinal())
                PARSE_WRN(@1, "Left operand of '.and.' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new BitwiseAnd(lhs, rhs);
        }
    }

rel_expr:
      add_expr
    | rel_expr '<' add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '<' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@3, (String("Right operand of operator '<' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of relative operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '<' comparison is not 'final', left operand is 'final' though.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '<' comparison is not 'final', right operand is 'final' though.");
            $$ = new Relation(lhs, Relation::LESS_THAN, rhs);
        }
    }
    | rel_expr '>' add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '>' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@3, (String("Right operand of operator '>' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of relative operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '>' comparison is not 'final', left operand is 'final' though.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '>' comparison is not 'final', right operand is 'final' though.");
            $$ = new Relation(lhs, Relation::GREATER_THAN, rhs);
        }
    }
    | rel_expr LE add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '<=' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@3, (String("Right operand of operator '<=' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of relative operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '<=' comparison is not 'final', left operand is 'final' though.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '<=' comparison is not 'final', right operand is 'final' though.");
            $$ = new Relation(lhs, Relation::LESS_OR_EQUAL, rhs);
        }
    }
    | rel_expr GE add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '>=' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@3, (String("Right operand of operator '>=' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of relative operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '>=' comparison is not 'final', left operand is 'final' though.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '>=' comparison is not 'final', right operand is 'final' though.");
            $$ = new Relation(lhs, Relation::GREATER_OR_EQUAL, rhs);
        }
    }
    | rel_expr '=' add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '=' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@3, (String("Right operand of operator '=' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of relative operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '=' comparison is not 'final', left operand is 'final' though.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '=' comparison is not 'final', right operand is 'final' though.");
            $$ = new Relation(lhs, Relation::EQUAL, rhs);
        }
    }
    | rel_expr '#' add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '#' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@3, (String("Right operand of operator '#' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of relative operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '#' comparison is not 'final', left operand is 'final' though.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '#' comparison is not 'final', right operand is 'final' though.");
            $$ = new Relation(lhs, Relation::NOT_EQUAL, rhs);
        }
    }

add_expr:
      mul_expr
    | add_expr '+' mul_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '+' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@1, (String("Right operand of operator '+' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->exprType() != rhs->exprType()) {
            PARSE_ERR(@2, (String("Operands of operator '+' must have same type; left operand is ") +
                      typeStr(lhs->exprType()) + " and right operand is " + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of '+' operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '+' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '+' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new Add(lhs,rhs);
        }
    }
    | add_expr '-' mul_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '-' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@1, (String("Right operand of operator '-' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->exprType() != rhs->exprType()) {
            PARSE_ERR(@2, (String("Operands of operator '-' must have same type; left operand is ") +
                      typeStr(lhs->exprType()) + " and right operand is " + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() != rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Operands of '-' operations must have same unit, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '-' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '-' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new Sub(lhs,rhs);
        }
    }

mul_expr:
    unary_expr
    | mul_expr '*' unary_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '*' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@1, (String("Right operand of operator '*' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() && rhs->asNumber()->unitType()) {
            PARSE_ERR(@2, (String("Only one operand of operator '*' may have a unit type, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->exprType() != rhs->exprType()) {
            PARSE_ERR(@2, (String("Operands of operator '*' must have same type; left operand is ") +
                      typeStr(lhs->exprType()) + " and right operand is " + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '*' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '*' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new Mul(lhs,rhs);
        }
    }
    | mul_expr '/' unary_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (!isNumber(lhs->exprType())) {
            PARSE_ERR(@1, (String("Left operand of operator '/' must be a scalar number expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!isNumber(rhs->exprType())) {
            PARSE_ERR(@1, (String("Right operand of operator '/' must be a scalar number expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->asNumber()->unitType() && rhs->asNumber()->unitType() &&
                   lhs->asNumber()->unitType() != rhs->asNumber()->unitType())
        {
            PARSE_ERR(@2, (String("Operands of operator '/' with two different unit types, left operand is ") +
                unitTypeStr(lhs->asNumber()->unitType()) + " and right operand is " +
                unitTypeStr(rhs->asNumber()->unitType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (!lhs->asNumber()->unitType() && rhs->asNumber()->unitType()) {
            PARSE_ERR(@3, ("Dividing left operand without any unit type by right operand with unit type (" +
                unitTypeStr(rhs->asNumber()->unitType()) + ") is not possible.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (lhs->exprType() != rhs->exprType()) {
            PARSE_ERR(@2, (String("Operands of operator '/' must have same type; left operand is ") +
                      typeStr(lhs->exprType()) + " and right operand is " + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asNumber()->isFinal() && !rhs->asNumber()->isFinal())
                PARSE_WRN(@3, "Right operand of '/' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asNumber()->isFinal() && rhs->asNumber()->isFinal())
                PARSE_WRN(@1, "Left operand of '/' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new Div(lhs,rhs);
        }
    }
    | mul_expr MOD unary_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of modulo operator must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of modulo operator must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral({ .value = 0 });
        } else {
            if (lhs->asInt()->unitType() || lhs->asInt()->hasUnitFactorEver())
                PARSE_ERR(@1, "Operands of modulo operator must not use any unit.");
            if (rhs->asInt()->unitType() || rhs->asInt()->hasUnitFactorEver())
                PARSE_ERR(@3, "Operands of modulo operator must not use any unit.");
            if (lhs->asInt()->isFinal() && !rhs->asInt()->isFinal())
                PARSE_WRN(@3, "Right operand of 'mod' operation is not 'final', result will be 'final' though since left operand is 'final'.");
            else if (!lhs->asInt()->isFinal() && rhs->asInt()->isFinal())
                PARSE_WRN(@1, "Left operand of 'mod' operation is not 'final', result will be 'final' though since right operand is 'final'.");
            $$ = new Mod(lhs,rhs);
        }
    }

%%

void InstrScript_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err) {
    //fprintf(stderr, "%d: %s\n", locp->first_line, err);
    context->addErr(locp->first_line, locp->last_line, locp->first_column+1, locp->last_column+1, err);
}

void InstrScript_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt) {
    //fprintf(stderr, "WRN %d: %s\n", locp->first_line, txt);
    context->addWrn(locp->first_line, locp->last_line, locp->first_column+1, locp->last_column+1, txt);
}

/// Custom implementation of yytnamerr() to ensure quotation is always stripped from token names before printing them to error messages.
int InstrScript_tnamerr(char* yyres, const char* yystr) {
  if (*yystr == '"') {
      int yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
/*
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
*/
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
/*
    do_not_strip_quotes: ;
*/
    }

  if (! yyres)
    return (int) yystrlen (yystr);

  return int( yystpcpy (yyres, yystr) - yyres );
}
