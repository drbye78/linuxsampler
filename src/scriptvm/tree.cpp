/*
 * Copyright (c) 2014 - 2019 Christian Schoenebeck and Andreas Persson
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include <cstdio>
#include <string.h>
#include "tree.h"
#include "../common/global_private.h"
#include "../common/RTMath.h"
#include <assert.h>

namespace LinuxSampler {
    
bool isNoOperation(StatementRef statement) {
    return statement->statementType() == STMT_NOOP;
}

String acceptedArgTypesStr(VMFunction* fn, vmint iArg) {
    static const ExprType_t allTypes[] = {
        INT_EXPR,
        INT_ARR_EXPR,
        REAL_EXPR,
        REAL_ARR_EXPR,
        STRING_EXPR,
        STRING_ARR_EXPR,
    };
    const size_t nTypes = sizeof(allTypes) / sizeof(ExprType_t);

    std::vector<ExprType_t> supportedTypes;
    for (int iType = 0; iType < nTypes; ++iType) {
        const ExprType_t& type = allTypes[iType];
        if (fn->acceptsArgType(iArg, type))
            supportedTypes.push_back(type);
    }
    assert(!supportedTypes.empty());

    if (supportedTypes.size() == 1) {
        return typeStr(*supportedTypes.begin());
    } else {
        String s = "either ";
        for (size_t i = 0; i < supportedTypes.size(); ++i) {
            const ExprType_t& type = supportedTypes[i];
            if (i == 0) {
                s += typeStr(type);
            } else if (i == supportedTypes.size() - 1) {
                s += " or " + typeStr(type);
            } else {
                s += ", " + typeStr(type);
            }
        }
        return s;
    }
}
    
Node::Node() {
}

Node::~Node() {
}

void Node::printIndents(int n) {
    for (int i = 0; i < n; ++i) printf("  ");
    fflush(stdout);
}

vmint Unit::convIntToUnitFactor(vmint iValue, VMUnit* srcUnit, VMUnit* dstUnit) {
    vmfloat f = (vmfloat) iValue;
    vmfloat factor = srcUnit->unitFactor() / dstUnit->unitFactor();
    if (sizeof(vmfloat) == sizeof(float))
        return llroundf(f * factor);
    else
        return llround(f * factor);
}

vmint Unit::convIntToUnitFactor(vmint iValue, vmfloat srcFactor, vmfloat dstFactor) {
    vmfloat f = (vmfloat) iValue;
    vmfloat factor = srcFactor / dstFactor;
    if (sizeof(vmfloat) == sizeof(float))
        return llroundf(f * factor);
    else
        return llround(f * factor);
}

vmfloat Unit::convRealToUnitFactor(vmfloat fValue, VMUnit* srcUnit, VMUnit* dstUnit) {
    vmfloat factor = srcUnit->unitFactor() / dstUnit->unitFactor();
    return fValue * factor;
}

vmfloat Unit::convRealToUnitFactor(vmfloat fValue, vmfloat srcFactor, vmfloat dstFactor) {
    vmfloat factor = srcFactor / dstFactor;
    return fValue * factor;
}

vmint IntExpr::evalIntToUnitFactor(vmfloat unitFactor) {
    vmfloat f = (vmfloat) evalInt();
    vmfloat factor = this->unitFactor() / unitFactor;
    if (sizeof(vmfloat) == sizeof(float))
        return llroundf(f * factor);
    else
        return llround(f * factor);
}

static String _unitFactorToShortStr(vmfloat unitFactor) {
    const long int tens = lround( log10(unitFactor) );
    switch (tens) {
        case  3: return "k";  // kilo  = 10^3
        case  2: return "h";  // hecto = 10^2
        case  1: return "da"; // deca  = 10
        case  0: return "" ;  //  --   = 1
        case -1: return "d";  // deci  = 10^-1
        case -2: return "c";  // centi = 10^-2 (this is also used for tuning "cents")
        case -3: return "m";  // milli = 10^-3
        case -4: return "md"; // milli deci = 10^-4
        case -5: return "mc"; // milli centi = 10^-5 (this is also used for tuning "cents")
        case -6: return "u";  // micro = 10^-6
        default: return "*10^" + ToString(tens);
    }
}

static String _unitToStr(Unit* unit) {
    const StdUnit_t type = unit->unitType();
    String sType;
    switch (type) {
        case VM_NO_UNIT: break;
        case VM_SECOND: sType = "s"; break;
        case VM_HERTZ: sType = "Hz"; break;
        case VM_BEL: sType = "B"; break;
    }

    String prefix = _unitFactorToShortStr( unit->unitFactor() );

    return prefix + sType;
}

String IntExpr::evalCastToStr() {
    return ToString(evalInt()) + _unitToStr(this);
}

vmfloat RealExpr::evalRealToUnitFactor(vmfloat unitFactor) {
    vmfloat f = evalReal();
    vmfloat factor = this->unitFactor() / unitFactor;
    return f * factor;
}

String RealExpr::evalCastToStr() {
    return ToString(evalReal()) + _unitToStr(this);
}

String IntArrayExpr::evalCastToStr() {
    String s = "{";
    for (vmint i = 0; i < arraySize(); ++i) {
        vmint val = evalIntElement(i);
        vmfloat factor = unitFactorOfElement(i);
        if (i) s += ",";
        s += ToString(val) + _unitFactorToShortStr(factor);
    }
    s += "}";
    return s;
}

String RealArrayExpr::evalCastToStr() {
    String s = "{";
    for (vmint i = 0; i < arraySize(); ++i) {
        vmfloat val = evalRealElement(i);
        vmfloat factor = unitFactorOfElement(i);
        if (i) s += ",";
        s += ToString(val) + _unitFactorToShortStr(factor);
    }
    s += "}";
    return s;
}

IntLiteral::IntLiteral(const IntLitDef& def) :
    IntExpr(), Unit(def.unitType),
    value(def.value), unitPrefixFactor(def.unitFactor),
    finalVal(def.isFinal)
{
}

vmint IntLiteral::evalInt() {
    return value;
}

void IntLiteral::dump(int level) {
    printIndents(level);
    printf("IntLiteral %lld\n", value);
}

RealLiteral::RealLiteral(const RealLitDef& def) :
    RealExpr(), Unit(def.unitType),
    value(def.value), unitPrefixFactor(def.unitFactor),
    finalVal(def.isFinal)
{
}

vmfloat RealLiteral::evalReal() {
    return value;
}

void RealLiteral::dump(int level) {
    printIndents(level);
    printf("RealLiteral %f\n", value);
}

void StringLiteral::dump(int level) {
    printIndents(level);
    printf("StringLiteral: '%s'\n", value.c_str());
}

Add::Add(NumberExprRef lhs, NumberExprRef rhs) :
    VaritypeScalarBinaryOp(lhs, rhs),
    Unit(
        // lhs and rhs are forced to be same unit type at parse time, so either one is fine here
        (lhs) ? lhs->unitType() : VM_NO_UNIT
    )
{
}

vmint Add::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    if (!pLHS || !pRHS) return 0;
    // eval*() call is required before calling unitFactor(), since the latter does not evaluate expressions!
    vmint lvalue = pLHS->evalInt();
    vmint rvalue = pRHS->evalInt();
    if (pLHS->unitFactor() == pRHS->unitFactor())
        return lvalue + rvalue;
    if (pLHS->unitFactor() < pRHS->unitFactor())
        return lvalue + Unit::convIntToUnitFactor(rvalue, pRHS, pLHS);
    else
        return Unit::convIntToUnitFactor(lvalue, pLHS, pRHS) + rvalue;
}

vmfloat Add::evalReal() {
    RealExpr* pLHS = dynamic_cast<RealExpr*>(&*lhs);
    RealExpr* pRHS = dynamic_cast<RealExpr*>(&*rhs);
    if (!pLHS || !pRHS) return 0;
    // eval*() call is required before calling unitFactor(), since the latter does not evaluate expressions!
    vmfloat lvalue = pLHS->evalReal();
    vmfloat rvalue = pRHS->evalReal();
    if (pLHS->unitFactor() == pRHS->unitFactor())
        return lvalue + rvalue;
    if (pLHS->unitFactor() < pRHS->unitFactor())
        return lvalue + Unit::convRealToUnitFactor(rvalue, pRHS, pLHS);
    else
        return Unit::convRealToUnitFactor(lvalue, pLHS, pRHS) + rvalue;
}

vmfloat Add::unitFactor() const {
    const NumberExpr* pLHS = dynamic_cast<const NumberExpr*>(&*lhs);
    const NumberExpr* pRHS = dynamic_cast<const NumberExpr*>(&*rhs);
    return (pLHS->unitFactor() < pRHS->unitFactor()) ? pLHS->unitFactor() : pRHS->unitFactor();
}

void Add::dump(int level) {
    printIndents(level);
    printf("Add(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

Sub::Sub(NumberExprRef lhs, NumberExprRef rhs) :
    VaritypeScalarBinaryOp(lhs, rhs),
    Unit(
        // lhs and rhs are forced to be same unit type at parse time, so either one is fine here
        (lhs) ? lhs->unitType() : VM_NO_UNIT
    )
{
}

vmint Sub::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    if (!pLHS || !pRHS) return 0;
    // eval*() call is required before calling unitFactor(), since the latter does not evaluate expressions!
    vmint lvalue = pLHS->evalInt();
    vmint rvalue = pRHS->evalInt();
    if (pLHS->unitFactor() == pRHS->unitFactor())
        return lvalue - rvalue;
    if (pLHS->unitFactor() < pRHS->unitFactor())
        return lvalue - Unit::convIntToUnitFactor(rvalue, pRHS, pLHS);
    else
        return Unit::convIntToUnitFactor(lvalue, pLHS, pRHS) - rvalue;
}

vmfloat Sub::evalReal() {
    RealExpr* pLHS = dynamic_cast<RealExpr*>(&*lhs);
    RealExpr* pRHS = dynamic_cast<RealExpr*>(&*rhs);
    if (!pLHS || !pRHS) return 0;
    // eval*() call is required before calling unitFactor(), since the latter does not evaluate expressions!
    vmfloat lvalue = pLHS->evalReal();
    vmfloat rvalue = pRHS->evalReal();
    if (pLHS->unitFactor() == pRHS->unitFactor())
        return lvalue - rvalue;
    if (pLHS->unitFactor() < pRHS->unitFactor())
        return lvalue - Unit::convRealToUnitFactor(rvalue, pRHS, pLHS);
    else
        return Unit::convRealToUnitFactor(lvalue, pLHS, pRHS) - rvalue;
}

vmfloat Sub::unitFactor() const {
    const NumberExpr* pLHS = dynamic_cast<const NumberExpr*>(&*lhs);
    const NumberExpr* pRHS = dynamic_cast<const NumberExpr*>(&*rhs);
    return (pLHS->unitFactor() < pRHS->unitFactor()) ? pLHS->unitFactor() : pRHS->unitFactor();
}

void Sub::dump(int level) {
    printIndents(level);
    printf("Sub(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

Mul::Mul(NumberExprRef lhs, NumberExprRef rhs) :
    VaritypeScalarBinaryOp(lhs, rhs),
    Unit(
        // currently the NKSP parser only allows a unit type on either side on multiplications
        (lhs->unitType()) ? lhs->unitType() : rhs->unitType()
    )
{
}

vmint Mul::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);;
    return (pLHS && pRHS) ? pLHS->evalInt() * pRHS->evalInt() : 0;
}

vmfloat Mul::evalReal() {
    RealExpr* pLHS = dynamic_cast<RealExpr*>(&*lhs);
    RealExpr* pRHS = dynamic_cast<RealExpr*>(&*rhs);;
    return (pLHS && pRHS) ? pLHS->evalReal() * pRHS->evalReal() : 0;
}

void Mul::dump(int level) {
    printIndents(level);
    printf("Mul(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

vmfloat Mul::unitFactor() const {
    const NumberExpr* pLHS = dynamic_cast<const NumberExpr*>(&*lhs);
    const NumberExpr* pRHS = dynamic_cast<const NumberExpr*>(&*rhs);
    return pLHS->unitFactor() * pRHS->unitFactor();
}

Div::Div(NumberExprRef lhs, NumberExprRef rhs) :
    VaritypeScalarBinaryOp(lhs, rhs),
    Unit(
        // the NKSP parser only allows either A) a unit type on left side and none
        // on right side or B) an identical unit type on both sides
        (lhs->unitType() && rhs->unitType()) ? VM_NO_UNIT : lhs->unitType()
    )
{
}

vmint Div::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    if (!pLHS || !pRHS) return 0;
    vmint l = pLHS->evalInt();
    vmint r = pRHS->evalInt();
    if (r == 0) return 0;
    return l / r;
}

vmfloat Div::evalReal() {
    RealExpr* pLHS = dynamic_cast<RealExpr*>(&*lhs);
    RealExpr* pRHS = dynamic_cast<RealExpr*>(&*rhs);
    if (!pLHS || !pRHS) return 0;
    vmfloat l = pLHS->evalReal();
    vmfloat r = pRHS->evalReal();
    if (r == vmfloat(0)) return 0;
    return l / r;
}

void Div::dump(int level) {
    printIndents(level);
    printf("Div(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

vmfloat Div::unitFactor() const {
    const NumberExpr* pLHS = dynamic_cast<const NumberExpr*>(&*lhs);
    const NumberExpr* pRHS = dynamic_cast<const NumberExpr*>(&*rhs);
    return pLHS->unitFactor() / pRHS->unitFactor();
}

vmint Mod::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    return (pLHS && pRHS) ? pLHS->evalInt() % pRHS->evalInt() : 0;
}

void Mod::dump(int level) {
    printIndents(level);
    printf("Mod(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

void Args::dump(int level) {
    printIndents(level);
    printf("Args(\n");
    for (std::vector<ExpressionRef>::iterator it = args.begin() ; it != args.end() ; ++it) {
        (*it)->dump(level+1);
    }
    printIndents(level);
    printf(")\n");
}

bool Args::isPolyphonic() const {
    for (vmint i = 0; i < args.size(); ++i)
        if (args[i]->isPolyphonic())
            return true;
    return false;
}

EventHandlers::EventHandlers() {
    //printf("EventHandlers::Constructor 0x%lx\n", (long long)this);
}

EventHandlers::~EventHandlers() {
}

void EventHandlers::add(EventHandlerRef arg) {
    args.push_back(arg);
}

void EventHandlers::dump(int level) {
    printIndents(level);
    printf("EventHandlers {\n");
    for (std::vector<EventHandlerRef>::iterator it = args.begin() ; it != args.end() ; ++it) {
        (*it)->dump(level+1);
    }
    printIndents(level);
    printf("}\n");
}

EventHandler* EventHandlers::eventHandlerByName(const String& name) const {
    for (vmint i = 0; i < args.size(); ++i)
        if (args.at(i)->eventHandlerName() == name)
            return const_cast<EventHandler*>(&*args.at(i));
    return NULL;
}

EventHandler* EventHandlers::eventHandler(uint index) const {
    if (index >= args.size()) return NULL;
    return const_cast<EventHandler*>(&*args.at(index));
}

bool EventHandlers::isPolyphonic() const {
    for (vmint i = 0; i < args.size(); ++i)
        if (args[i]->isPolyphonic())
            return true;
    return false;
}

Assignment::Assignment(VariableRef variable, ExpressionRef value)
   : variable(variable), value(value)
{
}

void Assignment::dump(int level) {
    printIndents(level);
    printf("Assignment\n");
}

StmtFlags_t Assignment::exec() {
    if (!variable)
        return StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    variable->assign(&*value);
    return STMT_SUCCESS;
}

EventHandler::EventHandler(StatementsRef statements) {
    this->statements = statements;
    usingPolyphonics = statements->isPolyphonic();
}

void EventHandler::dump(int level) {
    printIndents(level);
    printf("EventHandler {\n");
    statements->dump(level+1);
    printIndents(level);
    printf("}\n");
}

void Statements::dump(int level) {
    printIndents(level);
    printf("Statements {\n");
    for (std::vector<StatementRef>::iterator it = args.begin() ; it != args.end() ; ++it) {
        (*it)->dump(level+1);
    }
    printIndents(level);
    printf("}\n");
}

Statement* Statements::statement(uint i) {
    if (i >= args.size()) return NULL;
    return &*args.at(i);
}

bool Statements::isPolyphonic() const {
    for (vmint i = 0; i < args.size(); ++i)
        if (args[i]->isPolyphonic())
            return true;
    return false;
}

DynamicVariableCall::DynamicVariableCall(const String& name, ParserContext* ctx, VMDynVar* v) :
    Variable({
        .ctx = ctx,
        .elements = 0
    }),
    Unit(VM_NO_UNIT),
    dynVar(v), varName(name)
{
}

vmint DynamicVariableCall::evalInt() {
    VMIntExpr* expr = dynamic_cast<VMIntExpr*>(dynVar);
    if (!expr) return 0;
    return expr->evalInt();
}

String DynamicVariableCall::evalStr() {
    VMStringExpr* expr = dynamic_cast<VMStringExpr*>(dynVar);
    if (!expr) return "";
    return expr->evalStr();
}

String DynamicVariableCall::evalCastToStr() {
    if (dynVar->exprType() == STRING_EXPR) {
        return evalStr();
    } else {
        VMIntExpr* intExpr = dynamic_cast<VMIntExpr*>(dynVar);
        return intExpr ? ToString(intExpr->evalInt()) : "";
    }
}

void DynamicVariableCall::dump(int level) {
    printIndents(level);
    printf("Dynamic Variable '%s'\n", varName.c_str());
}

FunctionCall::FunctionCall(const char* function, ArgsRef args, VMFunction* fn) :
    Unit(
        (fn) ? fn->returnUnitType(dynamic_cast<VMFnArgs*>(&*args)) : VM_NO_UNIT
    ),
    functionName(function), args(args), fn(fn), result(NULL)
{
}

void FunctionCall::dump(int level) {
    printIndents(level);
    printf("FunctionCall '%s' args={\n", functionName.c_str());
    args->dump(level+1);
    printIndents(level);
    printf("}\n");
}

ExprType_t FunctionCall::exprType() const {
    if (!fn) return EMPTY_EXPR;
    FunctionCall* self = const_cast<FunctionCall*>(this);
    return fn->returnType(dynamic_cast<VMFnArgs*>(&*self->args));
}

vmfloat FunctionCall::unitFactor() const {
    if (!fn || !result) return VM_NO_FACTOR;
    VMExpr* expr = result->resultValue();
    if (!expr) return VM_NO_FACTOR;
    VMNumberExpr* scalar = expr->asNumber();
    if (!scalar) return VM_NO_FACTOR;
    return scalar->unitFactor();
}

bool FunctionCall::isFinal() const {
    if (!fn) return false;
    FunctionCall* self = const_cast<FunctionCall*>(this);
    return fn->returnsFinal(dynamic_cast<VMFnArgs*>(&*self->args));
}

VMFnResult* FunctionCall::execVMFn() {
    if (!fn) return NULL;
    // assuming here that all argument checks (amount and types) have been made
    // at parse time, to avoid time intensive checks on each function call
    return fn->exec(dynamic_cast<VMFnArgs*>(&*args));
}

StmtFlags_t FunctionCall::exec() {
    result = execVMFn();
    if (!result)
        return StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    return result->resultFlags();
}

vmint FunctionCall::evalInt() {
    result = execVMFn();
    if (!result) return 0;
    VMIntExpr* intExpr = dynamic_cast<VMIntExpr*>(result->resultValue());
    if (!intExpr) return 0;
    return intExpr->evalInt();
}

vmfloat FunctionCall::evalReal() {
    result = execVMFn();
    if (!result) return 0;
    VMRealExpr* realExpr = dynamic_cast<VMRealExpr*>(result->resultValue());
    if (!realExpr) return 0;
    return realExpr->evalReal();
}

VMIntArrayExpr* FunctionCall::asIntArray() const {
    if (!result) return 0;
    VMIntArrayExpr* intArrExpr = dynamic_cast<VMIntArrayExpr*>(result->resultValue());
    return intArrExpr;
}

VMRealArrayExpr* FunctionCall::asRealArray() const {
    if (!result) return 0;
    VMRealArrayExpr* realArrExpr = dynamic_cast<VMRealArrayExpr*>(result->resultValue());
    return realArrExpr;
}

String FunctionCall::evalStr() {
    result = execVMFn();
    if (!result) return "";
    VMStringExpr* strExpr = dynamic_cast<VMStringExpr*>(result->resultValue());
    if (!strExpr) return "";
    return strExpr->evalStr();
}

String FunctionCall::evalCastToStr() {
    result = execVMFn();
    if (!result) return "";
    const ExprType_t resultType = result->resultValue()->exprType();
    if (resultType == STRING_EXPR) {
        VMStringExpr* strExpr = dynamic_cast<VMStringExpr*>(result->resultValue());
        return strExpr ? strExpr->evalStr() : "";
    } else if (resultType == REAL_EXPR) {
        VMRealExpr* realExpr = dynamic_cast<VMRealExpr*>(result->resultValue());
        return realExpr ? ToString(realExpr->evalReal()) : "";
    } else {
        VMIntExpr* intExpr = dynamic_cast<VMIntExpr*>(result->resultValue());
        return intExpr ? ToString(intExpr->evalInt()) : "";
    }
}

Variable::Variable(const VariableDecl& decl) :
    context(decl.ctx), memPos(decl.memPos), bConst(decl.isConst)
{
}

NumberVariable::NumberVariable(const VariableDecl& decl) :
    Variable(decl),
    Unit(decl.unitType),
    unitFactorMemPos(decl.unitFactorMemPos), polyphonic(decl.isPolyphonic),
    finalVal(decl.isFinal)
{
}

vmfloat NumberVariable::unitFactor() const {
    if (isPolyphonic()) {
        return context->execContext->polyphonicUnitFactorMemory[unitFactorMemPos];
    }
    return (*context->globalUnitFactorMemory)[unitFactorMemPos];
}

inline static vmint postfixInc(vmint& object, vmint incBy) {
    const vmint i = object;
    object += incBy;
    return i;
}

IntVariable::IntVariable(const VariableDecl& decl) :
    NumberVariable({
        .ctx = decl.ctx,
        .isPolyphonic = decl.isPolyphonic,
        .isConst = decl.isConst,
        .elements = decl.elements,
        .memPos = (
            (!decl.ctx) ? 0 :
                (decl.isPolyphonic) ?
                    postfixInc(decl.ctx->polyphonicIntVarCount, decl.elements) :
                    postfixInc(decl.ctx->globalIntVarCount, decl.elements)
        ),
        .unitFactorMemPos = (
            (!decl.ctx) ? 0 :
                (decl.isPolyphonic) ?
                    postfixInc(decl.ctx->polyphonicUnitFactorCount, decl.elements) :
                    postfixInc(decl.ctx->globalUnitFactorCount, decl.elements)
        ),
        .unitType = decl.unitType,
        .isFinal = decl.isFinal,
    }),
    Unit(decl.unitType)
{
    //printf("IntVar parserctx=0x%lx memPOS=%d\n", ctx, memPos);
    assert(!decl.isPolyphonic || decl.ctx);
}

void IntVariable::assign(Expression* expr) {
    IntExpr* intExpr = dynamic_cast<IntExpr*>(expr);
    if (intExpr) {
        //NOTE: sequence matters! evalInt() must be called before getting unitFactor() !
        if (isPolyphonic()) {
            context->execContext->polyphonicIntMemory[memPos] = intExpr->evalInt();
            context->execContext->polyphonicUnitFactorMemory[unitFactorMemPos] = intExpr->unitFactor();
        } else {
            (*context->globalIntMemory)[memPos] = intExpr->evalInt();
            (*context->globalUnitFactorMemory)[unitFactorMemPos] = intExpr->unitFactor();
        }
    }
}

vmint IntVariable::evalInt() {
    //printf("IntVariable::eval pos=%d\n", memPos);
    if (isPolyphonic()) {
        //printf("evalInt() poly memPos=%d execCtx=0x%lx\n", memPos, (uint64_t)context->execContext);
        return context->execContext->polyphonicIntMemory[memPos];
    }
    return (*context->globalIntMemory)[memPos];
}

void IntVariable::dump(int level) {
    printIndents(level);
    printf("IntVariable\n");
    //printf("IntVariable memPos=%d\n", memPos);
}

RealVariable::RealVariable(const VariableDecl& decl) :
    NumberVariable({
        .ctx = decl.ctx,
        .isPolyphonic = decl.isPolyphonic,
        .isConst = decl.isConst,
        .elements = decl.elements,
        .memPos = (
            (!decl.ctx) ? 0 :
                (decl.isPolyphonic) ?
                    postfixInc(decl.ctx->polyphonicRealVarCount, decl.elements) :
                    postfixInc(decl.ctx->globalRealVarCount, decl.elements)
        ),
        .unitFactorMemPos = (
            (!decl.ctx) ? 0 :
                (decl.isPolyphonic) ?
                    postfixInc(decl.ctx->polyphonicUnitFactorCount, decl.elements) :
                    postfixInc(decl.ctx->globalUnitFactorCount, decl.elements)
        ),
        .unitType = decl.unitType,
        .isFinal = decl.isFinal,
    }),
    Unit(decl.unitType)
{
    //printf("RealVar parserctx=0x%lx memPOS=%d\n", ctx, memPos);
    assert(!decl.isPolyphonic || decl.ctx);
}

void RealVariable::assign(Expression* expr) {
    RealExpr* realExpr = dynamic_cast<RealExpr*>(expr);
    if (realExpr) {
        //NOTE: sequence matters! evalReal() must be called before getting unitFactor() !
        if (isPolyphonic()) {
            context->execContext->polyphonicRealMemory[memPos] = realExpr->evalReal();
            context->execContext->polyphonicUnitFactorMemory[unitFactorMemPos] = realExpr->unitFactor();
        } else {
            (*context->globalRealMemory)[memPos] = realExpr->evalReal();
            (*context->globalUnitFactorMemory)[unitFactorMemPos] = realExpr->unitFactor();
        }
    }
}

vmfloat RealVariable::evalReal() {
    //printf("RealVariable::eval pos=%d\n", memPos);
    if (isPolyphonic()) {
        //printf("evalReal() poly memPos=%d execCtx=0x%lx\n", memPos, (uint64_t)context->execContext);
        return context->execContext->polyphonicRealMemory[memPos];
    }
    return (*context->globalRealMemory)[memPos];
}

void RealVariable::dump(int level) {
    printIndents(level);
    printf("RealVariable\n");
    //printf("RealVariable memPos=%d\n", memPos);
}

ConstIntVariable::ConstIntVariable(const IntVarDef& def) :
    IntVariable({
        .ctx = def.ctx,
        .isPolyphonic = false,
        .isConst = true,
        .elements = 1,
        .memPos = def.memPos,
        .unitFactorMemPos = def.unitFactorMemPos,
        .unitType = def.unitType,
        .isFinal = def.isFinal,
    }),
    Unit(def.unitType),
    value(def.value), unitPrefixFactor(def.unitFactor)
{
}

void ConstIntVariable::assign(Expression* expr) {
    // ignore assignment
/*
    printf("ConstIntVariable::assign()\n");
    IntExpr* intExpr = dynamic_cast<IntExpr*>(expr);
    if (intExpr) {
        value = intExpr->evalInt();
    }
*/
}

