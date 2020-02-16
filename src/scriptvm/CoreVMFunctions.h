/*
 * Copyright (c) 2014-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_COREVMFUNCTIONS_H
#define LS_COREVMFUNCTIONS_H

#include "../common/global.h"
#include "common.h"

namespace LinuxSampler {
    
class ScriptVM;

///////////////////////////////////////////////////////////////////////////
// convenience base classes for built-in script functions ...

/**
 * An instance of this class is returned by built-in function implementations
 * which do not return a function return value.
 */
class VMEmptyResult FINAL : public VMFnResult, public VMExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call

    VMEmptyResult() : flags(STMT_SUCCESS) {}
    ExprType_t exprType() const OVERRIDE { return EMPTY_EXPR; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
};

/**
 * An instance of this class is returned by built-in function implementations
 * which return an integer value as function return value.
 */
class VMIntResult FINAL : public VMFnResult, public VMIntExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call
    vmint value; ///< result value of the function call
    vmfloat unitPrefixFactor; ///< unit factor of result value of the function call
    StdUnit_t unitBaseType; ///< fundamental standard measuring unit type (e.g. seconds), this is ALWAYS and ONLY set (to the actual unit type) by @c FunctionCall class for performance reasons.

    VMIntResult();
    vmint evalInt() OVERRIDE { return value; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
    vmfloat unitFactor() const OVERRIDE { return unitPrefixFactor; }
    StdUnit_t unitType() const OVERRIDE { return unitBaseType; }
    bool isFinal() const OVERRIDE { return false; } // actually never called, VMFunction::returnsFinal() is always used instead
};

/**
 * An instance of this class is returned by built-in function implementations
 * which return a real number (floating point) value as function return value.
 */
class VMRealResult FINAL : public VMFnResult, public VMRealExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call
    vmfloat value; ///< result value of the function call
    vmfloat unitPrefixFactor; ///< unit factor of result value of the function call
    StdUnit_t unitBaseType; ///< fundamental standard measuring unit type (e.g. seconds), this is ALWAYS and ONLY set (to the actual unit type) by @c FunctionCall class for performance reasons.

    VMRealResult();
    vmfloat evalReal() OVERRIDE { return value; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
    vmfloat unitFactor() const OVERRIDE { return unitPrefixFactor; }
    StdUnit_t unitType() const OVERRIDE { return unitBaseType; }
    bool isFinal() const OVERRIDE { return false; } // actually never called, VMFunction::returnsFinal() is always used instead
};

/**
 * An instance of this class is returned by built-in function implementations
 * which return a string value as function return value.
 */
class VMStringResult FINAL : public VMFnResult, public VMStringExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call
    String value; ///< result value of the function call

    VMStringResult() : flags(STMT_SUCCESS) {}
    String evalStr() OVERRIDE { return value; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
};

/**
 * Abstract base class for built-in script functions which do not return any
 * function return value (void).
 */
