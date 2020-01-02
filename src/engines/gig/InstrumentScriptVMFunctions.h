/*
 * Copyright (c) 2014 - 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_GIG_INSTRSCRIPTVMFUNCTIONS_H
#define LS_GIG_INSTRSCRIPTVMFUNCTIONS_H

#include "../common/InstrumentScriptVMFunctions.h"

namespace LinuxSampler { namespace gig {

    class InstrumentScriptVM;

    /**
     * Built-in script function:
     * 
     *     gig_set_dim_zone(event_id, dimension, zone)
     */
    class InstrumentScriptVMFunction_gig_set_dim_zone FINAL : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_gig_set_dim_zone(InstrumentScriptVM* parent);
        vmint minRequiredArgs() const OVERRIDE { return 3; }
        vmint maxAllowedArgs() const OVERRIDE { return 3; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE;
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    /**
     * Built-in script function:
     * 
     *     same_region(key1, key2)
     */
    class InstrumentScriptVMFunction_same_region FINAL : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_same_region(InstrumentScriptVM* parent);
        StdUnit_t returnUnitType(VMFnArgs* args) OVERRIDE { return VM_NO_UNIT; }
        bool returnsFinal(VMFnArgs* args) OVERRIDE { return false; }
        vmint minRequiredArgs() const OVERRIDE { return 2; }
        vmint maxAllowedArgs() const OVERRIDE { return 2; }
        bool acceptsArgType(vmint iArg, ExprType_t type) const OVERRIDE { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

}} // namespace LinuxSampler::gig

#endif // LS_GIG_INSTRSCRIPTVMFUNCTIONS_H
