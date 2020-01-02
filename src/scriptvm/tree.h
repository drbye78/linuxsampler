/*                                                              -*- c++ -*-
 *
 * Copyright (c) 2014 - 2019 Christian Schoenebeck and Andreas Persson
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// This header defines VM core implementation internal data types only used
// inside the parser and core VM implementation of this source directory. Not
// intended to be used in other source code parts (other source code
// directories) of the sampler.

#ifndef LS_INSTRPARSERTREE_H
#define LS_INSTRPARSERTREE_H

#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <string.h> // for memset()
#include <assert.h>
#include "../common/global.h"
#include "../common/Ref.h"
#include "../common/ArrayList.h"
#include "common.h"

namespace LinuxSampler {
    
class ParserContext;
class ExecContext;

enum StmtType_t {
    STMT_LEAF,
    STMT_LIST,
    STMT_BRANCH,
    STMT_LOOP,
    STMT_SYNC,
    STMT_NOOP,
};

/**
 * Convenience function used for retrieving the (assumed) data type of a given
 * script variable name.
 *
 * @param name - some script variable name (e.g. "$foo")
 * @return variable's assumed data type (e.g. INT_EXPR for example above)
 */
inline ExprType_t exprTypeOfVarName(const String& name) {
    if (name.empty()) return (ExprType_t) -1;
    const char prefix = name[0];
    switch (prefix) {
        case '$': return INT_EXPR;
        case '%': return INT_ARR_EXPR;
        case '~': return REAL_EXPR;
        case '?': return REAL_ARR_EXPR;
        case '@': return STRING_EXPR;
        case '!': return STRING_ARR_EXPR;
    }
    return (ExprType_t) -1;
}

inline ExprType_t scalarTypeOfArray(ExprType_t arrayType) {
    if (arrayType == INT_ARR_EXPR) return INT_EXPR;
    if (arrayType == REAL_ARR_EXPR) return REAL_EXPR;
    if (arrayType == STRING_ARR_EXPR) return STRING_EXPR;
    assert(false);
    return EMPTY_EXPR; // just to shut up the compiler
}

/**
 * Used by parser for parser error messages to provide a text with all data
 * types accepted by the given built-in function @a fn for the respective
 * function argument @a iArg.
 */
String acceptedArgTypesStr(VMFunction* fn, vmint iArg);

class Node {
public:
    Node();
    virtual ~Node();
    virtual void dump(int level = 0) = 0;
    virtual bool isPolyphonic() const = 0;
    void printIndents(int n);
};
typedef Ref<Node> NodeRef;

class Expression : virtual public VMExpr, virtual public Node {
public:
    virtual ExprType_t exprType() const = 0;
    virtual bool isConstExpr() const = 0;
    virtual String evalCastToStr() = 0;
};
typedef Ref<Expression,Node> ExpressionRef;

class Unit : virtual public VMUnit, virtual public Node {
    StdUnit_t unit;
public:
    Unit(StdUnit_t type) : unit(type) {}
    StdUnit_t unitType() const OVERRIDE { return unit; }
    static vmint convIntToUnitFactor(vmint iValue, VMUnit* srcUnit, VMUnit* dstUnit);
    static vmint convIntToUnitFactor(vmint iValue, vmfloat srcFactor, vmfloat dstFactor);
    static vmfloat convRealToUnitFactor(vmfloat fValue, VMUnit* srcUnit, VMUnit* dstUnit);
    static vmfloat convRealToUnitFactor(vmfloat fValue, vmfloat srcFactor, vmfloat dstFactor);
};
typedef Ref<Unit,Node> UnitRef;

class NumberExpr : virtual public Unit, virtual public VMNumberExpr, virtual public Expression {
public:
};
typedef Ref<NumberExpr,Node> NumberExprRef;

class IntExpr : virtual public NumberExpr, virtual public VMIntExpr {
public:
    ExprType_t exprType() const OVERRIDE { return INT_EXPR; }
    vmint evalIntToUnitFactor(vmfloat unitFactor);
    String evalCastToStr() OVERRIDE;
};
typedef Ref<IntExpr,Node> IntExprRef;

class RealExpr : virtual public NumberExpr, virtual public VMRealExpr  {
public:
    ExprType_t exprType() const OVERRIDE { return REAL_EXPR; }
    vmfloat evalRealToUnitFactor(vmfloat unitFactor);
    String evalCastToStr() OVERRIDE;
};
typedef Ref<RealExpr,Node> RealExprRef;

class ArrayExpr : virtual public VMArrayExpr, virtual public Expression {
public:
};
typedef Ref<ArrayExpr,Node> ArrayExprRef;

class IntArrayExpr : virtual public VMIntArrayExpr, virtual public ArrayExpr {
public:
    ExprType_t exprType() const OVERRIDE { return INT_ARR_EXPR; }
    String evalCastToStr() OVERRIDE;
};
typedef Ref<IntArrayExpr,Node> IntArrayExprRef;

class RealArrayExpr : virtual public VMRealArrayExpr, virtual public ArrayExpr {
public:
    ExprType_t exprType() const OVERRIDE { return REAL_ARR_EXPR; }
    String evalCastToStr() OVERRIDE;
};
typedef Ref<RealArrayExpr,Node> RealArrayExprRef;