class VMEmptyResultFunction : public VMFunction {
protected:
    VMEmptyResultFunction() : result(NULL) {}
    virtual ~VMEmptyResultFunction() {}
    ExprType_t returnType(VMFnArgs* args) OVERRIDE { return EMPTY_EXPR; }
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    VMFnResult* errorResult();
    VMFnResult* successResult();
    bool modifiesArg(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* allocResult(VMFnArgs* args) OVERRIDE { return new VMEmptyResult(); }
    void bindResult(VMFnResult* res) OVERRIDE;
    VMFnResult* boundResult() const OVERRIDE;
protected:
    VMEmptyResult* result;
};

struct VMIntFnResDef {
    vmint value = 0; //NOTE: sequence matters! Since this is usually initialized with VMIntExpr::evalInt() it should be before member unitFactor, since the latter is usually initialized with VMIntExpr::unitFactor() which does not evaluate the expression.
    vmfloat unitFactor = VM_NO_FACTOR;
};

struct VMRealFnResDef {
    vmfloat value = vmfloat(0); //NOTE: sequence matters! Since this is usually initialized with VMRealExpr::evalReal() it should be before member unitFactor, since the latter is usually initialized with VMRealExpr::unitFactor() which does not evaluate the expression.
    vmfloat unitFactor = VM_NO_FACTOR;
};

/**
 * Abstract base class for built-in script functions which return an integer
 * (scalar) as their function return value.
 */
class VMIntResultFunction : public VMFunction {
protected:
    VMIntResultFunction() : result(NULL) {}
    virtual ~VMIntResultFunction() {}
    ExprType_t returnType(VMFnArgs* args) OVERRIDE { return INT_EXPR; }
    VMFnResult* errorResult(vmint i = 0);
    VMFnResult* errorResult(VMIntFnResDef res);
    VMFnResult* successResult(vmint i = 0);
    VMFnResult* successResult(VMIntFnResDef res);
    bool modifiesArg(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* allocResult(VMFnArgs* args) OVERRIDE { return new VMIntResult(); }
    void bindResult(VMFnResult* res) OVERRIDE;
    VMFnResult* boundResult() const OVERRIDE;
protected:
    VMIntResult* result;
};

/**
 * Abstract base class for built-in script functions which return a real number
 * (floating point scalar) as their function return value.
 */
class VMRealResultFunction : public VMFunction {
protected:
    VMRealResultFunction() : result(NULL) {}
    virtual ~VMRealResultFunction() {}
    ExprType_t returnType(VMFnArgs* args) OVERRIDE { return REAL_EXPR; }
    VMFnResult* errorResult(vmfloat f = 0);
    VMFnResult* errorResult(VMRealFnResDef res);
    VMFnResult* successResult(vmfloat f = 0);
    VMFnResult* successResult(VMRealFnResDef res);
    bool modifiesArg(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* allocResult(VMFnArgs* args) OVERRIDE { return new VMRealResult(); }
    void bindResult(VMFnResult* res) OVERRIDE;
    VMFnResult* boundResult() const OVERRIDE;
protected:
    VMRealResult* result;
};

/**
 * Abstract base class for built-in script functions which return a string as
 * their function return value.
 */
class VMStringResultFunction : public VMFunction {
protected:
    VMStringResultFunction() : result(NULL) {}
    virtual ~VMStringResultFunction() {}
    ExprType_t returnType(VMFnArgs* args) OVERRIDE { return STRING_EXPR; }
    VMFnResult* errorResult(const String& s = "");
    VMFnResult* successResult(const String& s = "");
    bool modifiesArg(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* allocResult(VMFnArgs* args) OVERRIDE { return new VMStringResult(); }
    void bindResult(VMFnResult* res) OVERRIDE;
    VMFnResult* boundResult() const OVERRIDE;
protected:
    VMStringResult* result;
};

/**
 * Abstract base class for built-in script functions which either return an
 * integer or a real number (floating point scalar) as their function return
 * value. The actual return type is determined at parse time once after
 * potential arguments were associated with the respective function call.
 */
class VMNumberResultFunction : public VMFunction {
protected:
    VMNumberResultFunction() : intResult(NULL), realResult(NULL) {}
    virtual ~VMNumberResultFunction() {}
    VMFnResult* errorResult(vmint i);
    VMFnResult* errorResult(vmfloat f);
    VMFnResult* successResult(vmint i);
    VMFnResult* successResult(vmfloat f);
    VMFnResult* errorIntResult(VMIntFnResDef res);
    VMFnResult* errorRealResult(VMRealFnResDef res);
    VMFnResult* successIntResult(VMIntFnResDef res);
    VMFnResult* successRealResult(VMRealFnResDef res);
    bool modifiesArg(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* allocResult(VMFnArgs* args) OVERRIDE;
    void bindResult(VMFnResult* res) OVERRIDE;
    VMFnResult* boundResult() const OVERRIDE;
protected:
    VMIntResult* intResult;
    VMRealResult* realResult;
};


///////////////////////////////////////////////////////////////////////////
// implementations of core built-in script functions ...

/**
 * Implements the built-in message() script function.
 */
class CoreVMFunction_message FINAL : public VMEmptyResultFunction {
public:
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in exit() script function.
 */
class CoreVMFunction_exit FINAL : public VMEmptyResultFunction {
public:
    CoreVMFunction_exit(ScriptVM* vm) : vm(vm) {}
    vmint minRequiredArgs() const OVERRIDE { return 0; }
    vmint maxAllowedArgs() const OVERRIDE;
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
    bool acceptsArgFinal(vmint iArg) const OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
protected:
    ScriptVM* vm;
};

/**
 * Implements the built-in wait() script function.
 */
class CoreVMFunction_wait : public VMEmptyResultFunction {
public:
    CoreVMFunction_wait(ScriptVM* vm) : vm(vm) {}
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
protected:
    ScriptVM* vm;
};

/**
 * Implements the built-in abs() script function.
 */
class CoreVMFunction_abs FINAL : public VMNumberResultFunction {
public:
    ExprType_t returnType(VMFnArgs* args) OVERRIDE;
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in random() script function.
 */
class CoreVMFunction_random FINAL : public VMNumberResultFunction {
    using Super = VMNumberResultFunction; // just an alias for the super class
public:
    ExprType_t returnType(VMFnArgs* args) OVERRIDE;
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in num_elements() script function.
 */
class CoreVMFunction_num_elements FINAL : public VMIntResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in inc() script function.
 */
class CoreVMFunction_inc FINAL : public VMIntResultFunction {
    using Super = VMIntResultFunction; // just an alias for the super class
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    bool modifiesArg(vmint iArg) const OVERRIDE { return true; }
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in dec() script function.
 */
class CoreVMFunction_dec FINAL : public VMIntResultFunction {
    using Super = VMIntResultFunction; // just an alias for the super class
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    bool modifiesArg(vmint iArg) const OVERRIDE { return true; }
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in in_range() script function.
 */
class CoreVMFunction_in_range FINAL : public VMIntResultFunction {
    using Super = VMIntResultFunction; // just an alias for the super class
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    vmint minRequiredArgs() const OVERRIDE { return 3; }
    vmint maxAllowedArgs() const OVERRIDE { return 3; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in sh_left() script function.
 */
class CoreVMFunction_sh_left FINAL : public VMIntResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in sh_right() script function.
 */
class CoreVMFunction_sh_right FINAL : public VMIntResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in msb() script function.
 */
class CoreVMFunction_msb FINAL : public VMIntResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in lsb() script function.
 */
class CoreVMFunction_lsb FINAL : public VMIntResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return false; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in min() script function.
 */
class CoreVMFunction_min FINAL : public VMNumberResultFunction {
    using Super = VMNumberResultFunction; // just an alias for the super class
public:
    ExprType_t returnType(VMFnArgs* args) OVERRIDE;
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in max() script function.
 */
class CoreVMFunction_max FINAL : public VMNumberResultFunction {
    using Super = VMNumberResultFunction; // just an alias for the super class
public:
    ExprType_t returnType(VMFnArgs* args) OVERRIDE;
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in array_equal() script function.
 */
class CoreVMFunction_array_equal FINAL : public VMIntResultFunction {
    using Super = VMIntResultFunction; // just an alias for the super class
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in search() script function.
 */
class CoreVMFunction_search FINAL : public VMIntResultFunction {
    using Super = VMIntResultFunction; // just an alias for the super class
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
    bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                                   std::function<void(String)> wrn) OVERRIDE;
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in sort() script function.
 */
class CoreVMFunction_sort FINAL : public VMEmptyResultFunction {
public:
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
    bool modifiesArg(vmint iArg) const OVERRIDE { return iArg == 0; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in real_to_int() script function and its short hand
 * variant int(). The behaviour of the two built-in script functions are
 * identical ATM.
 */
class CoreVMFunction_real_to_int FINAL : public VMIntResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in int_to_real() script function and its short hand
 * variant real(). The behaviour of the two built-in script functions are
 * identical ATM.
 */
class CoreVMFunction_int_to_real FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in round() script function.
 */
class CoreVMFunction_round FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in ceil() script function.
 */
class CoreVMFunction_ceil FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in floor() script function.
 */
class CoreVMFunction_floor FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in sqrt() script function.
 */
class CoreVMFunction_sqrt FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in log() script function.
 */
class CoreVMFunction_log FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in log2() script function.
 */
class CoreVMFunction_log2 FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in log10() script function.
 */
class CoreVMFunction_log10 FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in exp() script function.
 */
class CoreVMFunction_exp FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in pow() script function.
 */
class CoreVMFunction_pow FINAL : public VMRealResultFunction {
    using Super = VMRealResultFunction; // just an alias for the super class
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 2; }
    vmint maxAllowedArgs() const OVERRIDE { return 2; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return iArg == 0; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in sin() script function.
 */
class CoreVMFunction_sin FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in cos() script function.
 */
class CoreVMFunction_cos FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in tan() script function.
 */
class CoreVMFunction_tan FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in asin() script function.
 */
class CoreVMFunction_asin FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in acos() script function.
 */
class CoreVMFunction_acos FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

/**
 * Implements the built-in atan() script function.
 */
class CoreVMFunction_atan FINAL : public VMRealResultFunction {
public:
    StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE;
    bool returnsFinal(VMFnArgs* args) OVERRIDE;
    vmint minRequiredArgs() const OVERRIDE { return 1; }
    vmint maxAllowedArgs() const OVERRIDE { return 1; }
    bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == REAL_EXPR; }
    bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE { return true; }
    bool acceptsArgFinal(vmint iArg) const OVERRIDE { return true; }
    VMFnResult* exec(VMFnArgs* args) OVERRIDE;
};

} // namespace LinuxSampler

#endif // LS_COREVMFUNCTIONS_H
