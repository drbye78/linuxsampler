/*
 * Copyright (c) 2016 - 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "InstrumentScriptVMFunctions.h"
#include "InstrumentScriptVMDynVars.h"
#include "InstrumentScriptVM.h"
#include "../AbstractEngineChannel.h"
#include "../EngineBase.h"

namespace LinuxSampler {

    // built-in variable $ENGINE_UPTIME

    vmint InstrumentScriptVMDynVar_ENGINE_UPTIME::evalInt() {

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        AbstractEngine* pEngine =
            static_cast<AbstractEngine*>(pEngineChannel->GetEngine());

        // engine's official playback time in milliseconds (offline bounce safe)
        return vmint( double(pEngine->FrameTime + m_vm->m_event->cause.FragmentPos()) /
                      double(pEngine->SampleRate) * 1000.0 );
    }

    // built-in variable $NI_CALLBACK_ID

    vmint InstrumentScriptVMDynVar_NI_CALLBACK_ID::evalInt() {

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        return pEngineChannel->GetScriptCallbackID(m_vm->m_event);
    }

    // built-in array variable %NKSP_CALLBACK_CHILD_ID[]

    InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID::InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMIntArrayExpr* InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID::asIntArray() const {
        return const_cast<VMIntArrayExpr*>( dynamic_cast<const VMIntArrayExpr*>(this) );
    }

    vmint InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID::arraySize() const {
        return m_vm->m_event->countChildHandlers();
    }

    vmint InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID::evalIntElement(vmuint i) {
        if (i >= arraySize()) return 0;
        return m_vm->m_event->childHandlerID[i];
    }

    // built-in variable %ALL_EVENTS

    InstrumentScriptVMDynVar_ALL_EVENTS::InstrumentScriptVMDynVar_ALL_EVENTS(InstrumentScriptVM* parent)
        : m_vm(parent), m_ids(NULL), m_numIDs(0)
    {
        m_ids = new note_id_t[GLOBAL_MAX_NOTES];
        memset(&m_ids[0], 0, GLOBAL_MAX_NOTES * sizeof(note_id_t));
    }

    InstrumentScriptVMDynVar_ALL_EVENTS::~InstrumentScriptVMDynVar_ALL_EVENTS() {
        if (m_ids) delete[] m_ids;
    }

    VMIntArrayExpr* InstrumentScriptVMDynVar_ALL_EVENTS::asIntArray() const {
        const_cast<InstrumentScriptVMDynVar_ALL_EVENTS*>(this)->updateNoteIDs();
        return const_cast<VMIntArrayExpr*>( dynamic_cast<const VMIntArrayExpr*>(this) );
    }

    vmint InstrumentScriptVMDynVar_ALL_EVENTS::arraySize() const {
        return m_numIDs;
    }

    vmint InstrumentScriptVMDynVar_ALL_EVENTS::evalIntElement(vmuint i) {
        if (i >= m_numIDs) return 0;
        return m_ids[i];
    }

    void InstrumentScriptVMDynVar_ALL_EVENTS::updateNoteIDs() {

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        m_numIDs = pEngineChannel->AllNoteIDs(&m_ids[0], GLOBAL_MAX_NOTES);

        // translate sampler engine internal note IDs to public script id scope
        for (vmuint i = 0; i < m_numIDs; ++i)
            m_ids[i] = ScriptID::fromNoteID(m_ids[i]);
    }

} // namespace LinuxSampler