class StringExpr : virtual public VMStringExpr, virtual public Expression {
public:
    ExprType_t exprType() const OVERRIDE { return STRING_EXPR; }
    String evalCastToStr() OVERRIDE { return evalStr(); }
};
typedef Ref<StringExpr,Node> StringExprRef;

struct IntLitDef {
    vmint value; //NOTE: sequence matters! Since this is usually initialized with VMIntExpr::evalInt() it should be before member unitFactor, since the latter is usually initialized with VMIntExpr::unitFactor() which does not evaluate the expression.
    vmfloat unitFactor = VM_NO_FACTOR;
    StdUnit_t unitType = VM_NO_UNIT;
    bool isFinal;
};

class IntLiteral FINAL : public IntExpr {
    bool finalVal;
    vmfloat unitPrefixFactor;
    vmint value;
public:
    IntLiteral(const IntLitDef& def);
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return unitPrefixFactor; }
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE { return true; }
    bool isPolyphonic() const OVERRIDE { return false; }
    bool isFinal() const OVERRIDE { return finalVal; }
};
typedef Ref<IntLiteral,Node> IntLiteralRef;

struct RealLitDef {
    vmfloat value; //NOTE: sequence matters! Since this is usually initialized with VMRealExpr::evalReal() it should be before member unitFactor, since the latter is usually initialized with VMRealExpr::unitFactor() which does not evaluate the expression.
    vmfloat unitFactor = VM_NO_FACTOR;
    StdUnit_t unitType = VM_NO_UNIT;
    bool isFinal;
};

class RealLiteral FINAL : public RealExpr {
    bool finalVal;
    vmfloat unitPrefixFactor;
    vmfloat value;
public:
    RealLiteral(const RealLitDef& def);
    vmfloat evalReal() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return unitPrefixFactor; }
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE { return true; }
    bool isPolyphonic() const OVERRIDE { return false; }
    bool isFinal() const OVERRIDE { return finalVal; }
};
typedef Ref<RealLiteral,Node> RealLiteralRef;

class StringLiteral FINAL : public StringExpr {
    String value;
public:
    StringLiteral(const String& value) : value(value) { }
    bool isConstExpr() const OVERRIDE { return true; }
    void dump(int level = 0) OVERRIDE;
    String evalStr() OVERRIDE { return value; }
    bool isPolyphonic() const OVERRIDE { return false; }
};
typedef Ref<StringLiteral,Node> StringLiteralRef;

class Args FINAL : virtual public VMFnArgs, virtual public Node {
public:
    std::vector<ExpressionRef> args;
    void add(ExpressionRef arg) { args.push_back(arg); }
    void dump(int level = 0) OVERRIDE;
    vmint argsCount() const OVERRIDE { return (vmint) args.size(); }
    VMExpr* arg(vmint i) OVERRIDE { return (i >= 0 && i < argsCount()) ? &*args.at(i) : NULL; }
    bool isPolyphonic() const OVERRIDE;
};
typedef Ref<Args,Node> ArgsRef;

struct VariableDecl {
    ParserContext* ctx;
    bool isPolyphonic;
    bool isConst;
    vmint elements = 1;
    vmint memPos;
    vmint unitFactorMemPos;
    StdUnit_t unitType = VM_NO_UNIT;
    bool isFinal;
};

class Variable : virtual public VMVariable, virtual public Expression {
public:
    bool isConstExpr() const OVERRIDE { return bConst; }
    bool isAssignable() const OVERRIDE { return !bConst; }
    virtual void assign(Expression* expr) = 0;
    void assignExpr(VMExpr* expr) OVERRIDE { Expression* e = dynamic_cast<Expression*>(expr); if (e) assign(e); }
protected:
    Variable(const VariableDecl& decl);

    ParserContext* context;
    vmint memPos;
    bool bConst;
};
typedef Ref<Variable,Node> VariableRef;

class NumberVariable : public Variable, virtual public NumberExpr {
    bool polyphonic;
    bool finalVal;
protected:
    vmint unitFactorMemPos;
public:
    bool isPolyphonic() const OVERRIDE { return polyphonic; }
    bool isFinal() const OVERRIDE { return finalVal; }
    vmfloat unitFactor() const OVERRIDE;
protected:
    NumberVariable(const VariableDecl& decl);
};
typedef Ref<NumberVariable,Node> NumberVariableRef;