vmint ConstIntVariable::evalInt() {
    return value;
}

void ConstIntVariable::dump(int level) {
    printIndents(level);
    printf("ConstIntVariable val=%lld\n", value);
}

ConstRealVariable::ConstRealVariable(const RealVarDef& def) :
    RealVariable({
        .ctx = def.ctx,
        .isPolyphonic = false,
        .isConst = true,
        .elements = 1,
        .memPos = def.memPos,
        .unitFactorMemPos = def.unitFactorMemPos,
        .unitType = def.unitType,
        .isFinal = def.isFinal,
    }),
    Unit(def.unitType),
    value(def.value), unitPrefixFactor(def.unitFactor)
{
}

void ConstRealVariable::assign(Expression* expr) {
    // ignore assignment
}

vmfloat ConstRealVariable::evalReal() {
    return value;
}

void ConstRealVariable::dump(int level) {
    printIndents(level);
    printf("ConstRealVariable val=%f\n", value);
}

BuiltInIntVariable::BuiltInIntVariable(const String& name, VMIntPtr* ptr) :
    IntVariable({
        .ctx = NULL,
        .isPolyphonic = false,
        .isConst = false, // may or may not be modifyable though!
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    }),
    Unit(VM_NO_UNIT),
    name(name), ptr(ptr)
{
}

