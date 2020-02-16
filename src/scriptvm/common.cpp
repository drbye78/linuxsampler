/*
 * Copyright (c) 2014-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "common.h"
#include <iostream>
#include "editor/SourceToken.h"

namespace LinuxSampler {

    ///////////////////////////////////////////////////////////////////////
    // class 'VMUnit'

    static vmfloat _unitFactor(MetricPrefix_t prefix) {
        switch (prefix) {
            case VM_NO_PREFIX:  return 1.f;
            case VM_KILO:       return 1000.f;
            case VM_HECTO:      return 100.f;
            case VM_DECA:       return 10.f;
            case VM_DECI:       return 0.1f;
            case VM_CENTI:      return 0.01f;
            case VM_MILLI:      return 0.001f;
            case VM_MICRO:      return 0.000001f;
        }
        return 1.f;
    }

    vmfloat VMUnit::unitFactor(MetricPrefix_t prefix) {
        return _unitFactor(prefix);
    }

    vmfloat VMUnit::unitFactor(MetricPrefix_t prefix1, MetricPrefix_t prefix2) {
        return _unitFactor(prefix1) * _unitFactor(prefix2);
    }

    vmfloat VMUnit::unitFactor(const MetricPrefix_t* prefixes, vmuint size) {
        vmfloat f = VM_NO_FACTOR;
        for (vmuint i = 0; i < size && prefixes[i]; ++i)
            f *= _unitFactor(prefixes[i]);
        return f;
    }

    bool VMUnit::hasUnitFactorNow() const {
        return unitFactor() != VM_NO_FACTOR;
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'VMExpr'

    VMIntExpr* VMExpr::asInt() const {
        return const_cast<VMIntExpr*>( dynamic_cast<const VMIntExpr*>(this) );
    }

    VMRealExpr* VMExpr::asReal() const {
        return const_cast<VMRealExpr*>( dynamic_cast<const VMRealExpr*>(this) );
    }

    VMNumberExpr* VMExpr::asNumber() const {
        return const_cast<VMNumberExpr*>(
            dynamic_cast<const VMNumberExpr*>(this)
        );
    }

    VMStringExpr* VMExpr::asString() const {
        return const_cast<VMStringExpr*>( dynamic_cast<const VMStringExpr*>(this) );
    }

    VMIntArrayExpr* VMExpr::asIntArray() const {
        return const_cast<VMIntArrayExpr*>( dynamic_cast<const VMIntArrayExpr*>(this) );
    }

    VMRealArrayExpr* VMExpr::asRealArray() const {
        return const_cast<VMRealArrayExpr*>(
            dynamic_cast<const VMRealArrayExpr*>(this)
        );
    }

    VMArrayExpr* VMExpr::asArray() const {
        return const_cast<VMArrayExpr*>(
            dynamic_cast<const VMArrayExpr*>(this)
        );
    }

    bool VMExpr::isModifyable() const {
        const VMVariable* var = dynamic_cast<const VMVariable*>(this);
        return (!var) ? false : var->isAssignable();
    }

    bool VMFunction::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        return type == VM_NO_UNIT;
    }

    bool VMFunction::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return false;
    }

    bool VMFunction::acceptsArgFinal(vmint iArg) const {
        return false;
    }

    void VMFunction::checkArgs(VMFnArgs* args,
                               std::function<void(String)> err,
                               std::function<void(String)> wrn)
    {
    }

    void VMFunction::wrnMsg(const String& txt) {
        std::cout << "[ScriptVM] " << txt << std::endl;
    }

    void VMFunction::errMsg(const String& txt) {
        std::cerr << "[ScriptVM] " << txt << std::endl;
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'VMNumberExpr'

    vmint VMNumberExpr::evalCastInt() {
        if (exprType() == INT_EXPR)
            return asInt()->evalInt();
        else
            return vmint( asReal()->evalReal() );
    }

    vmfloat VMNumberExpr::evalCastReal() {
        if (exprType() == REAL_EXPR)
            return asReal()->evalReal();
        else
            return vmfloat( asInt()->evalInt() );
    }

    vmint VMNumberExpr::evalCastInt(MetricPrefix_t prefix) {
        vmfloat f = evalCastReal();
        vmfloat factor = unitFactor() / _unitFactor(prefix);
        return vmint(f * factor);
    }

    vmint VMNumberExpr::evalCastInt(MetricPrefix_t prefix1, MetricPrefix_t prefix2) {
        vmfloat f = evalCastReal();
        vmfloat factor = unitFactor() /
                            ( _unitFactor(prefix1) * _unitFactor(prefix2) );
        return vmint(f * factor);
    }

    vmfloat VMNumberExpr::evalCastReal(MetricPrefix_t prefix) {
        vmfloat f = evalCastReal();
        vmfloat factor = unitFactor() / _unitFactor(prefix);
        return f * factor;
    }

    vmfloat VMNumberExpr::evalCastReal(MetricPrefix_t prefix1, MetricPrefix_t prefix2) {
        vmfloat f = evalCastReal();
        vmfloat factor = unitFactor() /
                            ( _unitFactor(prefix1) * _unitFactor(prefix2) );
        return f * factor;
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'VMIntExpr'

    vmint VMIntExpr::evalInt(MetricPrefix_t prefix) {
        vmfloat f = (vmfloat) evalInt();
        vmfloat factor = unitFactor() / _unitFactor(prefix);
        return vmint(f * factor);
    }

    vmint VMIntExpr::evalInt(MetricPrefix_t prefix1, MetricPrefix_t prefix2) {
        vmfloat f = (vmfloat) evalInt();
        vmfloat factor = unitFactor() /
                            ( _unitFactor(prefix1) * _unitFactor(prefix2) );
        return vmint(f * factor);
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'VMRealExpr'

    vmfloat VMRealExpr::evalReal(MetricPrefix_t prefix) {
        vmfloat f = evalReal();
        vmfloat factor = unitFactor() / _unitFactor(prefix);
        return f * factor;
    }

    vmfloat VMRealExpr::evalReal(MetricPrefix_t prefix1, MetricPrefix_t prefix2) {
        vmfloat f = evalReal();
        vmfloat factor = unitFactor() /
                            ( _unitFactor(prefix1) * _unitFactor(prefix2) );
        return f * factor;
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'VMFnResult'

    VMFnResult::~VMFnResult() {
    }

    ///////////////////////////////////////////////////////////////////////
    // class 'VMSourceToken'

    VMSourceToken::VMSourceToken() : m_token(NULL) {
    }

    VMSourceToken::VMSourceToken(SourceToken* ct) : m_token(ct) {
    }

    VMSourceToken::VMSourceToken(const VMSourceToken& other) {
        if (other.m_token) {
            m_token = new SourceToken;
            *m_token = *other.m_token;
        } else m_token = NULL;
    }

    VMSourceToken::~VMSourceToken() {
        if (m_token) {
            delete m_token;
            m_token = NULL;
        }
    }

    VMSourceToken& VMSourceToken::operator=(const VMSourceToken& other) {
        if (m_token) delete m_token;
        if (other.m_token) {
            m_token = new SourceToken;
            *m_token = *other.m_token;
        } else m_token = NULL;
        return *this;
    }

    String VMSourceToken::text() const {
        return (m_token) ? m_token->text() : "";
    }

    int VMSourceToken::firstLine() const {
        return (m_token) ? m_token->firstLine() : 0;
    }

    int VMSourceToken::firstColumn() const {
        return (m_token) ? m_token->firstColumn() : 0;
    }

    int VMSourceToken::firstByte() const {
        return (m_token) ? m_token->firstByte() : 0;
    }

    int VMSourceToken::lengthBytes() const {
        return (m_token) ? m_token->lengthBytes() : 0;
    }

    bool VMSourceToken::isEOF() const {
        return (m_token) ? m_token->isEOF() : true;
    }

    bool VMSourceToken::isNewLine() const {
        return (m_token) ? m_token->isNewLine() : false;
    }

    bool VMSourceToken::isKeyword() const {
        return (m_token) ? m_token->isKeyword() : false;
    }

    bool VMSourceToken::isVariableName() const {
        return (m_token) ? m_token->isVariableName() : false;
    }

    bool VMSourceToken::isIdentifier() const {
        return (m_token) ? m_token->isIdentifier() : false;
    }

    bool VMSourceToken::isNumberLiteral() const {
        return (m_token) ? m_token->isNumberLiteral() : false;
    }

    bool VMSourceToken::isStringLiteral() const {
        return (m_token) ? m_token->isStringLiteral() : false;
    }

    bool VMSourceToken::isComment() const {
        return (m_token) ? m_token->isComment() : false;
    }

    bool VMSourceToken::isPreprocessor() const {
        return (m_token) ? m_token->isPreprocessor() : false;
    }

    bool VMSourceToken::isMetricPrefix() const {
        return (m_token) ? m_token->isMetricPrefix() : false;
    }

    bool VMSourceToken::isStdUnit() const {
        return (m_token) ? m_token->isStdUnit() : false;
    }

    bool VMSourceToken::isOther() const {
        return (m_token) ? m_token->isOther() : true;
    }

    bool VMSourceToken::isIntegerVariable() const {
        return (m_token) ? m_token->isIntegerVariable() : false;
    }

    bool VMSourceToken::isRealVariable() const {
        return (m_token) ? m_token->isRealVariable() : false;
    }

    bool VMSourceToken::isStringVariable() const {
        return (m_token) ? m_token->isStringVariable() : false;
    }

    bool VMSourceToken::isIntArrayVariable() const {
        return (m_token) ? m_token->isIntegerArrayVariable() : false;
    }

    bool VMSourceToken::isRealArrayVariable() const {
        return (m_token) ? m_token->isRealArrayVariable() : false;
    }

    bool VMSourceToken::isArrayVariable() const { // deprecated API: will be removed !
        return isIntArrayVariable();
    }

    bool VMSourceToken::isEventHandlerName() const {
        return (m_token) ? m_token->isEventHandlerName() : false;
    }

} // namespace LinuxSampler
