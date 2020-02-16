/*
 * Copyright (c) 2014 - 2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_INSTRSCRIPTVMFUNCTIONS_H
#define LS_INSTRSCRIPTVMFUNCTIONS_H

#include "../../common/global.h"
#include "../../scriptvm/CoreVMFunctions.h"
#include "Note.h"

namespace LinuxSampler {

    class EventGroup;
    class InstrumentScriptVM;

    class InstrumentScriptVMFunction_play_note FINAL : public VMIntResultFunction {
        using Super = VMIntResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_play_note(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 4; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_controller FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_set_controller(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_rpn FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_set_rpn(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_nrpn FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_set_nrpn(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_ignore_event FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_ignore_event(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 0; }
        vmint maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_ignore_controller FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_ignore_controller(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 0; }
        vmint maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_note_off FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_note_off(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_event_mark FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_set_event_mark(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_delete_event_mark FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_delete_event_mark(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_by_marks FINAL : public VMFunction {
        using Super = VMFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_by_marks(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        bool modifiesArg(vmint iArg) const OVERRIDE { return false; }
        ExprType_t returnType(VMFnArgs* args) OVERRIDE { return INT_ARR_EXPR; }
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* allocResult(VMFnArgs* args) OVERRIDE;
        void bindResult(VMFnResult* res) OVERRIDE;
        VMFnResult* boundResult() const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
        class Result : public VMFnResult, public VMIntArrayExpr {
        public:
            StmtFlags_t flags;
            EventGroup* eventGroup;

            vmint arraySize() const OVERRIDE;
            vmint evalIntElement(vmuint i) OVERRIDE;
            void assignIntElement(vmuint i, vmint value) OVERRIDE {} // ignore assignment
            vmfloat unitFactorOfElement(vmuint i) const OVERRIDE { return VM_NO_FACTOR; }
            void assignElementUnitFactor(vmuint i, vmfloat factor) OVERRIDE {} // ignore assignment
            VMExpr* resultValue() OVERRIDE { return this; }
            StmtFlags_t resultFlags() OVERRIDE { return flags; }
            bool isConstExpr() const OVERRIDE { return false; }
        } *m_result;

        VMFnResult* errorResult();
        VMFnResult* successResult(EventGroup* eventGroup);
    };

    class InstrumentScriptVMFunction_change_vol FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_vol(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 3; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_tune FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_tune(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 3; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_pan FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_pan(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 3; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_cutoff FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_change_cutoff(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_reso FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_reso(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    //TODO: Derive from generalized, shared template class VMChangeSynthParamFunction instead (like e.g. change_cutoff_attack() implementation below already does).
    class InstrumentScriptVMFunction_change_attack FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_change_attack(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    //TODO: Derive from generalized, shared template class VMChangeSynthParamFunction instead (like e.g. change_cutoff_decay() implementation below already does).
    class InstrumentScriptVMFunction_change_decay FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_change_decay(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    //TODO: Derive from generalized, shared template class VMChangeSynthParamFunction instead (like e.g. change_cutoff_release() implementation below already does).
    class InstrumentScriptVMFunction_change_release FINAL : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        InstrumentScriptVMFunction_change_release(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class VMChangeSynthParamFunction : public VMEmptyResultFunction {
        using Super = VMEmptyResultFunction; // just an alias for the super class
    public:
        struct Opt_t {
            InstrumentScriptVM* vm = NULL; ///< Parent object owning the built-in function implementation object.
            bool acceptFinal = false; ///< Whether built-in function allows 'final' operator for its 2nd function argument.
            StdUnit_t unit = VM_NO_UNIT; ///< Whether built-in functions accepts a unit type for its 2nd function argument and if yes which one.
            bool acceptUnitPrefix = false; ///< Whether built-in function accepts metric unit prefix(es) for its 2nd function argument.
            bool acceptReal = false; ///< Whether the built-in function accepts both int and real number as data type for its 2nd function argument (otherwise its int only if false).
        };
        VMChangeSynthParamFunction(const Opt_t& opt)
            : m_vm(opt.vm), m_acceptReal(opt.acceptReal),
              m_acceptFinal(opt.acceptFinal),
              m_acceptUnitPrefix(opt.acceptUnitPrefix), m_unit(opt.unit) {}
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgFinal(vmint iArg) const OVERRIDE;
        void checkArgs(VMFnArgs* args, std::function<void(String)> err,
                       std::function<void(String)> wrn) OVERRIDE;

        template<class T_NoteParamType, T_NoteParamType NoteBase::_Override::*T_noteParam,
                vmint T_synthParam,
                vmint T_minValueNorm, vmint T_maxValueNorm, bool T_normalizeNorm,
                vmint T_minValueUnit, vmint T_maxValueUnit,
                MetricPrefix_t T_unitPrefix0, MetricPrefix_t ... T_unitPrefixN>
        VMFnResult* execTemplate(VMFnArgs* args, const char* functionName);
    protected:
        InstrumentScriptVM* const m_vm;
        const bool m_acceptReal;
        const bool m_acceptFinal;
        const bool m_acceptUnitPrefix;
        const StdUnit_t m_unit;
    };

    class InstrumentScriptVMFunction_change_sustain FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_sustain(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_BEL,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_cutoff_attack FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_attack(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_SECOND,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_cutoff_decay FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_decay(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_SECOND,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_cutoff_sustain FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_sustain(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_BEL,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_cutoff_release FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_release(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_SECOND,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_amp_lfo_depth FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_amp_lfo_depth(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_NO_UNIT,
            .acceptUnitPrefix = false,
            .acceptReal = false,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_amp_lfo_freq FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_amp_lfo_freq(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_HERTZ,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_cutoff_lfo_depth FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_lfo_depth(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_NO_UNIT,
            .acceptUnitPrefix = false,
            .acceptReal = false,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_cutoff_lfo_freq FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_lfo_freq(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_HERTZ,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_pitch_lfo_depth FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_pitch_lfo_depth(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_NO_UNIT,
            .acceptUnitPrefix = false,
            .acceptReal = false,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_pitch_lfo_freq FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_pitch_lfo_freq(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = true,
            .unit = VM_HERTZ,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_vol_time FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_vol_time(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = false,
            .unit = VM_SECOND,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_tune_time FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_tune_time(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = false,
            .unit = VM_SECOND,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_pan_time FINAL : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_pan_time(InstrumentScriptVM* parent) : VMChangeSynthParamFunction({
            .vm = parent,
            .acceptFinal = false,
            .unit = VM_SECOND,
            .acceptUnitPrefix = true,
            .acceptReal = true,
        }) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class VMChangeFadeCurveFunction : public VMEmptyResultFunction {
    public:
        VMChangeFadeCurveFunction(InstrumentScriptVM* parent) : m_vm(parent) {}
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;

        template<fade_curve_t NoteBase::_Override::*T_noteParam, vmint T_synthParam>
        VMFnResult* execTemplate(VMFnArgs* args, const char* functionName);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_vol_curve FINAL : public VMChangeFadeCurveFunction {
    public:
        InstrumentScriptVMFunction_change_vol_curve(InstrumentScriptVM* parent) : VMChangeFadeCurveFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_tune_curve FINAL : public VMChangeFadeCurveFunction {
    public:
        InstrumentScriptVMFunction_change_tune_curve(InstrumentScriptVM* parent) : VMChangeFadeCurveFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_change_pan_curve FINAL : public VMChangeFadeCurveFunction {
    public:
        InstrumentScriptVMFunction_change_pan_curve(InstrumentScriptVM* parent) : VMChangeFadeCurveFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_fade_in FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_fade_in(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_fade_out FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_fade_out(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 3; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_get_event_par FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_get_event_par(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_event_par FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_set_event_par(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 3; }
        vmint maxAllowedArgs() const OVERRIDE { return 3; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_note FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_note(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_velo FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_velo(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_play_pos FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_play_pos(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        bool acceptsArgUnitType(vmint iArg, StdUnit_t type) const OVERRIDE;
        bool acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_event_status FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_event_status(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_callback_status FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_callback_status(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    // overrides core wait() implementation
    class InstrumentScriptVMFunction_wait FINAL : public CoreVMFunction_wait {
    public:
        InstrumentScriptVMFunction_wait(InstrumentScriptVM* parent);
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    };

    class InstrumentScriptVMFunction_stop_wait FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_stop_wait(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_abort FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_abort(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 1; }
        vmint maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_fork FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_fork(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 0; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

} // namespace LinuxSampler

#endif // LS_INSTRSCRIPTVMFUNCTIONS_H