void BuiltInIntVariable::assign(Expression* expr) {
    IntExpr* valueExpr = dynamic_cast<IntExpr*>(expr);
    if (!valueExpr) return;
    ptr->assign(valueExpr->evalInt());
}

vmint BuiltInIntVariable::evalInt() {
    return ptr->evalInt();
}

void BuiltInIntVariable::dump(int level) {
    printIndents(level);
    printf("Built-in IntVar '%s'\n", name.c_str());
}

PolyphonicIntVariable::PolyphonicIntVariable(const VariableDecl& decl) :
    IntVariable({
        .ctx = decl.ctx,
        .isPolyphonic = true,
        .isConst = decl.isConst,
        .elements = 1,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = decl.unitType,
        .isFinal = decl.isFinal,
    }),
    Unit(decl.unitType)
{
}

void PolyphonicIntVariable::dump(int level) {
    printIndents(level);
    printf("PolyphonicIntVariable\n");
}

PolyphonicRealVariable::PolyphonicRealVariable(const VariableDecl& decl) :
    RealVariable({
        .ctx = decl.ctx,
        .isPolyphonic = true,
        .isConst = decl.isConst,
        .elements = 1,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = decl.unitType,
        .isFinal = decl.isFinal,
    }),
    Unit(decl.unitType)
{
}

