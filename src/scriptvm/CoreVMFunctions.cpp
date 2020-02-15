/*
 * Copyright (c) 2014-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "CoreVMFunctions.h"

#include <iostream>
#include <algorithm> // for std::sort()
#include <math.h>
#include <stdlib.h>
#include "tree.h"
#include "ScriptVM.h"
#include "../common/RTMath.h"

namespace LinuxSampler {

inline bool _fEqualX(vmfloat a, vmfloat b) {
    if (sizeof(vmfloat) == sizeof(float))
        return RTMath::fEqual32(a, b);
    else
        return RTMath::fEqual64(a, b);
}

///////////////////////////////////////////////////////////////////////////
// class VMIntResult

VMIntResult::VMIntResult() :
    flags(STMT_SUCCESS), value(0), unitPrefixFactor(VM_NO_FACTOR),
    unitBaseType(VM_NO_UNIT)
{
}

///////////////////////////////////////////////////////////////////////////
// class VMRealResult

VMRealResult::VMRealResult() :
    flags(STMT_SUCCESS), value(0), unitPrefixFactor(VM_NO_FACTOR),
    unitBaseType(VM_NO_UNIT)
{

}

///////////////////////////////////////////////////////////////////////////
// class VMEmptyResultFunction

VMFnResult* VMEmptyResultFunction::errorResult() {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    return &result;
}

VMFnResult* VMEmptyResultFunction::successResult() {
    result.flags = STMT_SUCCESS;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// class VMIntResultFunction

VMFnResult* VMIntResultFunction::errorResult(vmint i) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = i;
    result.unitPrefixFactor = VM_NO_FACTOR;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

VMFnResult* VMIntResultFunction::successResult(vmint i) {
    result.flags = STMT_SUCCESS;
    result.value = i;
    result.unitPrefixFactor = VM_NO_FACTOR;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

VMFnResult* VMIntResultFunction::errorResult(VMIntFnResDef res) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = res.value;
    result.unitPrefixFactor = res.unitFactor;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

VMFnResult* VMIntResultFunction::successResult(VMIntFnResDef res) {
    result.flags = STMT_SUCCESS;
    result.value = res.value;
    result.unitPrefixFactor = res.unitFactor;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// class VMRealResultFunction

VMFnResult* VMRealResultFunction::errorResult(vmfloat f) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = f;
    result.unitPrefixFactor = VM_NO_FACTOR;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

VMFnResult* VMRealResultFunction::errorResult(VMRealFnResDef res) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = res.value;
    result.unitPrefixFactor = res.unitFactor;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

VMFnResult* VMRealResultFunction::successResult(vmfloat f) {
    result.flags = STMT_SUCCESS;
    result.value = f;
    result.unitPrefixFactor = VM_NO_FACTOR;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

VMFnResult* VMRealResultFunction::successResult(VMRealFnResDef res) {
    result.flags = STMT_SUCCESS;
    result.value = res.value;
    result.unitPrefixFactor = res.unitFactor;
    result.unitBaseType = VM_NO_UNIT;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// class VMStringResultFunction

VMFnResult* VMStringResultFunction::errorResult(const String& s) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = s;
    return &result;
}

VMFnResult* VMStringResultFunction::successResult(const String& s) {
    result.flags = STMT_SUCCESS;
    result.value = s;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// class VMNumberResultFunction

VMFnResult* VMNumberResultFunction::errorResult(vmint i) {
    intResult.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    intResult.value = i;
    intResult.unitPrefixFactor = VM_NO_FACTOR;
    intResult.unitBaseType = VM_NO_UNIT;
    return &intResult;
}

VMFnResult* VMNumberResultFunction::errorResult(vmfloat f) {
    realResult.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    realResult.value = f;
    realResult.unitPrefixFactor = VM_NO_FACTOR;
    intResult.unitBaseType = VM_NO_UNIT;
    return &realResult;
}

VMFnResult* VMNumberResultFunction::successResult(vmint i) {
    intResult.flags = STMT_SUCCESS;
    intResult.value = i;
    intResult.unitPrefixFactor = VM_NO_FACTOR;
    intResult.unitBaseType = VM_NO_UNIT;
    return &intResult;
}

VMFnResult* VMNumberResultFunction::successResult(vmfloat f) {
    realResult.flags = STMT_SUCCESS;
    realResult.value = f;
    realResult.unitPrefixFactor = VM_NO_FACTOR;
    realResult.unitBaseType = VM_NO_UNIT;
    return &realResult;
}

VMFnResult* VMNumberResultFunction::errorIntResult(VMIntFnResDef res) {
    intResult.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    intResult.value = res.value;
    intResult.unitPrefixFactor = res.unitFactor;
    intResult.unitBaseType = VM_NO_UNIT;
    return &intResult;
}

VMFnResult* VMNumberResultFunction::errorRealResult(VMRealFnResDef res) {
    realResult.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    realResult.value = res.value;
    realResult.unitPrefixFactor = res.unitFactor;
    realResult.unitBaseType = VM_NO_UNIT;
    return &realResult;
}

VMFnResult* VMNumberResultFunction::successIntResult(VMIntFnResDef res) {
    intResult.flags = STMT_SUCCESS;
    intResult.value = res.value;
    intResult.unitPrefixFactor = res.unitFactor;
    intResult.unitBaseType = VM_NO_UNIT;
    return &intResult;
}

VMFnResult* VMNumberResultFunction::successRealResult(VMRealFnResDef res) {
    realResult.flags = STMT_SUCCESS;
    realResult.value = res.value;
    realResult.unitPrefixFactor = res.unitFactor;
    realResult.unitBaseType = VM_NO_UNIT;
    return &realResult;
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  message()

bool CoreVMFunction_message::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR || type == STRING_EXPR;
}

VMFnResult* CoreVMFunction_message::exec(VMFnArgs* args) {
    if (!args->argsCount()) return errorResult();

    uint64_t usecs = RTMath::unsafeMicroSeconds(RTMath::real_clock);

    VMStringExpr* strExpr = dynamic_cast<VMStringExpr*>(args->arg(0));
    if (strExpr) {
        printf("[ScriptVM %.3f] %s\n", usecs/1000000.f, strExpr->evalStr().c_str());
        return successResult();
    }

    VMRealExpr* realExpr = dynamic_cast<VMRealExpr*>(args->arg(0));
    if (realExpr) {
        printf("[ScriptVM %.3f] %f\n", usecs/1000000.f, realExpr->evalReal());
        return successResult();
    }

    VMIntExpr* intExpr = dynamic_cast<VMIntExpr*>(args->arg(0));
    if (intExpr) {
        printf("[ScriptVM %.3f] %" PRId64 "\n", usecs/1000000.f, (int64_t)intExpr->evalInt());
        return successResult();
    }

    return errorResult();
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  exit()

vmint CoreVMFunction_exit::maxAllowedArgs() const {
    return (vm->isExitResultEnabled()) ? 1 : 0;
}

bool CoreVMFunction_exit::acceptsArgType(vmint iArg, ExprType_t type) const {
    if (!vm->isExitResultEnabled()) return false;
    return type == INT_EXPR || type == REAL_EXPR || type == STRING_EXPR;
}

bool CoreVMFunction_exit::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
    if (!vm->isExitResultEnabled()) return false;
    return true;
}

bool CoreVMFunction_exit::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
    if (!vm->isExitResultEnabled()) return false;
    return true;
}
bool CoreVMFunction_exit::acceptsArgFinal(vmint iArg) const {
    if (!vm->isExitResultEnabled()) return false;
    return true;
}

VMFnResult* CoreVMFunction_exit::exec(VMFnArgs* args) {
    this->result.flags = STMT_ABORT_SIGNALLED;
    if (vm->isExitResultEnabled() && args->argsCount()) {
        ExecContext* ctx = dynamic_cast<ExecContext*>(vm->currentVMExecContext());
        switch (args->arg(0)->exprType()) {
            case INT_EXPR: {
                VMIntExpr* expr = args->arg(0)->asInt();
                ctx->exitRes.intLiteral = IntLiteral({
                    .value = expr->evalInt(),
                    .unitFactor = expr->unitFactor(),
                    .unitType = expr->unitType(),
                    .isFinal = expr->isFinal()
                });
                ctx->exitRes.value = &ctx->exitRes.intLiteral;
                break;
            }
            case REAL_EXPR: {
                VMRealExpr* expr = args->arg(0)->asReal();
                ctx->exitRes.realLiteral = RealLiteral({
                    .value = expr->evalReal(),
                    .unitFactor = expr->unitFactor(),
                    .unitType = expr->unitType(),
                    .isFinal = expr->isFinal()
                });
                ctx->exitRes.value = &ctx->exitRes.realLiteral;
                break;
            }
            case STRING_EXPR:
                ctx->exitRes.stringLiteral = StringLiteral(
                    args->arg(0)->asString()->evalStr()
                );
                ctx->exitRes.value = &ctx->exitRes.stringLiteral;
                break;
            default:
                ; // noop - just to shut up the compiler
        }
    }
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  wait()

bool CoreVMFunction_wait::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR;
}

bool CoreVMFunction_wait::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
    return type == VM_NO_UNIT || type == VM_SECOND;
}

bool CoreVMFunction_wait::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
    return type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
}

VMFnResult* CoreVMFunction_wait::exec(VMFnArgs* args) {
    ExecContext* ctx = dynamic_cast<ExecContext*>(vm->currentVMExecContext());
    VMNumberExpr* expr = args->arg(0)->asNumber();
    StdUnit_t unit = expr->unitType();
    vmint us = (unit) ? expr->evalCastInt(VM_MICRO) : expr->evalCastInt();
    if (us < 0) {
        wrnMsg("wait(): argument may not be negative! Aborting script!");
        this->result.flags = STMT_ABORT_SIGNALLED;
    } else if (us == 0) {
        wrnMsg("wait(): argument may not be zero! Aborting script!");
        this->result.flags = STMT_ABORT_SIGNALLED;
    } else {
        ctx->suspendMicroseconds = us;
        this->result.flags = STMT_SUSPEND_SIGNALLED;
    }
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  abs()

ExprType_t CoreVMFunction_abs::returnType(VMFnArgs* args) {
    return args->arg(0)->exprType();
}

StdUnit_t CoreVMFunction_abs::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_abs::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

bool CoreVMFunction_abs::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR;
}

VMFnResult* CoreVMFunction_abs::exec(VMFnArgs* args) {
    VMExpr* arg = args->arg(0);
    if (arg->exprType() == REAL_EXPR) {
        VMRealExpr* expr = arg->asReal();
        return successRealResult({
            .value = static_cast<vmfloat>(::fabs(expr->evalReal())),
            .unitFactor = expr->unitFactor()
        });
    } else {
        VMIntExpr* expr = arg->asInt();
        return successIntResult({
            .value = std::abs(expr->evalInt()),
            .unitFactor = expr->unitFactor()
        });
    }
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  random()

ExprType_t CoreVMFunction_random::returnType(VMFnArgs* args) {
    return (args->arg(0)->exprType() == INT_EXPR &&
            args->arg(1)->exprType() == INT_EXPR) ? INT_EXPR : REAL_EXPR;
}

StdUnit_t CoreVMFunction_random::returnUnitType(VMFnArgs* args) {
    // we ensure in checkArgs() below (which is called before this method here)
    // that both arguments must be of same unit type, so either one is fine here
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_random::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal() ||
           args->arg(1)->asNumber()->isFinal();
}

bool CoreVMFunction_random::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR;
}

void CoreVMFunction_random::checkArgs(VMFnArgs* args,
                                      std::function<void(String)> err,
                                      std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->asNumber()->unitType() !=
        args->arg(1)->asNumber()->unitType())
    {
        String a = unitTypeStr(args->arg(0)->asNumber()->unitType());
        String b = unitTypeStr(args->arg(1)->asNumber()->unitType());
        err("Argument 1 has unit type " + a + ", whereas argument 2 has unit type " + b + ".");
        return;
    }
    if (args->arg(0)->asNumber()->isFinal() !=
        args->arg(1)->asNumber()->isFinal())
    {
        String a = args->arg(0)->asNumber()->isFinal() ? "'final'" : "not 'final'";
        String b = args->arg(1)->asNumber()->isFinal() ? "'final'" : "not 'final'";
        wrn("Argument 1 is " + a + ", whereas argument 2 is " + b + ", function result will be final.");
    }
}

VMFnResult* CoreVMFunction_random::exec(VMFnArgs* args) {
    float f = float(::rand()) / float(RAND_MAX);

    VMNumberExpr* arg0 = args->arg(0)->asNumber();
    VMNumberExpr* arg1 = args->arg(1)->asNumber();

    if (arg0->exprType() == INT_EXPR && arg1->exprType() == INT_EXPR) {
        vmint iMin = args->arg(0)->asInt()->evalInt();
        vmint iMax = args->arg(1)->asInt()->evalInt();
        if (arg0->unitFactor() == arg1->unitFactor()) {
            return successIntResult({
                .value = vmint( iMin + roundf( f * float(iMax - iMin) ) ),
                .unitFactor = arg0->unitFactor()
            });
        } else if (arg0->unitFactor() < arg1->unitFactor()) {
            iMax = Unit::convIntToUnitFactor(iMax, arg1, arg0);
            return successIntResult({
                .value = vmint( iMin + roundf( f * float(iMax - iMin) ) ),
                .unitFactor = arg0->unitFactor()
            });
        } else { // arg0->unitFactor() > arg1->unitFactor() ...
            iMin = Unit::convIntToUnitFactor(iMin, arg0, arg1);
            return successIntResult({
                .value = vmint( iMin + roundf( f * float(iMax - iMin) ) ),
                .unitFactor = arg1->unitFactor()
            });
        }
    } else {
        vmfloat fMin = arg0->evalCastReal();
        vmfloat fMax = arg1->evalCastReal();
        if (arg0->unitFactor() == arg1->unitFactor()) {
            return successRealResult({
                .value = fMin + f * (fMax - fMin),
                .unitFactor = arg0->unitFactor()
            });
        } else if (arg0->unitFactor() < arg1->unitFactor()) {
            fMax = Unit::convRealToUnitFactor(fMax, arg1, arg0);
            return successRealResult({
                .value = fMin + f * (fMax - fMin),
                .unitFactor = arg0->unitFactor()
            });
        } else { // arg0->unitFactor() > arg1->unitFactor() ...
            fMin = Unit::convRealToUnitFactor(fMin, arg0, arg1);
            return successRealResult({
                .value = fMin + f * (fMax - fMin),
                .unitFactor = arg1->unitFactor()
            });
        }
    }
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  num_elements()

bool CoreVMFunction_num_elements::acceptsArgType(vmint iArg, ExprType_t type) const {
    return isArray(type);
}

VMFnResult* CoreVMFunction_num_elements::exec(VMFnArgs* args) {
    return successResult( args->arg(0)->asArray()->arraySize() );
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  inc()

StdUnit_t CoreVMFunction_inc::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_inc::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

void CoreVMFunction_inc::checkArgs(VMFnArgs* args,
                                   std::function<void(String)> err,
                                   std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->asNumber()->unitType()) {
        String unitType = unitTypeStr(args->arg(0)->asNumber()->unitType());
        wrn("Argument has a unit type (" + unitType + "), only the number before the unit will be incremented by one.");
    }
}

VMFnResult* CoreVMFunction_inc::exec(VMFnArgs* args) {
    VMExpr* arg = args->arg(0);
    VMIntExpr* in = dynamic_cast<VMIntExpr*>(arg);
    VMVariable* out = dynamic_cast<VMVariable*>(arg);
    vmint i = in->evalInt() + 1;
    IntLiteral tmp({
        .value = i,
        .unitFactor = in->unitFactor()
    });
    out->assignExpr(&tmp);
    return successResult({
        .value = i,
        .unitFactor = in->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  dec()

StdUnit_t CoreVMFunction_dec::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_dec::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

void CoreVMFunction_dec::checkArgs(VMFnArgs* args,
                                   std::function<void(String)> err,
                                   std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->asNumber()->unitType()) {
        String unitType = unitTypeStr(args->arg(0)->asNumber()->unitType());
        wrn("Argument has a unit type (" + unitType + "), only the number before the unit will be decremented by one.");
    }
}

VMFnResult* CoreVMFunction_dec::exec(VMFnArgs* args) {
    VMExpr* arg = args->arg(0);
    VMIntExpr* in = dynamic_cast<VMIntExpr*>(arg);
    VMVariable* out = dynamic_cast<VMVariable*>(arg);
    vmint i = in->evalInt() - 1;
    IntLiteral tmp({
        .value = i,
        .unitFactor = in->unitFactor()
    });
    out->assignExpr(&tmp);
    return successResult({
        .value = i,
        .unitFactor = in->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  in_range()

bool CoreVMFunction_in_range::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR;
}

void CoreVMFunction_in_range::checkArgs(VMFnArgs* args,
                                        std::function<void(String)> err,
                                        std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->asNumber()->unitType() !=
        args->arg(1)->asNumber()->unitType() ||
        args->arg(1)->asNumber()->unitType() !=
        args->arg(2)->asNumber()->unitType())
    {
        String a = unitTypeStr(args->arg(0)->asNumber()->unitType());
        String b = unitTypeStr(args->arg(1)->asNumber()->unitType());
        String c = unitTypeStr(args->arg(2)->asNumber()->unitType());
        err("Arguments must all have same unit, however argument 1 is " + a +
            ", argument 2 is " + b + ", argument 3 is " + c + ".");
        return;
    }
    if (args->arg(0)->exprType() != args->arg(1)->exprType() ||
        args->arg(1)->exprType() != args->arg(2)->exprType())
    {
        String a = typeStr(args->arg(0)->exprType());
        String b = typeStr(args->arg(1)->exprType());
        String c = typeStr(args->arg(2)->exprType());
        String r = typeStr(REAL_EXPR);
        wrn("Argument 1 is " + a + ", argument 2 is " + b +
            ", argument 3 is " + c + ", function result will be " + r + ".");
    }
}

template<class T>
inline void _swapByValue(T& a, T& b) {
    T tmp = a;
    a = b;
    b = tmp;
}

VMFnResult* CoreVMFunction_in_range::exec(VMFnArgs* args) {
    VMNumberExpr* argNeedle = args->arg(0)->asNumber();
    VMNumberExpr* argLo = args->arg(1)->asNumber();
    VMNumberExpr* argHi = args->arg(2)->asNumber();

    vmfloat needle = argNeedle->evalCastReal();
    vmfloat lo = argLo->evalCastReal();
    vmfloat hi = argHi->evalCastReal();

    needle *= argNeedle->unitFactor();
    lo *= argLo->unitFactor();
    hi *= argHi->unitFactor();

    if (lo > hi) _swapByValue(lo, hi);

    return successResult(needle >= lo && needle <= hi);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sh_left()

bool CoreVMFunction_sh_left::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_sh_left::exec(VMFnArgs* args) {
    vmint i = args->arg(0)->asInt()->evalInt();
    vmint n = args->arg(1)->asInt()->evalInt();
    return successResult(i << n);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sh_right()

bool CoreVMFunction_sh_right::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_sh_right::exec(VMFnArgs* args) {
    vmint i = args->arg(0)->asInt()->evalInt();
    vmint n = args->arg(1)->asInt()->evalInt();
    return successResult(i >> n);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  msb()

VMFnResult* CoreVMFunction_msb::exec(VMFnArgs* args) {
    vmint i = args->arg(0)->asInt()->evalInt();
    return successResult((i >> 7) & 127);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  lsb()

VMFnResult* CoreVMFunction_lsb::exec(VMFnArgs* args) {
    vmint i = args->arg(0)->asInt()->evalInt();
    return successResult(i & 127);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  min()

ExprType_t CoreVMFunction_min::returnType(VMFnArgs* args) {
    return (args->arg(0)->exprType() == REAL_EXPR ||
            args->arg(1)->exprType() == REAL_EXPR) ? REAL_EXPR : INT_EXPR;
}

StdUnit_t CoreVMFunction_min::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_min::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal() ||
           args->arg(1)->asNumber()->isFinal();
}

bool CoreVMFunction_min::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR;
}

void CoreVMFunction_min::checkArgs(VMFnArgs* args,
                                   std::function<void(String)> err,
                                   std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->asNumber()->unitType() !=
        args->arg(1)->asNumber()->unitType())
    {
        String a = unitTypeStr(args->arg(0)->asNumber()->unitType());
        String b = unitTypeStr(args->arg(1)->asNumber()->unitType());
        err("Argument 1 has unit type " + a + ", whereas argument 2 has unit type " + b + ".");
        return;
    }
    if (args->arg(0)->exprType() != args->arg(1)->exprType()) {
        String a = typeStr(args->arg(0)->exprType());
        String b = typeStr(args->arg(1)->exprType());
        String c = typeStr(REAL_EXPR);
        wrn("Argument 1 is " + a + ", whereas argument 2 is " + b + ", function result will be " + c + ".");
        return;
    }
    if (args->arg(0)->asNumber()->isFinal() !=
        args->arg(1)->asNumber()->isFinal())
    {
        String a = args->arg(0)->asNumber()->isFinal() ? "'final'" : "not 'final'";
        String b = args->arg(1)->asNumber()->isFinal() ? "'final'" : "not 'final'";
        wrn("Argument 1 is " + a + ", whereas argument 2 is " + b + ", function result will be final.");
    }
}

VMFnResult* CoreVMFunction_min::exec(VMFnArgs* args) {
    VMNumberExpr* lhs = args->arg(0)->asNumber();
    VMNumberExpr* rhs = args->arg(1)->asNumber();
    if (lhs->exprType() == REAL_EXPR && rhs->exprType() == REAL_EXPR) {
        vmfloat lm = lhs->asReal()->evalReal();
        vmfloat rm = rhs->asReal()->evalReal();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successRealResult({
            .value = (lprod < rprod) ? lm : rm,
            .unitFactor = (lprod < rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    } else if (lhs->exprType() == REAL_EXPR && rhs->exprType() == INT_EXPR) {
        vmfloat lm = lhs->asReal()->evalReal();
        vmint   rm = rhs->asInt()->evalInt();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successRealResult({
            .value = (lprod < rprod) ? lm : rm,
            .unitFactor = (lprod < rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    } else if (lhs->exprType() == INT_EXPR && rhs->exprType() == REAL_EXPR) {
        vmint   lm = lhs->asInt()->evalInt();
        vmfloat rm = rhs->asReal()->evalReal();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successRealResult({
            .value = (lprod < rprod) ? lm : rm,
            .unitFactor = (lprod < rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    } else {
        vmint lm = lhs->asInt()->evalInt();
        vmint rm = rhs->asInt()->evalInt();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successIntResult({
            .value = (lprod < rprod) ? lm : rm,
            .unitFactor = (lprod < rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    }
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  max()

ExprType_t CoreVMFunction_max::returnType(VMFnArgs* args) {
    return (args->arg(0)->exprType() == REAL_EXPR ||
            args->arg(1)->exprType() == REAL_EXPR) ? REAL_EXPR : INT_EXPR;
}

StdUnit_t CoreVMFunction_max::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_max::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal() ||
           args->arg(1)->asNumber()->isFinal();
}

bool CoreVMFunction_max::acceptsArgType(vmint iArg, ExprType_t type) const {
    return type == INT_EXPR || type == REAL_EXPR;
}

void CoreVMFunction_max::checkArgs(VMFnArgs* args,
                                   std::function<void(String)> err,
                                   std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->asNumber()->unitType() !=
        args->arg(1)->asNumber()->unitType())
    {
        String a = unitTypeStr(args->arg(0)->asNumber()->unitType());
        String b = unitTypeStr(args->arg(1)->asNumber()->unitType());
        err("Argument 1 has unit type " + a + ", whereas argument 2 has unit type " + b + ".");
        return;
    }
    if (args->arg(0)->exprType() != args->arg(1)->exprType()) {
        String a = typeStr(args->arg(0)->exprType());
        String b = typeStr(args->arg(1)->exprType());
        String c = typeStr(REAL_EXPR);
        wrn("Argument 1 is " + a + ", whereas argument 2 is " + b + ", function result will be " + c + ".");
        return;
    }
    if (args->arg(0)->asNumber()->isFinal() !=
        args->arg(1)->asNumber()->isFinal())
    {
        String a = args->arg(0)->asNumber()->isFinal() ? "'final'" : "not 'final'";
        String b = args->arg(1)->asNumber()->isFinal() ? "'final'" : "not 'final'";
        wrn("Argument 1 is " + a + ", whereas argument 2 is " + b + ", function result will be final.");
    }
}

VMFnResult* CoreVMFunction_max::exec(VMFnArgs* args) {
    VMNumberExpr* lhs = args->arg(0)->asNumber();
    VMNumberExpr* rhs = args->arg(1)->asNumber();
    if (lhs->exprType() == REAL_EXPR && rhs->exprType() == REAL_EXPR) {
        vmfloat lm = lhs->asReal()->evalReal();
        vmfloat rm = rhs->asReal()->evalReal();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successRealResult({
            .value = (lprod > rprod) ? lm : rm,
            .unitFactor = (lprod > rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    } else if (lhs->exprType() == REAL_EXPR && rhs->exprType() == INT_EXPR) {
        vmfloat lm = lhs->asReal()->evalReal();
        vmint   rm = rhs->asInt()->evalInt();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successRealResult({
            .value = (lprod > rprod) ? lm : rm,
            .unitFactor = (lprod > rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    } else if (lhs->exprType() == INT_EXPR && rhs->exprType() == REAL_EXPR) {
        vmint   lm = lhs->asInt()->evalInt();
        vmfloat rm = rhs->asReal()->evalReal();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successRealResult({
            .value = (lprod > rprod) ? lm : rm,
            .unitFactor = (lprod > rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    } else {
        vmint lm = lhs->asInt()->evalInt();
        vmint rm = rhs->asInt()->evalInt();
        vmfloat lprod = lm * lhs->unitFactor();
        vmfloat rprod = rm * rhs->unitFactor();
        return successIntResult({
            .value = (lprod > rprod) ? lm : rm,
            .unitFactor = (lprod > rprod) ? lhs->unitFactor() : rhs->unitFactor()
        });
    }
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  array_equal()

bool CoreVMFunction_array_equal::acceptsArgType(vmint iArg, ExprType_t type) const {
    return isArray(type);
}

void CoreVMFunction_array_equal::checkArgs(VMFnArgs* args,
                                           std::function<void(String)> err,
                                           std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->exprType() != args->arg(1)->exprType()) {
        String a = typeStr(args->arg(0)->exprType());
        String b = typeStr(args->arg(1)->exprType());
        err("Argument 1 is " + a + ", whereas argument 2 is " + b + ".");
        return;
    }
    if (args->arg(0)->asArray()->arraySize() !=
        args->arg(1)->asArray()->arraySize())
    {
        wrn("Result of function call is always false, since the passed two arrays were declared with different array sizes.");
    }
}

VMFnResult* CoreVMFunction_array_equal::exec(VMFnArgs* args) {
    VMArrayExpr* l = args->arg(0)->asArray();
    VMArrayExpr* r = args->arg(1)->asArray();
    if (l->arraySize() != r->arraySize()) {
        //wrnMsg("array_equal(): the two arrays differ in size");
        return successResult(0); // false
    }
    const vmint n = l->arraySize();
    // checkArgs() above ensured that we either have INT_ARR_EXPR on both sides
    // or REAL_ARR_EXPR on both sides, so we can simplify here (a bit)
    if (l->exprType() == INT_ARR_EXPR) {
        VMIntArrayExpr* lia = l->asIntArray();
        VMIntArrayExpr* ria = r->asIntArray();
        for (vmint i = 0; i < n; ++i) {
            vmint lvalue = lia->evalIntElement(i);
            vmint rvalue = ria->evalIntElement(i);
            vmfloat lfactor = lia->unitFactorOfElement(i);
            vmfloat rfactor = ria->unitFactorOfElement(i);
            if (lfactor == rfactor) {
                if (lvalue != rvalue)
                    return successResult(0); // false
                else
                    continue;
            }
            if (lfactor < rfactor) {
                if (lvalue != Unit::convIntToUnitFactor(rvalue, rfactor, lfactor))
                    return successResult(0); // false
                else
                    continue;
            } else {
                if (rvalue != Unit::convIntToUnitFactor(lvalue, lfactor, rfactor))
                    return successResult(0); // false
                else
                    continue;
            }
        }
    } else {
        VMRealArrayExpr* lra = l->asRealArray();
        VMRealArrayExpr* rra = r->asRealArray();
        for (vmint i = 0; i < n; ++i) {
            vmfloat lvalue = lra->evalRealElement(i);
            vmfloat rvalue = rra->evalRealElement(i);
            vmfloat lfactor = lra->unitFactorOfElement(i);
            vmfloat rfactor = rra->unitFactorOfElement(i);
            if (lfactor == rfactor) {
                if (!_fEqualX(lvalue, rvalue))
                    return successResult(0); // false
                else
                    continue;
            }
            if (lfactor < rfactor) {
                if (!_fEqualX(lvalue, Unit::convRealToUnitFactor(rvalue, rfactor, lfactor)))
                    return successResult(0); // false
                else
                    continue;
            } else {
                if (!_fEqualX(rvalue, Unit::convRealToUnitFactor(lvalue, lfactor, rfactor)))
                    return successResult(0); // false
                else
                    continue;
            }
        }
    }
    return successResult(1); // true
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  search()

bool CoreVMFunction_search::acceptsArgType(vmint iArg, ExprType_t type) const {
    if (iArg == 0)
        return isArray(type);
    else
        return type == INT_EXPR || type == REAL_EXPR;
}

void CoreVMFunction_search::checkArgs(VMFnArgs* args,
                                      std::function<void(String)> err,
                                      std::function<void(String)> wrn)
{
    // super class checks
    Super::checkArgs(args, err, wrn);

    // own checks ...
    if (args->arg(0)->exprType() == INT_ARR_EXPR &&
        args->arg(1)->exprType() != INT_EXPR)
    {
        String a = typeStr(INT_ARR_EXPR);
        String bIs = typeStr(args->arg(1)->exprType());
        String bShould = typeStr(INT_EXPR);
        err("Argument 1 is " + a + ", hence argument 2 should be " + bShould + ", is " + bIs + " though.");
        return;
    }
    if (args->arg(0)->exprType() == REAL_ARR_EXPR &&
        args->arg(1)->exprType() != REAL_EXPR)
    {
        String a = typeStr(REAL_ARR_EXPR);
        String bIs = typeStr(args->arg(1)->exprType());
        String bShould = typeStr(REAL_EXPR);
        err("Argument 1 is " + a + ", hence argument 2 should be " + bShould + ", is " + bIs + " though.");
        return;
    }
}

VMFnResult* CoreVMFunction_search::exec(VMFnArgs* args) {
    VMArrayExpr* a = args->arg(0)->asArray();
    const vmint n = a->arraySize();
    if (a->exprType() == INT_ARR_EXPR) {
        const vmint needle = args->arg(1)->asInt()->evalInt();
        VMIntArrayExpr* intArray = a->asIntArray();
        for (vmint i = 0; i < n; ++i)
            if (intArray->evalIntElement(i) == needle)
                return successResult(i);
    } else { // real array ...
        const vmfloat needle = args->arg(1)->asReal()->evalReal();
        VMRealArrayExpr* realArray = a->asRealArray();
        for (vmint i = 0; i < n; ++i) {
            const vmfloat value = realArray->evalRealElement(i);
            if (_fEqualX(value, needle))
                return successResult(i);
        }
    }
    return successResult(-1); // not found
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sort()

bool CoreVMFunction_sort::acceptsArgType(vmint iArg, ExprType_t type) const {
    if (iArg == 0)
        return isArray(type);
    else
        return type == INT_EXPR;
}

// The following structs and template classes act as adapters for allowing to
// use std sort algorithms on our arrays. It might look a bit more complicated
// than it ought to be, but there is one reason for the large amount of
// 'adapter' code below: the STL std algorithms rely on 'lvalues' to do their
// (e.g. sorting) jobs, that is they expect containers to have 'localizeable'
// data which essentially means their data should reside somewhere in memory and
// directly be accessible (readable and writable) there, which is not the case
// with our VM interfaces which actually always require virtual getter and
// setter methods to be called instead. So we must emulate lvalues by custom
// classes/structs which forward between our getters/setters and the lvalue
// access operators used by the STL std algorithms.

struct IntArrayAccessor {
    static inline vmint getPrimaryValue(VMIntArrayExpr* arr, vmint index) {
        return arr->evalIntElement(index);
    }
    static inline void setPrimaryValue(VMIntArrayExpr* arr, vmint index, vmint value) {
        arr->assignIntElement(index, value);
    }
};

struct RealArrayAccessor {
    static inline vmfloat getPrimaryValue(VMRealArrayExpr* arr, vmint index) {
        return arr->evalRealElement(index);
    }
    static inline void setPrimaryValue(VMRealArrayExpr* arr, vmint index, vmfloat value) {
        arr->assignRealElement(index, value);
    }
};

template<class T_array> // i.e. T_array is either VMIntArrayExpr or VMRealArrayExpr
struct ArrElemPOD {
    T_array* m_array;
    vmint m_index;
};

// This class is used for temporary values by std::sort().
template<class T_value> // i.e. T_value is either vmint or vmfloat
struct ScalarNmbrVal {
    T_value primValue;
    vmfloat unitFactor;

    inline bool operator<(const ScalarNmbrVal& other) const {
        return getProdValue() < other.getProdValue();
    }
    inline bool operator>(const ScalarNmbrVal& other) const {
        return getProdValue() > other.getProdValue();
    }
    inline vmfloat getProdValue() const {
        // simple solution for both vmint and vmfloat, should be fine for just sorting
        return primValue * unitFactor;
    }
};

// This class emulates lvalue access (access by reference) which is used by ArrExprIter::operator*() below.
template<class T_array, // T_array is either VMIntArrayExpr or VMRealArrayExpr
         class T_value, // T_value is either vmint or vmfloat
         class T_accessor> // T_accessor is either IntArrayAccessor or RealArrayAccessor
class ArrElemRef : protected ArrElemPOD<T_array> {
public:
    typedef ::LinuxSampler::ScalarNmbrVal<T_value> ScalarNmbrVal; // GCC 8.x requires this very detailed form of typedef (that is ::LinuxSampler:: as prefix), IMO a GCC bug

    inline ArrElemRef(T_array* a, vmint index) {
        this->m_array = a;
        this->m_index = index;
    }
    inline ArrElemRef(const ArrElemRef& ref) {
        this->m_array = ref.m_array;
        this->m_index = ref.m_index;
    }
    inline ArrElemRef& operator=(const ArrElemRef& e) {
        setPrimValue(e.getPrimValue());
        setUnitFactor(e.getUnitFactor());
        return *this;
    }
    inline ArrElemRef& operator=(ScalarNmbrVal value) {
        setPrimValue(value.primValue);
        setUnitFactor(value.unitFactor);
        return *this;
    }
    inline bool operator==(const ArrElemRef& e) const {
        return getProdValue() == e.getProdValue();
    }
    inline bool operator!=(const ArrElemRef& e) const {
        return !(operator==(e));
    }
    inline bool operator<(const ArrElemRef& e) const {
        return getProdValue() < e.getProdValue();
    }
    inline bool operator>(const ArrElemRef& e) const {
        return getProdValue() > e.getProdValue();
    }
    inline bool operator<=(const ArrElemRef& e) const {
        return getProdValue() <= e.getProdValue();
    }
    inline bool operator>=(const ArrElemRef& e) const {
        return getProdValue() >= e.getProdValue();
    }
    inline bool operator==(const ScalarNmbrVal& s) const {
        return getProdValue() == s.getProdValue();
    }
    inline bool operator!=(const ScalarNmbrVal& s) const {
        return !(operator==(s));
    }
    inline bool operator<(const ScalarNmbrVal& s) const {
        return getProdValue() < s.getProdValue();
    }
    inline bool operator>(const ScalarNmbrVal& s) const {
        return getProdValue() > s.getProdValue();
    }
    inline bool operator<=(const ScalarNmbrVal& s) const {
        return getProdValue() <= s.getProdValue();
    }
    inline bool operator>=(const ScalarNmbrVal& s) const {
        return getProdValue() >= s.getProdValue();
    }
    inline operator ScalarNmbrVal() {
        return {
            .primValue = getPrimValue() ,
            .unitFactor = getUnitFactor()
        };
    }
protected:
    inline T_value getPrimValue() const {
        return T_accessor::getPrimaryValue( this->m_array, this->m_index );
    }
    inline void setPrimValue(T_value value) {
        T_accessor::setPrimaryValue( this->m_array, this->m_index, value );
    }
    inline vmfloat getUnitFactor() const {
        return this->m_array->unitFactorOfElement(this->m_index);
    }
    inline void setUnitFactor(vmfloat factor) {
        this->m_array->assignElementUnitFactor(this->m_index, factor);
    }
    inline vmfloat getProdValue() const {
        // simple solution for both vmint and vmfloat, should be fine for just sorting
        vmfloat primary = (vmfloat) getPrimValue();
        vmfloat factor  = getUnitFactor();
        return primary * factor;
    }

    // allow swap() functions below to access protected methods here
    friend void swap(class ArrElemRef<T_array,T_value,T_accessor> a,
                     class ArrElemRef<T_array,T_value,T_accessor> b);
};

// custom iterator class to be used by std:sort() on our VM arrays
template<class T_array, class T_value, class T_accessor>
class ArrExprIter : public ArrElemPOD<T_array> {
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef ssize_t difference_type;
    typedef ::LinuxSampler::ArrElemRef<T_array, T_value, T_accessor> ArrElemRef; // GCC 8.x requires this very detailed form of typedef (that is ::LinuxSampler:: as prefix), IMO a GCC bug
    typedef ArrElemRef reference; // type used by STL for access by reference
    typedef void pointer; // type used by STL for -> operator result, we don't use that operator at all so just void it
    typedef ScalarNmbrVal<T_value> value_type; // type used by STL for temporary values

    ArrExprIter(T_array* a, vmint index) {
        this->m_array = a;
        this->m_index = index;
    }
    ArrExprIter(const ArrElemRef& ref) {
        this->m_array = ref.m_array;
        this->m_index = ref.m_index;
    }
    inline ArrElemRef operator*() {
        return ArrElemRef(this->m_array, this->m_index);
    }
    inline ArrExprIter& operator++() { // prefix increment
        ++(this->m_index);
        return *this;
    }
    inline ArrExprIter& operator--() { // prefix decrement
        --(this->m_index);
        return *this;
    }
    inline ArrExprIter operator++(int) { // postfix increment
        ArrExprIter it = *this;
        ++(this->m_index);
        return it;
    }
    inline ArrExprIter operator--(int) { // postfix decrement
        ArrExprIter it = *this;
        --(this->m_index);
        return it;
    }
    inline ArrExprIter& operator+=(difference_type d) {
        this->m_index += d;
        return *this;
    }
    inline ArrExprIter& operator-=(difference_type d) {
        this->m_index -= d;
        return *this;
    }
    inline bool operator==(const ArrExprIter& other) const {
        return this->m_index == other.m_index;
    }
    inline bool operator!=(const ArrExprIter& other) const {
        return this->m_index != other.m_index;
    }
    inline bool operator<(const ArrExprIter& other) const {
        return this->m_index < other.m_index;
    }
    inline bool operator>(const ArrExprIter& other) const {
        return this->m_index > other.m_index;
    }
    inline bool operator<=(const ArrExprIter& other) const {
        return this->m_index <= other.m_index;
    }
    inline bool operator>=(const ArrExprIter& other) const {
        return this->m_index >= other.m_index;
    }
    inline difference_type operator+(const ArrExprIter& other) const {
        return this->m_index + other.m_index;
    }
    inline difference_type operator-(const ArrExprIter& other) const {
        return this->m_index - other.m_index;
    }
    inline ArrExprIter operator-(difference_type d) const {
        return ArrExprIter(this->m_array, this->m_index - d);
    }
    inline ArrExprIter operator+(difference_type d) const {
        return ArrExprIter(this->m_array, this->m_index + d);
    }
    inline ArrExprIter operator*(difference_type factor) const {
        return ArrExprIter(this->m_array, this->m_index * factor);
    }
};

typedef ArrExprIter<VMIntArrayExpr,vmint,IntArrayAccessor> IntArrExprIter;
typedef ArrExprIter<VMRealArrayExpr,vmfloat,RealArrayAccessor> RealArrExprIter;

// intentionally not a template function to avoid potential clashes with other (i.e. system's) swap() functions
inline void swap(IntArrExprIter::ArrElemRef a,
                 IntArrExprIter::ArrElemRef b)
{
    vmint valueA = a.getPrimValue();
    vmint valueB = b.getPrimValue();
    vmfloat factorA = a.getUnitFactor();
    vmfloat factorB = b.getUnitFactor();
    a.setPrimValue(valueB);
    a.setUnitFactor(factorB);
    b.setPrimValue(valueA);
    b.setUnitFactor(factorA);
}

// intentionally not a template function to avoid potential clashes with other (i.e. system's) swap() functions
inline void swap(RealArrExprIter::ArrElemRef a,
                 RealArrExprIter::ArrElemRef b)
{
    vmfloat valueA = a.getPrimValue();
    vmfloat valueB = b.getPrimValue();
    vmfloat factorA = a.getUnitFactor();
    vmfloat factorB = b.getUnitFactor();
    a.setPrimValue(valueB);
    a.setUnitFactor(factorB);
    b.setPrimValue(valueA);
    b.setUnitFactor(factorA);
}

// used to sort in descending order (unlike the default behaviour of std::sort() which is ascending order)
template<class T> // T is either IntArrExprIter or RealArrExprIter
struct DescArrExprSorter {
    inline bool operator()(const typename T::value_type a, const typename T::value_type b) const {
        return a > b;
    }
};

VMFnResult* CoreVMFunction_sort::exec(VMFnArgs* args) {
    const bool bAscending =
        (args->argsCount() < 2) ? true : !args->arg(1)->asInt()->evalInt();

    if (args->arg(0)->exprType() == INT_ARR_EXPR) {
        VMIntArrayExpr* a = args->arg(0)->asIntArray();
        vmint n = a->arraySize();
        IntArrExprIter itBegin(a, 0);
        IntArrExprIter itEnd(a, n);
        if (bAscending) {
            std::sort(itBegin, itEnd);
        } else {
            DescArrExprSorter<IntArrExprIter> sorter;
            std::sort(itBegin, itEnd, sorter);
        }
    } else {
        VMRealArrayExpr* a = args->arg(0)->asRealArray();
        vmint n = a->arraySize();
        RealArrExprIter itBegin(a, 0);
        RealArrExprIter itEnd(a, n);
        if (bAscending) {
            std::sort(itBegin, itEnd);
        } else {
            DescArrExprSorter<RealArrExprIter> sorter;
            std::sort(itBegin, itEnd, sorter);
        }
    }

    return successResult();
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  real_to_int()  and  int()

StdUnit_t CoreVMFunction_real_to_int::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_real_to_int::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_real_to_int::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    return successResult({
        .value = vmint(f),
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  int_to_real()  and  real()

StdUnit_t CoreVMFunction_int_to_real::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_int_to_real::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_int_to_real::exec(VMFnArgs* args) {
    VMIntExpr* intExpr = args->arg(0)->asInt();
    vmint i = intExpr->evalInt();
    return successResult({
        .value = vmfloat(i),
        .unitFactor = intExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  round()

StdUnit_t CoreVMFunction_round::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_round::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_round::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::roundf(f);
    else
        f = ::round(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  ceil()

StdUnit_t CoreVMFunction_ceil::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_ceil::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_ceil::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::ceilf(f);
    else
        f = ::ceil(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  floor()

StdUnit_t CoreVMFunction_floor::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_floor::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_floor::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::floorf(f);
    else
        f = ::floor(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sqrt()

StdUnit_t CoreVMFunction_sqrt::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_sqrt::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_sqrt::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::sqrtf(f);
    else
        f = ::sqrt(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  log()

StdUnit_t CoreVMFunction_log::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_log::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_log::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::logf(f);
    else
        f = ::log(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  log2()

StdUnit_t CoreVMFunction_log2::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_log2::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_log2::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::log2f(f);
    else
        f = ::log2(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  log10()

StdUnit_t CoreVMFunction_log10::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_log10::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_log10::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::log10f(f);
    else
        f = ::log10(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  exp()

StdUnit_t CoreVMFunction_exp::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_exp::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_exp::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::expf(f);
    else
        f = ::exp(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  pow()

bool CoreVMFunction_pow::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
    if (iArg == 0)
        return true;
    else
        return type == VM_NO_UNIT;
}

bool CoreVMFunction_pow::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
    return iArg == 0;
}

StdUnit_t CoreVMFunction_pow::returnUnitType(VMFnArgs* args) {
    // pow() only allows unit for its 1st argument
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_pow::returnsFinal(VMFnArgs* args) {
    // pow() only allows 'final'ness for its 1st argument
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_pow::exec(VMFnArgs* args) {
    VMRealExpr* arg0 = args->arg(0)->asReal();
    VMRealExpr* arg1 = args->arg(1)->asReal();
    vmfloat a = arg0->evalReal();
    vmfloat b = arg1->evalReal();
    if (sizeof(vmfloat) == sizeof(float)) {
        return successResult({
            .value = ::powf(a,b),
            .unitFactor = arg0->unitFactor()
        });
    } else {
        return successResult({
            .value = static_cast<vmfloat>(::pow(a,b)),
            .unitFactor = arg0->unitFactor()
        });
    }
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sin()

StdUnit_t CoreVMFunction_sin::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_sin::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_sin::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::sinf(f);
    else
        f = ::sin(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  cos()

StdUnit_t CoreVMFunction_cos::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_cos::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_cos::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::cosf(f);
    else
        f = ::cos(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  tan()

StdUnit_t CoreVMFunction_tan::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_tan::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_tan::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::tanf(f);
    else
        f = ::tan(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  asin()

StdUnit_t CoreVMFunction_asin::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_asin::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_asin::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::asinf(f);
    else
        f = ::asin(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  acos()

StdUnit_t CoreVMFunction_acos::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_acos::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_acos::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::acosf(f);
    else
        f = ::acos(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  atan()

StdUnit_t CoreVMFunction_atan::returnUnitType(VMFnArgs* args) {
    return args->arg(0)->asNumber()->unitType();
}

bool CoreVMFunction_atan::returnsFinal(VMFnArgs* args) {
    return args->arg(0)->asNumber()->isFinal();
}

VMFnResult* CoreVMFunction_atan::exec(VMFnArgs* args) {
    VMRealExpr* realExpr = args->arg(0)->asReal();
    vmfloat f = realExpr->evalReal();
    if (sizeof(vmfloat) == sizeof(float))
        f = ::atanf(f);
    else
        f = ::atan(f);
    return successResult({
        .value = f,
        .unitFactor = realExpr->unitFactor()
    });
}

} // namespace LinuxSampler