class IntVariable : public NumberVariable, virtual public IntExpr {
public:
    IntVariable(const VariableDecl& decl);
    void assign(Expression* expr) OVERRIDE;
    vmint evalInt() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<IntVariable,Node> IntVariableRef;

class RealVariable : public NumberVariable, virtual public RealExpr {
public:
    RealVariable(const VariableDecl& decl);
    void assign(Expression* expr) OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<RealVariable,Node> RealVariableRef;

struct IntVarDef /* : VariableDecl*/ { //NOTE: derived, aggregate initializer-lists requires C++17
    // additions for RealVarDef
    vmint value = 0; //NOTE: sequence matters! Since this is usually initialized with VMIntExpr::evalInt() it should be before member unitFactor, since the latter is usually initialized with VMIntExpr::unitFactor() which does not evaluate the expression.
    vmfloat unitFactor = VM_NO_FACTOR;
    // copied from VariableDecl
    ParserContext* ctx;
    bool isPolyphonic;
    bool isConst;
    vmint elements = 1;
    vmint memPos;
    vmint unitFactorMemPos;
    StdUnit_t unitType = VM_NO_UNIT;
    bool isFinal;
};

class ConstIntVariable FINAL : public IntVariable {
    vmfloat unitPrefixFactor;
    vmint value;
public:
    ConstIntVariable(const IntVarDef& def);
    void assign(Expression* expr) OVERRIDE;
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return unitPrefixFactor; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<ConstIntVariable,Node> ConstIntVariableRef;

struct RealVarDef /* : VariableDecl*/ { //NOTE: derived, aggregate initializer-lists requires C++17
    // additions for RealVarDef
    vmfloat value = vmfloat(0); //NOTE: sequence matters! Since this is usually initialized with VMRealExpr::evalReal() it should be before member unitFactor, since the latter is usually initialized with VMRealExpr::unitFactor() which does not evaluate the expression.
    vmfloat unitFactor = VM_NO_FACTOR;
    // copied from VariableDecl
    ParserContext* ctx;
    bool isPolyphonic;
    bool isConst;
    vmint elements = 1;
    vmint memPos;
    vmint unitFactorMemPos;
    StdUnit_t unitType = VM_NO_UNIT;
    bool isFinal;
};

class ConstRealVariable FINAL : public RealVariable {
    vmfloat unitPrefixFactor;
    vmfloat value;
public:
    ConstRealVariable(const RealVarDef& def);
    void assign(Expression* expr) OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return unitPrefixFactor; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<ConstRealVariable,Node> ConstRealVariableRef;

class BuiltInIntVariable FINAL : public IntVariable {
    String name;
    VMIntPtr* ptr;
public:
    BuiltInIntVariable(const String& name, VMIntPtr* ptr);
    bool isAssignable() const OVERRIDE { return ptr->isAssignable(); }
    void assign(Expression* expr) OVERRIDE;
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_UNIT; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<BuiltInIntVariable,Node> BuiltInIntVariableRef;

class PolyphonicIntVariable FINAL : public IntVariable {
public:
    PolyphonicIntVariable(const VariableDecl& decl);
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<PolyphonicIntVariable,Node> PolyphonicIntVariableRef;

class PolyphonicRealVariable FINAL : public RealVariable {
public:
    PolyphonicRealVariable(const VariableDecl& decl);
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<PolyphonicRealVariable,Node> PolyphonicRealVariableRef;

class IntArrayVariable : public Variable, virtual public IntArrayExpr {
    ArrayList<vmint> values;
    ArrayList<vmfloat> unitFactors;
public:
    IntArrayVariable(ParserContext* ctx, vmint size);
    IntArrayVariable(ParserContext* ctx, vmint size, ArgsRef values, bool _bConst = false);
    void assign(Expression* expr) OVERRIDE {} // ignore scalar assignment
    ExprType_t exprType() const OVERRIDE { return INT_ARR_EXPR; }
    virtual vmint arraySize() const OVERRIDE { return values.size(); }
    virtual vmint evalIntElement(vmuint i) OVERRIDE;
    virtual void assignIntElement(vmuint i, vmint value) OVERRIDE;
    vmfloat unitFactorOfElement(vmuint i) const OVERRIDE;
    void assignElementUnitFactor(vmuint i, vmfloat factor) OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return false; }
protected:
    IntArrayVariable(ParserContext* ctx, bool bConst);
};
typedef Ref<IntArrayVariable,Node> IntArrayVariableRef;

class RealArrayVariable FINAL : public Variable, virtual public RealArrayExpr {
    ArrayList<vmfloat> values;
    ArrayList<vmfloat> unitFactors;
public:
    RealArrayVariable(ParserContext* ctx, vmint size);
    RealArrayVariable(ParserContext* ctx, vmint size, ArgsRef values, bool _bConst = false);
    void assign(Expression* expr) OVERRIDE {} // ignore scalar assignment
    ExprType_t exprType() const OVERRIDE { return REAL_ARR_EXPR; }
    virtual vmint arraySize() const OVERRIDE { return values.size(); }
    virtual vmfloat evalRealElement(vmuint i) OVERRIDE;
    virtual void assignRealElement(vmuint i, vmfloat value) OVERRIDE;
    vmfloat unitFactorOfElement(vmuint i) const OVERRIDE;
    void assignElementUnitFactor(vmuint i, vmfloat factor) OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return false; }
protected:
    RealArrayVariable(ParserContext* ctx, bool bConst);
};
typedef Ref<RealArrayVariable,Node> RealArrayVariableRef;

class BuiltInIntArrayVariable FINAL : public IntArrayVariable {
    String name;
    VMInt8Array* array;
public:
    BuiltInIntArrayVariable(const String& name, VMInt8Array* array);
    vmint arraySize() const OVERRIDE { return array->size; }
    vmint evalIntElement(vmuint i) OVERRIDE;
    void assignIntElement(vmuint i, vmint value) OVERRIDE;
    vmfloat unitFactorOfElement(vmuint i) const OVERRIDE { return VM_NO_FACTOR; }
    void assignElementUnitFactor(vmuint i, vmfloat factor) OVERRIDE {}
    bool isAssignable() const OVERRIDE { return !array->readonly; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<BuiltInIntArrayVariable,Node> BuiltInIntArrayVariableRef;

class IntArrayElement FINAL : public IntVariable {
    IntArrayExprRef array;
    IntExprRef index;
    vmint currentIndex;
public:
    IntArrayElement(IntArrayExprRef array, IntExprRef arrayIndex);
    void assign(Expression* expr) OVERRIDE;
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<IntArrayElement,Node> IntArrayElementRef;

class RealArrayElement FINAL : public RealVariable {
    RealArrayExprRef array;
    IntExprRef index;
    vmint currentIndex;
public:
    RealArrayElement(RealArrayExprRef array, IntExprRef arrayIndex);
    void assign(Expression* expr) OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<RealArrayElement,Node> RealArrayElementRef;

class StringVariable : public Variable, virtual public StringExpr {
public:
    StringVariable(ParserContext* ctx);
    void assign(Expression* expr) OVERRIDE;
    String evalStr() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return false; }
protected:
    StringVariable(ParserContext* ctx, bool bConst);
};
typedef Ref<StringVariable,Node> StringVariableRef;

class ConstStringVariable FINAL : public StringVariable {
    String value;
public:
    ConstStringVariable(ParserContext* ctx, String value = "");
    void assign(Expression* expr) OVERRIDE;
    String evalStr() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<ConstStringVariable,Node> ConstStringVariableRef;

class BinaryOp : virtual public Expression {
protected:
    ExpressionRef lhs;
    ExpressionRef rhs;
public:
    BinaryOp(ExpressionRef lhs, ExpressionRef rhs) : lhs(lhs), rhs(rhs) { }
    bool isConstExpr() const OVERRIDE { return lhs->isConstExpr() && rhs->isConstExpr(); }
    bool isPolyphonic() const OVERRIDE { return lhs->isPolyphonic() || rhs->isPolyphonic(); }
};
typedef Ref<BinaryOp,Node> BinaryOpRef;

class NumberBinaryOp : public BinaryOp, virtual public NumberExpr {
public:
    NumberBinaryOp(NumberExprRef lhs, NumberExprRef rhs) : BinaryOp(lhs, rhs) { }
    bool isFinal() const OVERRIDE;
};
typedef Ref<NumberBinaryOp,Node> NumberBinaryOpRef;

class IntBinaryOp : public NumberBinaryOp, virtual public IntExpr {
public:
    IntBinaryOp(IntExprRef lhs, IntExprRef rhs) : NumberBinaryOp(lhs, rhs) { }
};
typedef Ref<IntBinaryOp,Node> IntBinaryOpRef;

class VaritypeScalarBinaryOp : public NumberBinaryOp, virtual public IntExpr, virtual public RealExpr {
public:
    VaritypeScalarBinaryOp(NumberExprRef lhs, NumberExprRef rhs) : NumberBinaryOp(lhs, rhs) { }
    ExprType_t exprType() const OVERRIDE;
    String evalCastToStr() OVERRIDE;
};
typedef Ref<VaritypeScalarBinaryOp,Node> VaritypeScalarBinaryOpRef;

class Add FINAL : public VaritypeScalarBinaryOp {
public:
    Add(NumberExprRef lhs, NumberExprRef rhs);
    vmint evalInt() OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<Add,Node> AddRef;

class Sub FINAL : public VaritypeScalarBinaryOp {
public:
    Sub(NumberExprRef lhs, NumberExprRef rhs);
    vmint evalInt() OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<Sub,Node> SubRef;

class Mul FINAL : public VaritypeScalarBinaryOp {
public:
    Mul(NumberExprRef lhs, NumberExprRef rhs);
    vmint evalInt() OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<Mul,Node> MulRef;

class Div FINAL : public VaritypeScalarBinaryOp {
public:
    Div(NumberExprRef lhs, NumberExprRef rhs);
    vmint evalInt() OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    vmfloat unitFactor() const OVERRIDE;
};
typedef Ref<Div,Node> DivRef;

class Mod FINAL : public IntBinaryOp {
public:
    Mod(IntExprRef lhs, IntExprRef rhs) : IntBinaryOp(lhs, rhs), Unit(VM_NO_UNIT) { }
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<Mod,Node> ModRef;

class Statement : virtual public Node {
public:
    virtual StmtType_t statementType() const = 0;
};
typedef Ref<Statement,Node> StatementRef;

// Just used by parser to avoid "not a statement" parser warning, will be
// filtered out by parser. So it will not be part of the VM tree after parsing.
class NoOperation FINAL : public Statement {
public:
    NoOperation() : Statement() {}
    StmtType_t statementType() const OVERRIDE { return STMT_NOOP; }
    void dump(int level = 0) OVERRIDE {}
    bool isPolyphonic() const OVERRIDE { return false; }
};
typedef Ref<NoOperation,Node> NoOperationRef;

bool isNoOperation(StatementRef statement);

class LeafStatement : public Statement {
public:
    virtual StmtFlags_t exec() = 0;
    StmtType_t statementType() const OVERRIDE { return STMT_LEAF; }
};
typedef Ref<LeafStatement,Node> LeafStatementRef;

class Statements : public Statement {
    std::vector<StatementRef> args;
public:
    void add(StatementRef arg) { args.push_back(arg); }
    void dump(int level = 0) OVERRIDE;
    StmtType_t statementType() const OVERRIDE { return STMT_LIST; }
    virtual Statement* statement(uint i);
    bool isPolyphonic() const OVERRIDE;
};
typedef Ref<Statements,Node> StatementsRef;

class BranchStatement : public Statement {
public:
    StmtType_t statementType() const OVERRIDE { return STMT_BRANCH; }
    virtual vmint evalBranch() = 0;
    virtual Statements* branch(vmuint i) const = 0;
};

class DynamicVariableCall FINAL : public Variable, virtual public IntExpr, virtual public StringExpr, virtual public IntArrayExpr {
    VMDynVar* dynVar;
    String varName;
public:
    DynamicVariableCall(const String& name, ParserContext* ctx, VMDynVar* v);
    ExprType_t exprType() const OVERRIDE { return dynVar->exprType(); }
    bool isConstExpr() const OVERRIDE { return dynVar->isConstExpr(); }
    bool isAssignable() const OVERRIDE { return dynVar->isAssignable(); }
    bool isPolyphonic() const OVERRIDE { return false; }
    void assign(Expression* expr) OVERRIDE { dynVar->assignExpr(expr); }
    VMIntArrayExpr* asIntArray() const OVERRIDE { return dynVar->asIntArray(); }
    vmint evalInt() OVERRIDE;
    String evalStr() OVERRIDE;
    String evalCastToStr() OVERRIDE;
    vmint arraySize() const OVERRIDE { return dynVar->asIntArray()->arraySize(); }
    vmint evalIntElement(vmuint i) OVERRIDE { return dynVar->asIntArray()->evalIntElement(i); }
    void assignIntElement(vmuint i, vmint value) OVERRIDE { return dynVar->asIntArray()->assignIntElement(i, value); }
    vmfloat unitFactorOfElement(vmuint i) const OVERRIDE { return VM_NO_FACTOR; }
    void assignElementUnitFactor(vmuint i, vmfloat factor) OVERRIDE {}
    void dump(int level = 0) OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    bool isFinal() const OVERRIDE { return false; }
};
typedef Ref<DynamicVariableCall,Node> DynamicVariableCallRef;

class FunctionCall : virtual public LeafStatement, virtual public IntExpr, virtual public RealExpr, virtual public StringExpr {
    String functionName;
    ArgsRef args;
    VMFunction* fn;
    VMFnResult* result;
public:
    FunctionCall(const char* function, ArgsRef args, VMFunction* fn);
    void dump(int level = 0) OVERRIDE;
    StmtFlags_t exec() OVERRIDE;
    vmint evalInt() OVERRIDE;
    vmfloat evalReal() OVERRIDE;
    VMIntArrayExpr* asIntArray() const OVERRIDE;
    VMRealArrayExpr* asRealArray() const OVERRIDE;
    String evalStr() OVERRIDE;
    bool isConstExpr() const OVERRIDE { return false; }
    ExprType_t exprType() const OVERRIDE;
    String evalCastToStr() OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return args->isPolyphonic(); }
    vmfloat unitFactor() const OVERRIDE;
    bool isFinal() const OVERRIDE;
protected:
    VMFnResult* execVMFn();
};
typedef Ref<FunctionCall,Node> FunctionCallRef;

class NoFunctionCall FINAL : public FunctionCall {
public:
    NoFunctionCall() : FunctionCall("nothing", new Args, NULL), Unit(VM_NO_UNIT) {}
    StmtType_t statementType() const OVERRIDE { return STMT_NOOP; }
};
typedef Ref<NoFunctionCall,Node> NoFunctionCallRef;

class EventHandler : virtual public Statements, virtual public VMEventHandler {
    StatementsRef statements;
    bool usingPolyphonics;
public:
    void dump(int level = 0) OVERRIDE;
    StmtFlags_t exec();
    EventHandler(StatementsRef statements);
    Statement* statement(uint i) OVERRIDE { return statements->statement(i); }
    bool isPolyphonic() const OVERRIDE { return usingPolyphonics; }
};
typedef Ref<EventHandler,Node> EventHandlerRef;

class OnNote FINAL : public EventHandler {
public:
    OnNote(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const OVERRIDE { return VM_EVENT_HANDLER_NOTE; }
    String eventHandlerName() const OVERRIDE { return "note"; }
};
typedef Ref<OnNote,Node> OnNoteRef;

class OnInit FINAL : public EventHandler {
public:
    OnInit(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const OVERRIDE { return VM_EVENT_HANDLER_INIT; }
    String eventHandlerName() const OVERRIDE { return "init"; }
};
typedef Ref<OnInit,Node> OnInitRef;

class OnRelease FINAL : public EventHandler {
public:
    OnRelease(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const OVERRIDE { return VM_EVENT_HANDLER_RELEASE; }
    String eventHandlerName() const OVERRIDE { return "release"; }
};
typedef Ref<OnRelease,Node> OnReleaseRef;

class OnController FINAL : public EventHandler {
public:
    OnController(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const OVERRIDE { return VM_EVENT_HANDLER_CONTROLLER; }
    String eventHandlerName() const OVERRIDE { return "controller"; }
};
typedef Ref<OnController,Node> OnControllerRef;

class EventHandlers FINAL : virtual public Node {
    std::vector<EventHandlerRef> args;
public:
    EventHandlers();
    ~EventHandlers();
    void add(EventHandlerRef arg);
    void dump(int level = 0) OVERRIDE;
    EventHandler* eventHandlerByName(const String& name) const;
    EventHandler* eventHandler(uint index) const;
    inline uint size() const { return (int) args.size(); }
    bool isPolyphonic() const OVERRIDE;
};
typedef Ref<EventHandlers,Node> EventHandlersRef;

class Assignment FINAL : public LeafStatement {
protected:
    VariableRef variable;
    ExpressionRef value;
public:
    Assignment(VariableRef variable, ExpressionRef value);
    void dump(int level = 0) OVERRIDE;
    StmtFlags_t exec() OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return (variable && variable->isPolyphonic()) || (value && value->isPolyphonic()); }
};
typedef Ref<Assignment,Node> AssignmentRef;

class If FINAL : public BranchStatement {
    IntExprRef condition;
    StatementsRef ifStatements;
    StatementsRef elseStatements;
public:
    If(IntExprRef condition, StatementsRef ifStatements, StatementsRef elseStatements) :
        condition(condition), ifStatements(ifStatements), elseStatements(elseStatements) { }
    If(IntExprRef condition, StatementsRef statements) :
        condition(condition), ifStatements(statements) { }
    void dump(int level = 0) OVERRIDE;
    vmint evalBranch() OVERRIDE;
    Statements* branch(vmuint i) const OVERRIDE;
    bool isPolyphonic() const OVERRIDE;
};
typedef Ref<If,Node> IfRef;

struct CaseBranch FINAL {
    IntExprRef from;
    IntExprRef to;
    StatementsRef statements;
};
typedef std::vector<CaseBranch> CaseBranches;

class SelectCase FINAL : public BranchStatement {
    IntExprRef select;
    CaseBranches branches;
public:
    SelectCase(IntExprRef select, const CaseBranches& branches) : select(select), branches(branches) { }
    void dump(int level = 0) OVERRIDE;
    vmint evalBranch() OVERRIDE;
    Statements* branch(vmuint i) const OVERRIDE;
    bool isPolyphonic() const OVERRIDE;
};
typedef Ref<SelectCase,Node> SelectCaseRef;

class While FINAL : public Statement {
    IntExprRef m_condition;
    StatementsRef m_statements;
public:
    While(IntExprRef condition, StatementsRef statements) :
        m_condition(condition), m_statements(statements) {}
    StmtType_t statementType() const OVERRIDE { return STMT_LOOP; }
    void dump(int level = 0) OVERRIDE;
    bool evalLoopStartCondition();
    Statements* statements() const;
    bool isPolyphonic() const OVERRIDE { return m_condition->isPolyphonic() || m_statements->isPolyphonic(); }
};

class SyncBlock FINAL : public Statement {
    StatementsRef m_statements;
public:
    SyncBlock(StatementsRef statements) : m_statements(statements) {}
    StmtType_t statementType() const OVERRIDE { return STMT_SYNC; }
    void dump(int level = 0) OVERRIDE;
    Statements* statements() const;
    bool isPolyphonic() const OVERRIDE { return m_statements->isPolyphonic(); }
};
typedef Ref<SyncBlock,Node> SyncBlockRef;

class Neg FINAL : virtual public IntExpr, virtual public RealExpr {
    NumberExprRef expr;
public:
    Neg(NumberExprRef expr) : Unit(expr->unitType()), expr(expr) { }
    ExprType_t exprType() const OVERRIDE { return expr->exprType(); }
    vmint evalInt() OVERRIDE { return (expr) ? -(expr->asInt()->evalInt()) : 0; }
    vmfloat evalReal() OVERRIDE { return (expr) ? -(expr->asReal()->evalReal()) : vmfloat(0); }
    String evalCastToStr() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE { return expr->isConstExpr(); }
    bool isPolyphonic() const OVERRIDE { return expr->isPolyphonic(); }
    vmfloat unitFactor() const OVERRIDE { return expr->unitFactor(); }
    bool isFinal() const OVERRIDE { return expr->isFinal(); }
};
typedef Ref<Neg,Node> NegRef;

class ConcatString FINAL : public StringExpr {
    ExpressionRef lhs;
    ExpressionRef rhs;
public:
    ConcatString(ExpressionRef lhs, ExpressionRef rhs) : lhs(lhs), rhs(rhs) {}
    String evalStr() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return lhs->isPolyphonic() || rhs->isPolyphonic(); }
};
typedef Ref<ConcatString,Node> ConcatStringRef;

class Relation FINAL : public IntExpr {
public:
    enum Type {
        LESS_THAN,
        GREATER_THAN,
        LESS_OR_EQUAL,
        GREATER_OR_EQUAL,
        EQUAL,
        NOT_EQUAL
    };
    Relation(ExpressionRef lhs, Type type, ExpressionRef rhs);
    vmint evalInt() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return lhs->isPolyphonic() || rhs->isPolyphonic(); }
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    bool isFinal() const OVERRIDE { return false; }
private:
    ExpressionRef lhs;
    ExpressionRef rhs;
    Type type;
};
typedef Ref<Relation,Node> RelationRef;

class Or FINAL : public IntBinaryOp {
public:
    Or(IntExprRef lhs, IntExprRef rhs) : IntBinaryOp(lhs,rhs), Unit(VM_NO_UNIT) {}
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<Or,Node> OrRef;

class BitwiseOr FINAL : public IntBinaryOp {
public:
    BitwiseOr(IntExprRef lhs, IntExprRef rhs) : IntBinaryOp(lhs,rhs), Unit(VM_NO_UNIT) {}
    vmint evalInt() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
};
typedef Ref<BitwiseOr,Node> BitwiseOrRef;

class And FINAL : public IntBinaryOp {
public:
    And(IntExprRef lhs, IntExprRef rhs) : IntBinaryOp(lhs,rhs), Unit(VM_NO_UNIT) {}
    vmint evalInt() OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<And,Node> AndRef;

class BitwiseAnd FINAL : public IntBinaryOp {
public:
    BitwiseAnd(IntExprRef lhs, IntExprRef rhs) : IntBinaryOp(lhs,rhs), Unit(VM_NO_UNIT) {}
    vmint evalInt() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
};
typedef Ref<BitwiseAnd,Node> BitwiseAndRef;

class Not FINAL : virtual public IntExpr {
    IntExprRef expr;
public:
    Not(IntExprRef expr) : Unit(VM_NO_UNIT), expr(expr) {}
    vmint evalInt() OVERRIDE { return !expr->evalInt(); }
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE { return expr->isConstExpr(); }
    bool isPolyphonic() const OVERRIDE { return expr->isPolyphonic(); }
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    bool isFinal() const OVERRIDE { return expr->isFinal(); }
};
typedef Ref<Not,Node> NotRef;

class BitwiseNot FINAL : virtual public IntExpr {
    IntExprRef expr;
public:
    BitwiseNot(IntExprRef expr) : Unit(VM_NO_UNIT), expr(expr) {}
    vmint evalInt() OVERRIDE { return ~expr->evalInt(); }
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE { return expr->isConstExpr(); }
    bool isPolyphonic() const OVERRIDE { return expr->isPolyphonic(); }
    vmfloat unitFactor() const OVERRIDE { return VM_NO_FACTOR; }
    bool isFinal() const OVERRIDE { return expr->isFinal(); }
};
typedef Ref<BitwiseNot,Node> BitwiseNotRef;

class Final FINAL : virtual public IntExpr, virtual public RealExpr {
    NumberExprRef expr;
public:
    Final(NumberExprRef expr) : Unit(expr->unitType()), expr(expr) {}
    ExprType_t exprType() const OVERRIDE { return expr->exprType(); }
    vmint evalInt() OVERRIDE { return expr->asInt()->evalInt(); }
    vmfloat evalReal() OVERRIDE { return expr->asReal()->evalReal(); }
    String evalCastToStr() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
    bool isConstExpr() const OVERRIDE { return expr->isConstExpr(); }
    bool isPolyphonic() const OVERRIDE { return expr->isPolyphonic(); }
    vmfloat unitFactor() const OVERRIDE { return expr->unitFactor(); }
    bool isFinal() const OVERRIDE { return true; }
};
typedef Ref<Final,Node> FinalRef;

class ParserContext FINAL : public VMParserContext {
public:
    struct Error {
        String txt;
        int line;
    };
    typedef Error Warning;

    void* scanner;
    std::istream* is;
    std::vector<ParserIssue> vErrors;
    std::vector<ParserIssue> vWarnings;
    std::vector<ParserIssue> vIssues;
    std::vector<CodeBlock>   vPreprocessorComments;

    std::set<String> builtinPreprocessorConditions;
    std::set<String> userPreprocessorConditions;

    std::map<String,VariableRef> vartable;
    std::map<String,StatementsRef> userFnTable;
    vmint globalIntVarCount;
    vmint globalRealVarCount;
    vmint globalStrVarCount;
    vmint globalUnitFactorCount;
    vmint polyphonicIntVarCount;
    vmint polyphonicRealVarCount;
    vmint polyphonicUnitFactorCount;

    EventHandlersRef handlers;

    OnInitRef onInit;
    OnNoteRef onNote;
    OnReleaseRef onRelease;
    OnControllerRef onController;

    ArrayList<vmint>* globalIntMemory;
    ArrayList<vmfloat>* globalRealMemory;
    ArrayList<String>* globalStrMemory;
    ArrayList<vmfloat>* globalUnitFactorMemory;
    vmint requiredMaxStackSize;

    VMFunctionProvider* functionProvider;

    ExecContext* execContext;

    ParserContext(VMFunctionProvider* parent) :
        scanner(NULL), is(NULL),
        globalIntVarCount(0), globalRealVarCount(0), globalStrVarCount(0),
        globalUnitFactorCount(0),
        polyphonicIntVarCount(0), polyphonicRealVarCount(0),
        polyphonicUnitFactorCount(0),
        globalIntMemory(NULL), globalRealMemory(NULL), globalStrMemory(NULL),
        globalUnitFactorMemory(NULL),
        requiredMaxStackSize(-1),
        functionProvider(parent), execContext(NULL)
    {
    }
    virtual ~ParserContext();
    VariableRef globalVar(const String& name);
    IntVariableRef globalIntVar(const String& name);
    RealVariableRef globalRealVar(const String& name);
    StringVariableRef globalStrVar(const String& name);
    VariableRef variableByName(const String& name);
    StatementsRef userFunctionByName(const String& name);
    void addErr(int firstLine, int lastLine, int firstColumn, int lastColumn, const char* txt);
    void addWrn(int firstLine, int lastLine, int firstColumn, int lastColumn, const char* txt);
    void addPreprocessorComment(int firstLine, int lastLine, int firstColumn, int lastColumn);
    void createScanner(std::istream* is);
    void destroyScanner();
    bool setPreprocessorCondition(const char* name);
    bool resetPreprocessorCondition(const char* name);
    bool isPreprocessorConditionSet(const char* name);
    std::vector<ParserIssue> issues() const OVERRIDE;
    std::vector<ParserIssue> errors() const OVERRIDE;
    std::vector<ParserIssue> warnings() const OVERRIDE;
    std::vector<CodeBlock> preprocessorComments() const OVERRIDE;
    VMEventHandler* eventHandler(uint index) OVERRIDE;
    VMEventHandler* eventHandlerByName(const String& name) OVERRIDE;
    void registerBuiltInConstIntVariables(const std::map<String,vmint>& vars);
    void registerBuiltInConstRealVariables(const std::map<String,vmfloat>& vars);
    void registerBuiltInIntVariables(const std::map<String,VMIntPtr*>& vars);
    void registerBuiltInIntArrayVariables(const std::map<String,VMInt8Array*>& vars);
    void registerBuiltInDynVariables(const std::map<String,VMDynVar*>& vars);
};

class ExecContext FINAL : public VMExecContext {
public:
    struct StackFrame {
        Statement* statement;
        int subindex;

        StackFrame() {
            statement = NULL;
            subindex  = -1;
        }
    };

    ArrayList<vmint> polyphonicIntMemory;
    ArrayList<vmfloat> polyphonicRealMemory;
    ArrayList<vmfloat> polyphonicUnitFactorMemory;
    VMExecStatus_t status;
    StmtFlags_t flags;
    ArrayList<StackFrame> stack;
    int stackFrame;
    vmint suspendMicroseconds;
    size_t instructionsCount;
    struct ExitRes {
        Expression* value;
        IntLiteral intLiteral;
        RealLiteral realLiteral;
        StringLiteral stringLiteral;

        ExitRes() :
            intLiteral({ .value = 0 }), realLiteral({ .value = 0.0 }),
            stringLiteral("") { }
    } exitRes;

    ExecContext();
    virtual ~ExecContext() {}

    inline void pushStack(Statement* stmt) {
        stackFrame++;
        //printf("pushStack() -> %d\n", stackFrame);
        if (stackFrame >= stack.size()) return;
        stack[stackFrame].statement = stmt;
        stack[stackFrame].subindex  = 0;
    }

    inline void popStack() {
        stack[stackFrame].statement = NULL;
        stack[stackFrame].subindex  = -1;
        stackFrame--;
        //printf("popStack() -> %d\n", stackFrame);
    }

    inline void reset() {
        stack[0].statement = NULL;
        stack[0].subindex  = -1;
        stackFrame = -1;
        flags = STMT_SUCCESS;
    }

    inline void clearExitRes() {
        exitRes.value = NULL;
    }

    vmint suspensionTimeMicroseconds() const OVERRIDE {
        return suspendMicroseconds;
    }

    void resetPolyphonicData() OVERRIDE {
        if (!polyphonicIntMemory.empty())
            memset(&polyphonicIntMemory[0], 0, polyphonicIntMemory.size() * sizeof(vmint));
        if (!polyphonicRealMemory.empty())
            memset(&polyphonicRealMemory[0], 0, polyphonicRealMemory.size() * sizeof(vmfloat));
        if (!polyphonicUnitFactorMemory.empty()) {
            const vmint sz = polyphonicUnitFactorMemory.size();
            for (vmint i = 0; i < sz; ++i)
                polyphonicUnitFactorMemory[i] = VM_NO_FACTOR;
        }
    }

    size_t instructionsPerformed() const OVERRIDE {
        return instructionsCount;
    }

    void signalAbort() OVERRIDE {
        flags = StmtFlags_t(flags | STMT_ABORT_SIGNALLED);
    }

    void forkTo(VMExecContext* ectx) const OVERRIDE;

    VMExpr* exitResult() OVERRIDE {
        return exitRes.value;
    }
};

} // namespace LinuxSampler

#endif // LS_INSTRPARSERTREE_H