void PolyphonicRealVariable::dump(int level) {
    printIndents(level);
    printf("PolyphonicRealVariable\n");
}

IntArrayVariable::IntArrayVariable(ParserContext* ctx, vmint size) :
    Variable({
        .ctx = ctx,
        .isPolyphonic = false,
        .isConst = false,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    })
{
    values.resize(size);
    memset(&values[0], 0, size * sizeof(vmint));

    unitFactors.resize(size);
    for (size_t i = 0; i < size; ++i)
        unitFactors[i] = VM_NO_FACTOR;
}

IntArrayVariable::IntArrayVariable(ParserContext* ctx, vmint size,
                                   ArgsRef values, bool _bConst) :
    Variable({
        .ctx = ctx,
        .isPolyphonic = false,
        .isConst = _bConst,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    })
{
    this->values.resize(size);
    this->unitFactors.resize(size);
    for (vmint i = 0; i < values->argsCount(); ++i) {
        VMIntExpr* expr = dynamic_cast<VMIntExpr*>(values->arg(i));
        if (expr) {
            this->values[i] = expr->evalInt();
            this->unitFactors[i] = expr->unitFactor();
        }
    }
}

IntArrayVariable::IntArrayVariable(ParserContext* ctx, bool bConst) :
    Variable({
        .ctx = ctx,
        .isPolyphonic = false,
        .isConst = bConst,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    })
{
}

vmint IntArrayVariable::evalIntElement(vmuint i) {
    if (i >= values.size()) return 0;
    return values[i];
}

void IntArrayVariable::assignIntElement(vmuint i, vmint value) {
    if (i >= values.size()) return;
    values[i] = value;
}

vmfloat IntArrayVariable::unitFactorOfElement(vmuint i) const {
    if (i >= unitFactors.size()) return VM_NO_FACTOR;
    return unitFactors[i];
}

void IntArrayVariable::assignElementUnitFactor(vmuint i, vmfloat factor) {
    if (i >= unitFactors.size()) return;
    unitFactors[i] = factor;
}

void IntArrayVariable::dump(int level) {
    printIndents(level);
    printf("IntArray(");
    for (vmint i = 0; i < values.size(); ++i) {
        if (i % 12 == 0) {
            printf("\n");
            printIndents(level+1);
        }
        printf("%lld, ", values[i]);
    }
    printIndents(level);
    printf(")\n");
}

RealArrayVariable::RealArrayVariable(ParserContext* ctx, vmint size) :
    Variable({
        .ctx = ctx,
        .isPolyphonic = false,
        .isConst = false,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    })
{
    values.resize(size);
    memset(&values[0], 0, size * sizeof(vmfloat));

    unitFactors.resize(size);
    for (size_t i = 0; i < size; ++i)
        unitFactors[i] = VM_NO_FACTOR;
}

RealArrayVariable::RealArrayVariable(ParserContext* ctx, vmint size,
                                     ArgsRef values, bool _bConst) :
    Variable({
        .ctx = ctx,
        .isPolyphonic = false,
        .isConst = _bConst,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    })
{
    this->values.resize(size);
    this->unitFactors.resize(size);
    for (vmint i = 0; i < values->argsCount(); ++i) {
        VMRealExpr* expr = dynamic_cast<VMRealExpr*>(values->arg(i));
        if (expr) {
            this->values[i] = expr->evalReal();
            this->unitFactors[i] = expr->unitFactor();
        }
    }
}

RealArrayVariable::RealArrayVariable(ParserContext* ctx, bool bConst) :
    Variable({
        .ctx = ctx,
        .isPolyphonic = false,
        .isConst = bConst,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    })
{
}

vmfloat RealArrayVariable::evalRealElement(vmuint i) {
    if (i >= values.size()) return 0;
    return values[i];
}

void RealArrayVariable::assignRealElement(vmuint i, vmfloat value) {
    if (i >= values.size()) return;
    values[i] = value;
}

vmfloat RealArrayVariable::unitFactorOfElement(vmuint i) const {
    if (i >= unitFactors.size()) return VM_NO_FACTOR;
    return unitFactors[i];
}

void RealArrayVariable::assignElementUnitFactor(vmuint i, vmfloat factor) {
    if (i >= unitFactors.size()) return;
    unitFactors[i] = factor;
}

void RealArrayVariable::dump(int level) {
    printIndents(level);
    printf("RealArray(");
    for (vmint i = 0; i < values.size(); ++i) {
        if (i % 12 == 0) {
            printf("\n");
            printIndents(level+1);
        }
        printf("%f, ", values[i]);
    }
    printIndents(level);
    printf(")\n");
}

BuiltInIntArrayVariable::BuiltInIntArrayVariable(const String& name,
                                                 VMInt8Array* array) :
    IntArrayVariable(NULL, false),
    name(name), array(array)
{
}

vmint BuiltInIntArrayVariable::evalIntElement(vmuint i) {
    return i >= array->size ? 0 : array->data[i];
}

void BuiltInIntArrayVariable::assignIntElement(vmuint i, vmint value) {
    if (i >= array->size) return;
    array->data[i] = value;
}

void BuiltInIntArrayVariable::dump(int level) {
    printIndents(level);
    printf("Built-In Int Array Variable '%s'\n", name.c_str());
}

IntArrayElement::IntArrayElement(IntArrayExprRef array, IntExprRef arrayIndex) :
    IntVariable({
        .ctx = NULL,
        .isPolyphonic = (array) ? array->isPolyphonic() : false,
        .isConst = (array) ? array->isConstExpr() : false,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    }),
    Unit(VM_NO_UNIT),
    array(array), index(arrayIndex), currentIndex(-1)
{
}

void IntArrayElement::assign(Expression* expr) {
    IntExpr* valueExpr = dynamic_cast<IntExpr*>(expr);
    if (!valueExpr) return;
    vmint value = valueExpr->evalInt();
    vmfloat unitFactor = valueExpr->unitFactor();

    if (!index) return;
    vmint idx = currentIndex = index->evalInt();
    if (idx < 0 || idx >= array->arraySize()) return;

    array->assignIntElement(idx, value);
    array->assignElementUnitFactor(idx, unitFactor);
}

vmint IntArrayElement::evalInt() {
    if (!index) return 0;
    vmint idx = currentIndex = index->evalInt();
    if (idx < 0 || idx >= array->arraySize()) return 0;

    return array->evalIntElement(idx);
}

vmfloat IntArrayElement::unitFactor() const {
    if (!index) return VM_NO_FACTOR;
    vmint idx = currentIndex;
    if (idx < 0 || idx >= array->arraySize()) return 0;

    return array->unitFactorOfElement(idx);
}

void IntArrayElement::dump(int level) {
    printIndents(level);
    printf("IntArrayElement\n");
}

RealArrayElement::RealArrayElement(RealArrayExprRef array, IntExprRef arrayIndex) :
    RealVariable({
        .ctx = NULL,
        .isPolyphonic = (array) ? array->isPolyphonic() : false,
        .isConst = (array) ? array->isConstExpr() : false,
        .elements = 0,
        .memPos = 0,
        .unitFactorMemPos = 0,
        .unitType = VM_NO_UNIT,
        .isFinal = false,
    }),
    Unit(VM_NO_UNIT),
    array(array), index(arrayIndex), currentIndex(-1)
{
}

void RealArrayElement::assign(Expression* expr) {
    RealExpr* valueExpr = dynamic_cast<RealExpr*>(expr);
    if (!valueExpr) return;
    vmfloat value = valueExpr->evalReal();
    vmfloat unitFactor = valueExpr->unitFactor();

    if (!index) return;
    vmint idx = currentIndex = index->evalInt();
    if (idx < 0 || idx >= array->arraySize()) return;

    array->assignRealElement(idx, value);
    array->assignElementUnitFactor(idx, unitFactor);
}

vmfloat RealArrayElement::evalReal() {
    if (!index) return 0;
    vmint idx = currentIndex = index->evalInt();
    if (idx < 0 || idx >= array->arraySize()) return 0;

    return array->evalRealElement(idx);
}

vmfloat RealArrayElement::unitFactor() const {
    if (!index) return VM_NO_FACTOR;
    vmint idx = currentIndex;
    if (idx < 0 || idx >= array->arraySize()) return 0;

    return array->unitFactorOfElement(idx);
}

void RealArrayElement::dump(int level) {
    printIndents(level);
    printf("RealArrayElement\n");
}

StringVariable::StringVariable(ParserContext* ctx) :
    Variable({
        .ctx = ctx,
        .elements = 1,
        .memPos = ctx->globalStrVarCount++
    })
{
}

StringVariable::StringVariable(ParserContext* ctx, bool bConst) :
    Variable({
        .ctx = ctx,
        .isConst = bConst,
        .memPos = 0,
    })
{
}

void StringVariable::assign(Expression* expr) {
    StringExpr* strExpr = dynamic_cast<StringExpr*>(expr);
    (*context->globalStrMemory)[memPos] = strExpr->evalStr();
}

String StringVariable::evalStr() {
    //printf("StringVariable::eval pos=%d\n", memPos);
    return (*context->globalStrMemory)[memPos];
}

void StringVariable::dump(int level) {
    printIndents(level);
    printf("StringVariable memPos=%lld\n", memPos);
}

ConstStringVariable::ConstStringVariable(ParserContext* ctx, String _value)
    : StringVariable(ctx,true), value(_value)
{
}

void ConstStringVariable::assign(Expression* expr) {
    // ignore assignment
//     StringExpr* strExpr = dynamic_cast<StringExpr*>(expr);
//     if (strExpr) value = strExpr->evalStr();
}

String ConstStringVariable::evalStr() {
    return value;
}

void ConstStringVariable::dump(int level) {
    printIndents(level);
    printf("ConstStringVariable val='%s'\n", value.c_str());
}

bool NumberBinaryOp::isFinal() const {
    NumberExprRef l = (NumberExprRef) lhs;
    NumberExprRef r = (NumberExprRef) rhs;
    return l->isFinal() || r->isFinal();
}

ExprType_t VaritypeScalarBinaryOp::exprType() const {
    return (lhs->exprType() == REAL_EXPR || rhs->exprType() == REAL_EXPR) ? REAL_EXPR : INT_EXPR;
}

String VaritypeScalarBinaryOp::evalCastToStr() {
    return (exprType() == REAL_EXPR) ?
        RealExpr::evalCastToStr() : IntExpr::evalCastToStr();
}

void If::dump(int level) {
    printIndents(level);
    if (ifStatements && elseStatements)
        printf("if cond stmts1 else stmts2 end if\n");
    else if (ifStatements)
        printf("if cond statements end if\n");
    else
        printf("if [INVALID]\n");
}

vmint If::evalBranch() {
    if (condition->evalInt()) return 0;
    if (elseStatements) return 1;
    return -1;
}

Statements* If::branch(vmuint i) const {
    if (i == 0) return (Statements*) &*ifStatements;
    if (i == 1) return (elseStatements) ? (Statements*) &*elseStatements : NULL;
    return NULL;
}

bool If::isPolyphonic() const {
    if (condition->isPolyphonic() || ifStatements->isPolyphonic())
        return true;
    return elseStatements ? elseStatements->isPolyphonic() : false;
}

void SelectCase::dump(int level) {
    printIndents(level);
    if (select)
        if (select->isConstExpr())
            printf("Case select %lld\n", select->evalInt());
        else
            printf("Case select [runtime expr]\n");
    else
        printf("Case select NULL\n");
    for (vmint i = 0; i < branches.size(); ++i) {
        printIndents(level+1);
        CaseBranch& branch = branches[i];
        if (branch.from && branch.to)
            if (branch.from->isConstExpr() && branch.to->isConstExpr())
                printf("case %lld to %lld\n", branch.from->evalInt(), branch.to->evalInt());
            else if (branch.from->isConstExpr() && !branch.to->isConstExpr())
                printf("case %lld to [runtime expr]\n", branch.from->evalInt());
            else if (!branch.from->isConstExpr() && branch.to->isConstExpr())
                printf("case [runtime expr] to %lld\n", branch.to->evalInt());
            else
                printf("case [runtime expr] to [runtime expr]\n");
        else if (branch.from)
            if (branch.from->isConstExpr())
                printf("case %lld\n", branch.from->evalInt());
            else
                printf("case [runtime expr]\n");
        else
            printf("case NULL\n");
    }
}

vmint SelectCase::evalBranch() {
    vmint value = select->evalInt();
    for (vmint i = 0; i < branches.size(); ++i) {
        if (branches.at(i).from && branches.at(i).to) { // i.e. "case 4 to 7" ...
            if (branches.at(i).from->evalInt() <= value &&
                branches.at(i).to->evalInt() >= value) return i;
        } else { // i.e. "case 5" ...
            if (branches.at(i).from->evalInt() == value) return i;
        }
    }
    return -1;
}

Statements* SelectCase::branch(vmuint i) const {
    if (i < branches.size())
        return const_cast<Statements*>( &*branches[i].statements );
    return NULL;
}

bool SelectCase::isPolyphonic() const {
    if (select->isPolyphonic()) return true;
    for (vmint i = 0; i < branches.size(); ++i)
        if (branches[i].statements->isPolyphonic())
            return true;
    return false;
}

void While::dump(int level) {
    printIndents(level);
    if (m_condition)
        if (m_condition->isConstExpr())
            printf("while (%lld) {\n", m_condition->evalInt());
        else
            printf("while ([runtime expr]) {\n");
    else
        printf("while ([INVALID]) {\n");
    m_statements->dump(level+1);
    printIndents(level);
    printf("}\n");
}

Statements* While::statements() const {
    return (m_statements) ? const_cast<Statements*>( &*m_statements ) : NULL;
}

bool While::evalLoopStartCondition() {
    if (!m_condition) return false;
    return m_condition->evalInt();
}

void SyncBlock::dump(int level) {
    printIndents(level);
    printf("sync {\n");
    m_statements->dump(level+1);
    printIndents(level);
    printf("}\n");
}

Statements* SyncBlock::statements() const {
    return (m_statements) ? const_cast<Statements*>( &*m_statements ) : NULL;
}

String Neg::evalCastToStr() {
    return expr->evalCastToStr();
}

void Neg::dump(int level) {
    printIndents(level);
    printf("Negative Expr\n");
}

String ConcatString::evalStr() {
    // temporaries required here to enforce the associative left (to right) order
    // ( required for GCC and Visual Studio, see:
    //   http://stackoverflow.com/questions/25842902/why-stdstring-concatenation-operator-works-like-right-associative-one
    //   Personally I am not convinced that this is "not a bug" of the
    //   compiler/STL implementation and the allegedly underlying "function call"
    //   nature causing this is IMO no profound reason that the C++ language's
    //   "+" operator's left associativity is ignored. -- Christian, 2016-07-14 )
    String l = lhs->evalCastToStr();
    String r = rhs->evalCastToStr();
    return l + r;
}

void ConcatString::dump(int level) {
    printIndents(level);
    printf("ConcatString(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")");
}

bool ConcatString::isConstExpr() const {
    return lhs->isConstExpr() && rhs->isConstExpr();
}

Relation::Relation(ExpressionRef lhs, Type type, ExpressionRef rhs) :
    Unit(VM_NO_UNIT),
    lhs(lhs), rhs(rhs), type(type)
{
}

// Equal / unequal comparison of real numbers in NKSP scripts:
//
// Unlike system level languages like C/C++ we are less conservative about
// comparing floating point numbers for 'equalness' or 'unequalness' in NKSP
// scripts. Due to the musical context of the NKSP language we automatically
// take the (to be) expected floating point tolerances into account when
// comparing two floating point numbers with each other, however only for '='
// and '#' operators. The '<=' and '>=' still use conservative low level
// floating point comparison for not breaking their transitivity feature.

template<typename T_LHS, typename T_RHS>
struct RelComparer {
    static inline bool isEqual(T_LHS a, T_RHS b) { // for int comparison ('3 = 3')
        return a == b;
    }
    static inline bool isUnequal(T_LHS a, T_RHS b) { // for int comparison ('3 # 3')
        return a != b;
    }
};

template<>
struct RelComparer<float,float> {
    static inline bool isEqual(float a, float b) { // for real number comparison ('3.1 = 3.1')
        return RTMath::fEqual32(a, b);
    }
    static inline bool isUnequal(float a, float b) { // for real number comparison ('3.1 # 3.1')
        return !RTMath::fEqual32(a, b);
    }
};

template<>
struct RelComparer<double,double> {
    static inline bool isEqual(double a, double b) { // for future purpose
        return RTMath::fEqual64(a, b);
    }
    static inline bool isUnqqual(double a, double b) { // for future purpose
        return !RTMath::fEqual64(a, b);
    }
};

template<class T_LHS, class T_RHS>
inline vmint _evalRelation(Relation::Type type, T_LHS lhs, T_RHS rhs) {
    switch (type) {
        case Relation::LESS_THAN:
            return lhs < rhs;
        case Relation::GREATER_THAN:
            return lhs > rhs;
        case Relation::LESS_OR_EQUAL:
            return lhs <= rhs;
        case Relation::GREATER_OR_EQUAL:
            return lhs >= rhs;
        case Relation::EQUAL:
            return RelComparer<typeof(lhs),typeof(rhs)>::isEqual(lhs, rhs);
        case Relation::NOT_EQUAL:
            return RelComparer<typeof(lhs),typeof(rhs)>::isUnequal(lhs, rhs);
    }
    return 0;
}

template<class T_LVALUE, class T_RVALUE, class T_LEXPR, class T_REXPR>
inline vmint _evalRealRelation(Relation::Type type,
                               T_LVALUE lvalue, T_RVALUE rvalue,
                               T_LEXPR* pLHS, T_REXPR* pRHS)
{
    if (pLHS->unitFactor() == pRHS->unitFactor())
        return _evalRelation(type, lvalue, rvalue);
    if (pLHS->unitFactor() < pRHS->unitFactor())
        return _evalRelation(type, lvalue, Unit::convRealToUnitFactor(rvalue, pRHS, pLHS));
    else
        return _evalRelation(type, Unit::convRealToUnitFactor(lvalue, pLHS, pRHS), rvalue);
}

template<class T_LEXPR, class T_REXPR>
inline vmint _evalIntRelation(Relation::Type type,
                              vmint lvalue, vmint rvalue,
                              T_LEXPR* pLHS, T_REXPR* pRHS)
{
    if (pLHS->unitFactor() == pRHS->unitFactor())
        return _evalRelation(type, lvalue, rvalue);
    if (pLHS->unitFactor() < pRHS->unitFactor())
        return _evalRelation(type, lvalue, Unit::convIntToUnitFactor(rvalue, pRHS, pLHS));
    else
        return _evalRelation(type, Unit::convIntToUnitFactor(lvalue, pLHS, pRHS), rvalue);
}

vmint Relation::evalInt() {
    const ExprType_t lType = lhs->exprType();
    const ExprType_t rType = rhs->exprType();
    if (lType == STRING_EXPR || rType == STRING_EXPR) {
        switch (type) {
            case EQUAL:
                return lhs->evalCastToStr() == rhs->evalCastToStr();
            case NOT_EQUAL:
                return lhs->evalCastToStr() != rhs->evalCastToStr();
            default:
                return 0;
        }
    } else if (lType == REAL_EXPR && rType == REAL_EXPR) {
        vmfloat lvalue = lhs->asReal()->evalReal();
        vmfloat rvalue = rhs->asReal()->evalReal();
        return _evalRealRelation(
            type, lvalue, rvalue, lhs->asReal(), rhs->asReal()
        );
    } else if (lType == REAL_EXPR && rType == INT_EXPR) {
        vmfloat lvalue = lhs->asReal()->evalReal();
        vmint rvalue = rhs->asInt()->evalInt();
        return _evalRealRelation(
            type, lvalue, rvalue, lhs->asReal(), rhs->asInt()
        );
    } else if (lType == INT_EXPR && rType == REAL_EXPR) {
        vmint lvalue = lhs->asInt()->evalInt();
        vmfloat rvalue = rhs->asReal()->evalReal();
        return _evalRealRelation(
            type, lvalue, rvalue, lhs->asInt(), rhs->asReal()
        );
    } else {
        vmint lvalue = lhs->asInt()->evalInt();
        vmint rvalue = rhs->asInt()->evalInt();
        return _evalIntRelation(
            type, lvalue, rvalue, lhs->asInt(), rhs->asInt()
        );
    }
}

void Relation::dump(int level) {
    printIndents(level);
    printf("Relation(\n");
    lhs->dump(level+1);
    printIndents(level);
    switch (type) {
        case LESS_THAN:
            printf("LESS_THAN\n");
            break;
        case GREATER_THAN:
            printf("GREATER_THAN\n");
            break;
        case LESS_OR_EQUAL:
            printf("LESS_OR_EQUAL\n");
            break;
        case GREATER_OR_EQUAL:
            printf("GREATER_OR_EQUAL\n");
            break;
        case EQUAL:
            printf("EQUAL\n");
            break;
        case NOT_EQUAL:
            printf("NOT_EQUAL\n");
            break;
    }
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

bool Relation::isConstExpr() const {
    return lhs->isConstExpr() && rhs->isConstExpr();
}

vmint Or::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    if (pLHS->evalInt()) return 1;
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    return (pRHS->evalInt()) ? 1 : 0;
}

void Or::dump(int level) {
    printIndents(level);
    printf("Or(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

vmint BitwiseOr::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    return pLHS->evalInt() | pRHS->evalInt();
}

void BitwiseOr::dump(int level) {
    printIndents(level);
    printf("BitwiseOr(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

vmint And::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    if (!pLHS->evalInt()) return 0;
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    return (pRHS->evalInt()) ? 1 : 0;
}

void And::dump(int level) {
    printIndents(level);
    printf("And(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

vmint BitwiseAnd::evalInt() {
    IntExpr* pLHS = dynamic_cast<IntExpr*>(&*lhs);
    IntExpr* pRHS = dynamic_cast<IntExpr*>(&*rhs);
    return pLHS->evalInt() & pRHS->evalInt();
}

void BitwiseAnd::dump(int level) {
    printIndents(level);
    printf("BitwiseAnd(\n");
    lhs->dump(level+1);
    printIndents(level);
    printf(",\n");
    rhs->dump(level+1);
    printIndents(level);
    printf(")\n");
}

void Not::dump(int level) {
    printIndents(level);
    printf("Not(\n");
    expr->dump(level+1);
    printIndents(level);
    printf(")\n");
}

void BitwiseNot::dump(int level) {
    printIndents(level);
    printf("BitwiseNot(\n");
    expr->dump(level+1);
    printIndents(level);
    printf(")\n");
}

String Final::evalCastToStr() {
    if (exprType() == REAL_EXPR)
        return ToString(evalReal());
    else
        return ToString(evalInt());
}

void Final::dump(int level) {
    printIndents(level);
    printf("Final(\n");
    expr->dump(level+1);
    printIndents(level);
    printf(")\n");
}

StatementsRef ParserContext::userFunctionByName(const String& name) {
    if (!userFnTable.count(name)) {
        return StatementsRef();
    }
    return userFnTable.find(name)->second;
}

VariableRef ParserContext::variableByName(const String& name) {
    if (!vartable.count(name)) {
        return VariableRef();
    }
    return vartable.find(name)->second;
}

VariableRef ParserContext::globalVar(const String& name) {
    if (!vartable.count(name)) {
        //printf("No global var '%s'\n", name.c_str());
        //for (std::map<String,VariableRef>::const_iterator it = vartable.begin(); it != vartable.end(); ++it)
        //    printf("-> var '%s'\n", it->first.c_str());
        return VariableRef();
    }
    return vartable.find(name)->second;
}

IntVariableRef ParserContext::globalIntVar(const String& name) {
    return globalVar(name);
}

RealVariableRef ParserContext::globalRealVar(const String& name) {
    return globalVar(name);
}

StringVariableRef ParserContext::globalStrVar(const String& name) {
    return globalVar(name);
}

ParserContext::~ParserContext() {
    destroyScanner();
    if (globalIntMemory) {
        delete globalIntMemory;
        globalIntMemory = NULL;
    }
    if (globalRealMemory) {
        delete globalRealMemory;
        globalRealMemory = NULL;
    }
}

void ParserContext::addErr(int firstLine, int lastLine, int firstColumn, int lastColumn, const char* txt) {
    ParserIssue e;
    e.type = PARSER_ERROR;
    e.txt = txt;
    e.firstLine = firstLine;
    e.lastLine = lastLine;
    e.firstColumn = firstColumn;
    e.lastColumn = lastColumn;
    vErrors.push_back(e);
    vIssues.push_back(e);
}

void ParserContext::addWrn(int firstLine, int lastLine, int firstColumn, int lastColumn, const char* txt) {
    ParserIssue w;
    w.type = PARSER_WARNING;
    w.txt = txt;
    w.firstLine = firstLine;
    w.lastLine = lastLine;
    w.firstColumn = firstColumn;
    w.lastColumn = lastColumn;
    vWarnings.push_back(w);
    vIssues.push_back(w);
}

void ParserContext::addPreprocessorComment(int firstLine, int lastLine, int firstColumn, int lastColumn) {
    CodeBlock block;
    block.firstLine = firstLine;
    block.lastLine = lastLine;
    block.firstColumn = firstColumn;
    block.lastColumn = lastColumn;
    vPreprocessorComments.push_back(block);
}

bool ParserContext::setPreprocessorCondition(const char* name) {
    if (builtinPreprocessorConditions.count(name)) return false;
    if (userPreprocessorConditions.count(name)) return false;
    userPreprocessorConditions.insert(name);
    return true;
}

bool ParserContext::resetPreprocessorCondition(const char* name) {
    if (builtinPreprocessorConditions.count(name)) return false;
    if (!userPreprocessorConditions.count(name)) return false;
    userPreprocessorConditions.erase(name);
    return true;
}

bool ParserContext::isPreprocessorConditionSet(const char* name) {
    if (builtinPreprocessorConditions.count(name)) return true;
    return userPreprocessorConditions.count(name);
}

std::vector<ParserIssue> ParserContext::issues() const {
    return vIssues;
}

std::vector<ParserIssue> ParserContext::errors() const {
    return vErrors;
}

std::vector<ParserIssue> ParserContext::warnings() const {
    return vWarnings;
}

std::vector<CodeBlock> ParserContext::preprocessorComments() const {
    return vPreprocessorComments;
}

VMEventHandler* ParserContext::eventHandler(uint index) {
    if (!handlers) return NULL;
    return handlers->eventHandler(index);
}

VMEventHandler* ParserContext::eventHandlerByName(const String& name) {
    if (!handlers) return NULL;
    return handlers->eventHandlerByName(name);
}

void ParserContext::registerBuiltInConstIntVariables(const std::map<String,vmint>& vars) {
    for (std::map<String,vmint>::const_iterator it = vars.begin();
         it != vars.end(); ++it)
    {
        ConstIntVariableRef ref = new ConstIntVariable({
            .value = it->second
        });
        vartable[it->first] = ref;
    }
}

void ParserContext::registerBuiltInConstRealVariables(const std::map<String,vmfloat>& vars) {
    for (std::map<String,vmfloat>::const_iterator it = vars.begin();
         it != vars.end(); ++it)
    {
        ConstRealVariableRef ref = new ConstRealVariable({
            .value = it->second
        });
        vartable[it->first] = ref;
    }
}

void ParserContext::registerBuiltInIntVariables(const std::map<String,VMIntPtr*>& vars) {
    for (std::map<String,VMIntPtr*>::const_iterator it = vars.begin();
         it != vars.end(); ++it)
    {
        BuiltInIntVariableRef ref = new BuiltInIntVariable(it->first, it->second);
        vartable[it->first] = ref;
    }
}

void ParserContext::registerBuiltInIntArrayVariables(const std::map<String,VMInt8Array*>& vars) {
    for (std::map<String,VMInt8Array*>::const_iterator it = vars.begin();
         it != vars.end(); ++it)
    {
        BuiltInIntArrayVariableRef ref = new BuiltInIntArrayVariable(it->first, it->second);
        vartable[it->first] = ref;
    }
}

void ParserContext::registerBuiltInDynVariables(const std::map<String,VMDynVar*>& vars) {
    for (std::map<String,VMDynVar*>::const_iterator it = vars.begin();
         it != vars.end(); ++it)
    {
        DynamicVariableCallRef ref = new DynamicVariableCall(it->first, this, it->second);
        vartable[it->first] = ref;
    }
}

ExecContext::ExecContext() :
    status(VM_EXEC_NOT_RUNNING), flags(STMT_SUCCESS), stackFrame(-1),
    suspendMicroseconds(0), instructionsCount(0)
{
    exitRes.value = NULL;
}

void ExecContext::forkTo(VMExecContext* ectx) const {
    ExecContext* child = dynamic_cast<ExecContext*>(ectx);

    child->polyphonicIntMemory.copyFlatFrom(polyphonicIntMemory);
    child->polyphonicRealMemory.copyFlatFrom(polyphonicRealMemory);
    child->status = VM_EXEC_SUSPENDED;
    child->flags = STMT_SUCCESS;
    child->stack.copyFlatFrom(stack);
    child->stackFrame = stackFrame;
    child->suspendMicroseconds = 0;
    child->instructionsCount = 0;
}

} // namespace LinuxSampler
