/*
 * Copyright (c) 2014-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "InstrumentScriptVMFunctions.h"
#include "InstrumentScriptVM.h"
#include "../AbstractEngineChannel.h"
#include "../../common/global_private.h"

namespace LinuxSampler {

    // play_note() function

    InstrumentScriptVMFunction_play_note::InstrumentScriptVMFunction_play_note(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_play_note::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 2 || iArg == 3)
            return type == INT_EXPR || type == REAL_EXPR;
        else
            return type == INT_EXPR;
    }

    bool InstrumentScriptVMFunction_play_note::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 2 || iArg == 3)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_play_note::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        if (iArg == 2 || iArg == 3)
            return type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
        else
            return false;
    }

    void InstrumentScriptVMFunction_play_note::checkArgs(VMFnArgs* args,
                                                         std::function<void(String)> err,
                                                         std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->arg(0)->isConstExpr()) {
            vmint note = args->arg(0)->asNumber()->evalCastInt();
            if (note < 0 || note > 127) {
                err("MIDI note number value for argument 1 must be between 0..127");
                return;
            }
        }
        if (args->argsCount() >= 2 && args->arg(1)->isConstExpr()) {
            vmint velocity = args->arg(1)->asNumber()->evalCastInt();
            if (velocity < 0 || velocity > 127) {
                err("MIDI velocity value for argument 2 must be between 0..127");
                return;
            }
        }
        if (args->argsCount() >= 3 && args->arg(2)->isConstExpr()) {
            VMNumberExpr* argSampleOffset = args->arg(2)->asNumber();
            vmint sampleoffset =
                (argSampleOffset->unitType()) ?
                    argSampleOffset->evalCastInt(VM_MICRO) :
                    argSampleOffset->evalCastInt();
            if (sampleoffset < -1) {
                err("Sample offset of argument 3 may not be less than -1");
                return;
            }
        }
        if (args->argsCount() >= 4 && args->arg(3)->isConstExpr()) {
            VMNumberExpr* argDuration = args->arg(3)->asNumber();
            vmint duration =
                (argDuration->unitType()) ?
                    argDuration->evalCastInt(VM_MICRO) :
                    argDuration->evalCastInt();
            if (duration < -2) {
                err("Argument 4 must be a duration value of at least -2 or higher");
                return;
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_play_note::exec(VMFnArgs* args) {
        vmint note = args->arg(0)->asInt()->evalInt();
        vmint velocity = (args->argsCount() >= 2) ? args->arg(1)->asInt()->evalInt() : 127;
        VMNumberExpr* argDuration = (args->argsCount() >= 4) ? args->arg(3)->asNumber() : NULL;
        vmint duration =
            (argDuration) ?
                (argDuration->unitType()) ?
                    argDuration->evalCastInt(VM_MICRO) :
                    argDuration->evalCastInt() : 0; //TODO: -1 might be a better default value instead of 0

        if (note < 0 || note > 127) {
            errMsg("play_note(): argument 1 is an invalid note number");
            return errorResult(0);
        }

        if (velocity < 0 || velocity > 127) {
            errMsg("play_note(): argument 2 is an invalid velocity value");
            return errorResult(0);
        }

        if (duration < -2) {
            errMsg("play_note(): argument 4 must be a duration value of at least -2 or higher");
            return errorResult(0);
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
        e.Init(); // clear IDs
        e.Type = Event::type_play_note;
        e.Param.Note.Key = note;
        e.Param.Note.Velocity = velocity;
        // make this new note dependent to the life time of the original note
        if (duration == -1) {
            if (m_vm->currentVMEventHandler()->eventHandlerType() != VM_EVENT_HANDLER_NOTE) {
                errMsg("play_note(): -1 for argument 4 may only be used for note event handlers");
                return errorResult(0);
            }
            e.Param.Note.ParentNoteID = m_vm->m_event->cause.Param.Note.ID;
            // check if that requested parent note is actually still alive
            NoteBase* pParentNote =
                pEngineChannel->pEngine->NoteByID( e.Param.Note.ParentNoteID );
            // if parent note is already gone then this new note is not required anymore
            if (!pParentNote)
                return successResult(0);
        }

        const note_id_t id = pEngineChannel->ScheduleNoteMicroSec(&e, 0);

        // if a sample offset is supplied, assign the offset as override
        // to the previously created Note object
        if (args->argsCount() >= 3) {
            VMNumberExpr* argSampleOffset = args->arg(2)->asNumber();
            vmint sampleoffset =
                (argSampleOffset->unitType()) ?
                    argSampleOffset->evalCastInt(VM_MICRO) :
                    argSampleOffset->evalCastInt();
            if (sampleoffset >= 0) {
                NoteBase* pNote = pEngineChannel->pEngine->NoteByID(id);
                if (pNote) {
                    pNote->Override.SampleOffset =
                        (decltype(pNote->Override.SampleOffset)) sampleoffset;
                }
            } else if (sampleoffset < -1) {
                errMsg("play_note(): sample offset of argument 3 may not be less than -1");
            }
        }

        // if a duration is supplied (and play-note event was scheduled
        // successfully above), then schedule a subsequent stop-note event
        if (id && duration > 0) {
            e.Type = Event::type_stop_note;
            e.Param.Note.ID = id;
            e.Param.Note.Velocity = 127;
            pEngineChannel->ScheduleEventMicroSec(&e, duration);
        }

        // even if id is null, don't return an errorResult() here, because that
        // would abort the script, and under heavy load it may be considerable
        // that ScheduleNoteMicroSec() fails above, so simply ignore that
        return successResult( ScriptID::fromNoteID(id) );
    }

    // set_controller() function

    InstrumentScriptVMFunction_set_controller::InstrumentScriptVMFunction_set_controller(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_set_controller::exec(VMFnArgs* args) {
        vmint controller = args->arg(0)->asInt()->evalInt();
        vmint value      = args->arg(1)->asInt()->evalInt();

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
        e.Init(); // clear IDs
        if (controller == CTRL_TABLE_IDX_AFTERTOUCH) {
            e.Type = Event::type_channel_pressure;
            e.Param.ChannelPressure.Value = value & 127;
        } else if (controller == CTRL_TABLE_IDX_PITCHBEND) {
            e.Type = Event::type_pitchbend;
            e.Param.Pitch.Pitch = value;
        } else if (controller >= 0 && controller <= 127) {
            e.Type = Event::type_control_change;
            e.Param.CC.Controller = controller;
            e.Param.CC.Value = value;
        } else {
            errMsg("set_controller(): argument 1 is an invalid controller");
            return errorResult();
        }

        const event_id_t id = pEngineChannel->ScheduleEventMicroSec(&e, 0);

        // even if id is null, don't return an errorResult() here, because that
        // would abort the script, and under heavy load it may be considerable
        // that ScheduleEventMicroSec() fails above, so simply ignore that
        return successResult( ScriptID::fromEventID(id) );
    }

    // set_rpn() function

    InstrumentScriptVMFunction_set_rpn::InstrumentScriptVMFunction_set_rpn(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_set_rpn::exec(VMFnArgs* args) {
        vmint parameter = args->arg(0)->asInt()->evalInt();
        vmint value     = args->arg(1)->asInt()->evalInt();

        if (parameter < 0 || parameter > 16383) {
            errMsg("set_rpn(): argument 1 exceeds RPN parameter number range");
            return errorResult();
        }
        if (value < 0 || value > 16383) {
            errMsg("set_rpn(): argument 2 exceeds RPN value range");
            return errorResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
        e.Init(); // clear IDs
        e.Type = Event::type_rpn;
        e.Param.RPN.Parameter = parameter;
        e.Param.RPN.Value = value;

        const event_id_t id = pEngineChannel->ScheduleEventMicroSec(&e, 0);

        // even if id is null, don't return an errorResult() here, because that
        // would abort the script, and under heavy load it may be considerable
        // that ScheduleEventMicroSec() fails above, so simply ignore that
        return successResult( ScriptID::fromEventID(id) );
    }

    // set_nrpn() function

    InstrumentScriptVMFunction_set_nrpn::InstrumentScriptVMFunction_set_nrpn(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_set_nrpn::exec(VMFnArgs* args) {
        vmint parameter = args->arg(0)->asInt()->evalInt();
        vmint value     = args->arg(1)->asInt()->evalInt();

        if (parameter < 0 || parameter > 16383) {
            errMsg("set_nrpn(): argument 1 exceeds NRPN parameter number range");
            return errorResult();
        }
        if (value < 0 || value > 16383) {
            errMsg("set_nrpn(): argument 2 exceeds NRPN value range");
            return errorResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
        e.Init(); // clear IDs
        e.Type = Event::type_nrpn;
        e.Param.NRPN.Parameter = parameter;
        e.Param.NRPN.Value = value;

        const event_id_t id = pEngineChannel->ScheduleEventMicroSec(&e, 0);

        // even if id is null, don't return an errorResult() here, because that
        // would abort the script, and under heavy load it may be considerable
        // that ScheduleEventMicroSec() fails above, so simply ignore that
        return successResult( ScriptID::fromEventID(id) );
    }

    // ignore_event() function

    InstrumentScriptVMFunction_ignore_event::InstrumentScriptVMFunction_ignore_event(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_ignore_event::acceptsArgType(vmint iArg, ExprType_t type) const {
        return type == INT_EXPR || type == INT_ARR_EXPR;
    }

    VMFnResult* InstrumentScriptVMFunction_ignore_event::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
                static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->argsCount() == 0 || args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = (args->argsCount() >= 1) ? args->arg(0)->asInt()->evalInt() : m_vm->m_event->id;
            if (!id && args->argsCount() >= 1) {
                wrnMsg("ignore_event(): event ID argument may not be zero");
                // not errorResult(), because that would abort the script, not intentional in this case
                return successResult();
            }
            pEngineChannel->IgnoreEventByScriptID(id);
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (int i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);     
                pEngineChannel->IgnoreEventByScriptID(id);
            }
        }

        return successResult();
    }

    // ignore_controller() function

    InstrumentScriptVMFunction_ignore_controller::InstrumentScriptVMFunction_ignore_controller(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_ignore_controller::exec(VMFnArgs* args) {
        const ScriptID id = (args->argsCount() >= 1) ? args->arg(0)->asInt()->evalInt() : m_vm->m_event->id;
        if (!id && args->argsCount() >= 1) {
            wrnMsg("ignore_controller(): event ID argument may not be zero");
            return successResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        pEngineChannel->IgnoreEventByScriptID(id);

        return successResult();
    }

    // note_off() function

    InstrumentScriptVMFunction_note_off::InstrumentScriptVMFunction_note_off(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_note_off::acceptsArgType(vmint iArg, ExprType_t type) const {
        return type == INT_EXPR || type == INT_ARR_EXPR;
    }

    void InstrumentScriptVMFunction_note_off::checkArgs(VMFnArgs* args,
                                                        std::function<void(String)> err,
                                                        std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2 && args->arg(1)->isConstExpr() && args->arg(1)->exprType() == INT_EXPR) {
            vmint velocity = args->arg(1)->asInt()->evalInt();
            if (velocity < 0 || velocity > 127) {
                err("MIDI velocity value for argument 2 must be between 0..127");
                return;
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_note_off::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        vmint velocity = (args->argsCount() >= 2) ? args->arg(1)->asInt()->evalInt() : 127;
        if (velocity < 0 || velocity > 127) {
            errMsg("note_off(): argument 2 is an invalid velocity value");
            return errorResult();
        }

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();   
            if (!id) {
                wrnMsg("note_off(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("note_off(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            Event e = pNote->cause;
            e.Init(); // clear IDs
            e.CopyTimeFrom(m_vm->m_event->cause); // set fragment time for "now"
            e.Type = Event::type_stop_note;
            e.Param.Note.ID = id.noteID();
            e.Param.Note.Key = pNote->hostKey;
            e.Param.Note.Velocity = velocity;

            pEngineChannel->ScheduleEventMicroSec(&e, 0);
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                Event e = pNote->cause;
                e.Init(); // clear IDs
                e.CopyTimeFrom(m_vm->m_event->cause); // set fragment time for "now"
                e.Type = Event::type_stop_note;
                e.Param.Note.ID = id.noteID();
                e.Param.Note.Key = pNote->hostKey;
                e.Param.Note.Velocity = velocity;

                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        }

        return successResult();
    }

    // set_event_mark() function

    InstrumentScriptVMFunction_set_event_mark::InstrumentScriptVMFunction_set_event_mark(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    void InstrumentScriptVMFunction_set_event_mark::checkArgs(VMFnArgs* args,
                                                              std::function<void(String)> err,
                                                              std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2 && args->arg(1)->isConstExpr()) {
            const vmint groupID = args->arg(1)->asInt()->evalInt();
            if (groupID < 0 || groupID >= INSTR_SCRIPT_EVENT_GROUPS) {
                err("Argument 2 value is an invalid group id.");
                return;
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_set_event_mark::exec(VMFnArgs* args) {
        const ScriptID id = args->arg(0)->asInt()->evalInt();
        const vmint groupID = args->arg(1)->asInt()->evalInt();

        if (groupID < 0 || groupID >= INSTR_SCRIPT_EVENT_GROUPS) {
            errMsg("set_event_mark(): argument 2 is an invalid group id");
            return errorResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        // check if the event/note still exists
        switch (id.type()) {
            case ScriptID::EVENT: {
                RTList<Event>::Iterator itEvent = pEngineChannel->pEngine->EventByID( id.eventID() );
                if (!itEvent) return successResult();
                break;
            }
            case ScriptID::NOTE: {
                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) return successResult();
                break;
            }
        }

        pEngineChannel->pScript->eventGroups[groupID].insert(id);

        return successResult();
    }

    // delete_event_mark() function

    InstrumentScriptVMFunction_delete_event_mark::InstrumentScriptVMFunction_delete_event_mark(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    void InstrumentScriptVMFunction_delete_event_mark::checkArgs(VMFnArgs* args,
                                                                 std::function<void(String)> err,
                                                                 std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2 && args->arg(1)->isConstExpr()) {
            const vmint groupID = args->arg(1)->asInt()->evalInt();
            if (groupID < 0 || groupID >= INSTR_SCRIPT_EVENT_GROUPS) {
                err("Argument 2 value is an invalid group id.");
                return;
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_delete_event_mark::exec(VMFnArgs* args) {
        const ScriptID id = args->arg(0)->asInt()->evalInt();
        const vmint groupID = args->arg(1)->asInt()->evalInt();

        if (groupID < 0 || groupID >= INSTR_SCRIPT_EVENT_GROUPS) {
            errMsg("delete_event_mark(): argument 2 is an invalid group id");
            return errorResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        pEngineChannel->pScript->eventGroups[groupID].erase(id);

        return successResult();
    }

    // by_marks() function

    InstrumentScriptVMFunction_by_marks::InstrumentScriptVMFunction_by_marks(InstrumentScriptVM* parent)
        : m_vm(parent), m_result(NULL)
    {
    }

    vmint InstrumentScriptVMFunction_by_marks::Result::arraySize() const {
        return eventGroup->size();
    }

    vmint InstrumentScriptVMFunction_by_marks::Result::evalIntElement(vmuint i) {
        return (*eventGroup)[i];
    }

    VMFnResult* InstrumentScriptVMFunction_by_marks::errorResult() {
        m_result->eventGroup = NULL;
        m_result->flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
        return m_result;
    }

    VMFnResult* InstrumentScriptVMFunction_by_marks::successResult(EventGroup* eventGroup) {
        m_result->eventGroup = eventGroup;
        m_result->flags = STMT_SUCCESS;
        return m_result;
    }

    void InstrumentScriptVMFunction_by_marks::checkArgs(VMFnArgs* args,
                                                        std::function<void(String)> err,
                                                        std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->arg(0)->isConstExpr()) {
            const vmint groupID = args->arg(0)->asInt()->evalInt();
            if (groupID < 0 || groupID >= INSTR_SCRIPT_EVENT_GROUPS) {
                err("Argument value is an invalid group id.");
                return;
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_by_marks::allocResult(VMFnArgs* args) {
        return new Result;
    }

    void InstrumentScriptVMFunction_by_marks::bindResult(VMFnResult* res) {
        m_result = dynamic_cast<Result*>(res);
    }

    VMFnResult* InstrumentScriptVMFunction_by_marks::boundResult() const {
        return m_result;
    }

    VMFnResult* InstrumentScriptVMFunction_by_marks::exec(VMFnArgs* args) {
        vmint groupID = args->arg(0)->asInt()->evalInt();

        if (groupID < 0 || groupID >= INSTR_SCRIPT_EVENT_GROUPS) {
            errMsg("by_marks(): argument is an invalid group id");
            return errorResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        return successResult( &pEngineChannel->pScript->eventGroups[groupID] );
    }

    // change_vol() function

    InstrumentScriptVMFunction_change_vol::InstrumentScriptVMFunction_change_vol(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_vol::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else if (iArg == 1)
            return type == INT_EXPR || type == REAL_EXPR;
        else
            return type == INT_EXPR;
    }

    bool InstrumentScriptVMFunction_change_vol::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_BEL;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_change_vol::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_BEL; // only allow metric prefix(es) if 'Bel' is used as unit type
    }

    bool InstrumentScriptVMFunction_change_vol::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    VMFnResult* InstrumentScriptVMFunction_change_vol::exec(VMFnArgs* args) {
        StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint volume =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MILLI,VM_DECI) :
                args->arg(1)->asNumber()->evalCastInt(); // volume change in milli dB
        bool isFinal = args->arg(1)->asNumber()->isFinal();
        bool relative = (args->argsCount() >= 3) ? (args->arg(2)->asInt()->evalInt() & 1) : false;
        const float fVolumeLin = RTMath::DecibelToLinRatio(float(volume) / 1000.f);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_vol(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_vol(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_vol() was called immediately after note was triggered
            // then immediately apply the volume to note object, but only if
            // change_vol_time() has not been called before
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime &&
                pNote->Override.VolumeTime <= DEFAULT_NOTE_VOLUME_TIME_S)
            {
                if (relative)
                    pNote->Override.Volume.Value *= fVolumeLin;
                else
                    pNote->Override.Volume.Value = fVolumeLin;
                pNote->Override.Volume.Final = isFinal;
            } else { // otherwise schedule the volume change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_volume;
                e.Param.NoteSynthParam.Delta    = fVolumeLin;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, relative, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_vol() was called immediately after note was triggered
                // then immediately apply the volume to Note object, but only if
                // change_vol_time() has not been called before
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime &&
                    pNote->Override.VolumeTime <= DEFAULT_NOTE_VOLUME_TIME_S)
                {
                    if (relative)
                        pNote->Override.Volume.Value *= fVolumeLin;
                    else
                        pNote->Override.Volume.Value = fVolumeLin;
                    pNote->Override.Volume.Final = isFinal;
                } else { // otherwise schedule the volume change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_volume;
                    e.Param.NoteSynthParam.Delta    = fVolumeLin;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, relative, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_tune() function

    InstrumentScriptVMFunction_change_tune::InstrumentScriptVMFunction_change_tune(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_tune::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else if (iArg == 1)
            return type == INT_EXPR || type == REAL_EXPR;
        else
            return type == INT_EXPR;
    }

    bool InstrumentScriptVMFunction_change_tune::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1;
    }

    bool InstrumentScriptVMFunction_change_tune::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    VMFnResult* InstrumentScriptVMFunction_change_tune::exec(VMFnArgs* args) {
        vmint tune =
            (args->arg(1)->asNumber()->hasUnitFactorNow())
                ? args->arg(1)->asNumber()->evalCastInt(VM_MILLI,VM_CENTI)
                : args->arg(1)->asNumber()->evalCastInt(); // tuning change in milli cents
        bool isFinal = args->arg(1)->asNumber()->isFinal();
        StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        bool relative = (args->argsCount() >= 3) ? (args->arg(2)->asInt()->evalInt() & 1) : false;
        const float fFreqRatio = RTMath::CentsToFreqRatioUnlimited(float(tune) / 1000.f);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_tune(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_tune(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_tune() was called immediately after note was triggered
            // then immediately apply the tuning to Note object, but only if
            // change_tune_time() has not been called before
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime &&
                pNote->Override.PitchTime <= DEFAULT_NOTE_PITCH_TIME_S)
            {
                if (relative) 
                    pNote->Override.Pitch.Value *= fFreqRatio;
                else
                    pNote->Override.Pitch.Value = fFreqRatio;
                pNote->Override.Pitch.Final = isFinal;
            } else { // otherwise schedule tuning change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_pitch;
                e.Param.NoteSynthParam.Delta    = fFreqRatio;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, relative, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_tune() was called immediately after note was triggered
                // then immediately apply the tuning to Note object, but only if
                // change_tune_time() has not been called before
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime &&
                    pNote->Override.PitchTime <= DEFAULT_NOTE_PITCH_TIME_S)
                {
                    if (relative) 
                        pNote->Override.Pitch.Value *= fFreqRatio;
                    else
                        pNote->Override.Pitch.Value = fFreqRatio;
                    pNote->Override.Pitch.Final = isFinal;
                } else { // otherwise schedule tuning change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_pitch;
                    e.Param.NoteSynthParam.Delta    = fFreqRatio;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, relative, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_pan() function

    InstrumentScriptVMFunction_change_pan::InstrumentScriptVMFunction_change_pan(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_pan::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR;
    }

    bool InstrumentScriptVMFunction_change_pan::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    VMFnResult* InstrumentScriptVMFunction_change_pan::exec(VMFnArgs* args) {
        vmint pan    = args->arg(1)->asInt()->evalInt();
        bool isFinal = args->arg(1)->asInt()->isFinal();
        bool relative = (args->argsCount() >= 3) ? (args->arg(2)->asInt()->evalInt() & 1) : false;

        if (pan > 1000) {
            wrnMsg("change_pan(): argument 2 may not be larger than 1000");
            pan = 1000;
        } else if (pan < -1000) {
            wrnMsg("change_pan(): argument 2 may not be smaller than -1000");
            pan = -1000;
        }
        const float fPan = float(pan) / 1000.f;

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_pan(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_pan(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_pan() was called immediately after note was triggered
            // then immediately apply the panning to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                if (relative) {
                    pNote->Override.Pan.Value = RTMath::RelativeSummedAvg(pNote->Override.Pan.Value, fPan, ++pNote->Override.Pan.Sources);
                } else {
                    pNote->Override.Pan.Value = fPan;
                    pNote->Override.Pan.Sources = 1; // only relevant on subsequent change_pan() calls on same note with 'relative' being set
                }
                pNote->Override.Pan.Final = isFinal;
            } else { // otherwise schedule panning change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_pan;
                e.Param.NoteSynthParam.Delta    = fPan;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, relative, false
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_pan() was called immediately after note was triggered
                // then immediately apply the panning to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    if (relative) {
                        pNote->Override.Pan.Value = RTMath::RelativeSummedAvg(pNote->Override.Pan.Value, fPan, ++pNote->Override.Pan.Sources);
                    } else {
                        pNote->Override.Pan.Value = fPan;
                        pNote->Override.Pan.Sources = 1; // only relevant on subsequent change_pan() calls on same note with 'relative' being set
                    }
                    pNote->Override.Pan.Final = isFinal;
                } else { // otherwise schedule panning change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_pan;
                    e.Param.NoteSynthParam.Delta    = fPan;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, relative, false
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    #define VM_FILTER_PAR_MAX_VALUE 1000000
    #define VM_FILTER_PAR_MAX_HZ 30000
    #define VM_EG_PAR_MAX_VALUE 1000000

    // change_cutoff() function

    InstrumentScriptVMFunction_change_cutoff::InstrumentScriptVMFunction_change_cutoff(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_cutoff::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else if (iArg == 1)
            return type == INT_EXPR || type == REAL_EXPR;
        else
            return type == INT_EXPR;
    }

    bool InstrumentScriptVMFunction_change_cutoff::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_HERTZ;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_change_cutoff::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_HERTZ; // only allow metric prefix(es) if 'Hz' is used as unit type
    }

    bool InstrumentScriptVMFunction_change_cutoff::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    void InstrumentScriptVMFunction_change_cutoff::checkArgs(VMFnArgs* args,
                                                             std::function<void(String)> err,
                                                             std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2) {
            VMNumberExpr* argCutoff = args->arg(1)->asNumber();
            if (argCutoff->unitType() && !argCutoff->isFinal()) {
                wrn("Argument 2 implies 'final' value when using Hz as unit for cutoff frequency.");
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_change_cutoff::exec(VMFnArgs* args) {
        const StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint cutoff =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_NO_PREFIX) :
                args->arg(1)->asNumber()->evalCastInt();
        const bool isFinal =
            (unit) ?
                true : // imply 'final' value if unit type is used
                args->arg(1)->asNumber()->isFinal();
        // note: intentionally not checking against a max. value here if no unit!
        // (to allow i.e. passing 2000000 for doubling cutoff frequency)
        if (unit && cutoff > VM_FILTER_PAR_MAX_HZ) {
            wrnMsg("change_cutoff(): argument 2 may not be larger than " strfy(VM_FILTER_PAR_MAX_HZ) " Hz");
            cutoff = VM_FILTER_PAR_MAX_HZ;
        } else if (cutoff < 0) {
            wrnMsg("change_cutoff(): argument 2 may not be negative");
            cutoff = 0;
        }
        const float fCutoff =
            (unit) ? cutoff : float(cutoff) / float(VM_FILTER_PAR_MAX_VALUE);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_cutoff(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_cutoff(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_cutoff() was called immediately after note was triggered
            // then immediately apply cutoff to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.Cutoff.Value = fCutoff;
                pNote->Override.Cutoff.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
            } else { // otherwise schedule cutoff change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_cutoff;
                e.Param.NoteSynthParam.Delta    = fCutoff;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, false, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_cutoff() was called immediately after note was triggered
                // then immediately apply cutoff to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.Cutoff.Value = fCutoff;
                    pNote->Override.Cutoff.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
                } else { // otherwise schedule cutoff change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_cutoff;
                    e.Param.NoteSynthParam.Delta    = fCutoff;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, false, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_reso() function

    InstrumentScriptVMFunction_change_reso::InstrumentScriptVMFunction_change_reso(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_reso::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR;
    }

    bool InstrumentScriptVMFunction_change_reso::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    VMFnResult* InstrumentScriptVMFunction_change_reso::exec(VMFnArgs* args) {
        vmint resonance = args->arg(1)->asInt()->evalInt();
        bool isFinal    = args->arg(1)->asInt()->isFinal();
        // note: intentionally not checking against a max. value here!
        // (to allow i.e. passing 2000000 for doubling the resonance)
        if (resonance < 0) {
            wrnMsg("change_reso(): argument 2 may not be negative");
            resonance = 0;
        }
        const float fResonance = float(resonance) / float(VM_FILTER_PAR_MAX_VALUE);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_reso(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_reso(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_reso() was called immediately after note was triggered
            // then immediately apply resonance to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.Resonance.Value = fResonance;
                pNote->Override.Resonance.Final = isFinal;
            } else { // otherwise schedule resonance change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_resonance;
                e.Param.NoteSynthParam.Delta    = fResonance;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, false, false
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_reso() was called immediately after note was triggered
                // then immediately apply resonance to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.Resonance.Value = fResonance;
                    pNote->Override.Resonance.Final = isFinal;
                } else { // otherwise schedule resonance change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_resonance;
                    e.Param.NoteSynthParam.Delta    = fResonance;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, false, false
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_attack() function
    //
    //TODO: Derive from generalized, shared template class
    // VMChangeSynthParamFunction instead (like e.g. change_cutoff_attack()
    // implementation below already does) to ease maintenance.

    InstrumentScriptVMFunction_change_attack::InstrumentScriptVMFunction_change_attack(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_attack::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR || type == REAL_EXPR;
    }

    bool InstrumentScriptVMFunction_change_attack::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_change_attack::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
    }

    bool InstrumentScriptVMFunction_change_attack::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    void InstrumentScriptVMFunction_change_attack::checkArgs(VMFnArgs* args,
                                                             std::function<void(String)> err,
                                                             std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2) {
            VMNumberExpr* argTime = args->arg(1)->asNumber();
            if (argTime->unitType() && !argTime->isFinal()) {
                wrn("Argument 2 implies 'final' value when using seconds as unit for attack time.");
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_change_attack::exec(VMFnArgs* args) {
        const StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint attack =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MICRO) :
                args->arg(1)->asNumber()->evalCastInt();
        const bool isFinal =
            (unit) ?
                true : // imply 'final' value if unit type is used
                args->arg(1)->asNumber()->isFinal();
        // note: intentionally not checking against a max. value here!
        // (to allow i.e. passing 2000000 for doubling the attack time)
        if (attack < 0) {
            wrnMsg("change_attack(): argument 2 may not be negative");
            attack = 0;
        }
        const float fAttack =
            (unit) ?
                float(attack) / 1000000.f /* us -> s */ :
                float(attack) / float(VM_EG_PAR_MAX_VALUE);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_attack(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_attack(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_attack() was called immediately after note was triggered
            // then immediately apply attack to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.Attack.Value = fAttack;
                pNote->Override.Attack.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
            } else { // otherwise schedule attack change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_attack;
                e.Param.NoteSynthParam.Delta    = fAttack;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, false, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_attack() was called immediately after note was triggered
                // then immediately apply attack to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.Attack.Value = fAttack;
                    pNote->Override.Attack.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
                } else { // otherwise schedule attack change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_attack;
                    e.Param.NoteSynthParam.Delta    = fAttack;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, false, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_decay() function
    //
    //TODO: Derive from generalized, shared template class
    // VMChangeSynthParamFunction instead (like e.g. change_cutoff_decay()
    // implementation below already does) to ease maintenance.

    InstrumentScriptVMFunction_change_decay::InstrumentScriptVMFunction_change_decay(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_decay::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR || type == REAL_EXPR;
    }

    bool InstrumentScriptVMFunction_change_decay::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_change_decay::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
    }

    bool InstrumentScriptVMFunction_change_decay::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    void InstrumentScriptVMFunction_change_decay::checkArgs(VMFnArgs* args,
                                                            std::function<void(String)> err,
                                                            std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2) {
            VMNumberExpr* argTime = args->arg(1)->asNumber();
            if (argTime->unitType() && !argTime->isFinal()) {
                wrn("Argument 2 implies 'final' value when using seconds as unit for decay time.");
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_change_decay::exec(VMFnArgs* args) {
        const StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint decay =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MICRO) :
                args->arg(1)->asNumber()->evalCastInt();
        const bool isFinal =
            (unit) ?
                true : // imply 'final' value if unit type is used
                args->arg(1)->asNumber()->isFinal();
        // note: intentionally not checking against a max. value here!
        // (to allow i.e. passing 2000000 for doubling the decay time)
        if (decay < 0) {
            wrnMsg("change_decay(): argument 2 may not be negative");
            decay = 0;
        }
        const float fDecay =
            (unit) ?
                float(decay) / 1000000.f /* us -> s */ :
                float(decay) / float(VM_EG_PAR_MAX_VALUE);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_decay(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_decay(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_decay() was called immediately after note was triggered
            // then immediately apply decay to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.Decay.Value = fDecay;
                pNote->Override.Decay.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
            } else { // otherwise schedule decay change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_decay;
                e.Param.NoteSynthParam.Delta    = fDecay;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, false, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_decay() was called immediately after note was triggered
                // then immediately apply decay to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.Decay.Value = fDecay;
                    pNote->Override.Decay.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
                } else { // otherwise schedule decay change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_decay;
                    e.Param.NoteSynthParam.Delta    = fDecay;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, false, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_release() function
    //
    //TODO: Derive from generalized, shared template class
    // VMChangeSynthParamFunction instead (like e.g. change_cutoff_release()
    // implementation below already does) to ease maintenance.

    InstrumentScriptVMFunction_change_release::InstrumentScriptVMFunction_change_release(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_release::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR || type == REAL_EXPR;
    }

    bool InstrumentScriptVMFunction_change_release::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_change_release::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
    }

    bool InstrumentScriptVMFunction_change_release::acceptsArgFinal(vmint iArg) const {
        return iArg == 1;
    }

    void InstrumentScriptVMFunction_change_release::checkArgs(VMFnArgs* args,
                                                              std::function<void(String)> err,
                                                              std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (args->argsCount() >= 2) {
            VMNumberExpr* argTime = args->arg(1)->asNumber();
            if (argTime->unitType() && !argTime->isFinal()) {
                wrn("Argument 2 implies 'final' value when using seconds as unit for release time.");
            }
        }
    }

    VMFnResult* InstrumentScriptVMFunction_change_release::exec(VMFnArgs* args) {
        const StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint release =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MICRO) :
                args->arg(1)->asNumber()->evalCastInt();
        const bool isFinal =
            (unit) ?
                true : // imply 'final' value if unit type is used
                args->arg(1)->asNumber()->isFinal();
        // note: intentionally not checking against a max. value here!
        // (to allow i.e. passing 2000000 for doubling the release time)
        if (release < 0) {
            wrnMsg("change_release(): argument 2 may not be negative");
            release = 0;
        }
        const float fRelease =
            (unit) ?
                float(release) / 1000000.f /* us -> s */ :
                float(release) / float(VM_EG_PAR_MAX_VALUE);

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("change_release(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("change_release(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if change_release() was called immediately after note was triggered
            // then immediately apply relase to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.Release.Value = fRelease;
                pNote->Override.Release.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
            } else { // otherwise schedule release change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_release;
                e.Param.NoteSynthParam.Delta    = fRelease;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, false, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if change_release() was called immediately after note was triggered
                // then immediately apply relase to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.Release.Value = fRelease;
                    pNote->Override.Release.Scope = NoteBase::scopeBy_FinalUnit(isFinal, unit);
                } else { // otherwise schedule release change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_release;
                    e.Param.NoteSynthParam.Delta    = fRelease;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, false, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // template for change_*() functions

    bool VMChangeSynthParamFunction::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR || (m_acceptReal && type == REAL_EXPR);
    }

    bool VMChangeSynthParamFunction::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == m_unit;
        else
            return type == VM_NO_UNIT;
    }

    bool VMChangeSynthParamFunction::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return m_acceptUnitPrefix && iArg == 1 && type == m_unit; // only allow metric prefix(es) if approprirate unit type is used (e.g. Hz)
    }

    bool VMChangeSynthParamFunction::acceptsArgFinal(vmint iArg) const {
        return (m_acceptFinal) ? (iArg == 1) : false;
    }

    inline static void setNoteParamScopeBy_FinalUnit(NoteBase::Param& param, const bool bFinal, const StdUnit_t unit) {
        param.Scope = NoteBase::scopeBy_FinalUnit(bFinal, unit);
    }

    inline static void setNoteParamScopeBy_FinalUnit(NoteBase::Norm& param, const bool bFinal, const StdUnit_t unit) {
        param.Final = bFinal;
    }

    inline static void setNoteParamScopeBy_FinalUnit(float& param, const bool bFinal, const StdUnit_t unit) {
        /* NOOP */
    }

    template<class T>
    inline static void setNoteParamValue(T& param, vmfloat value) {
        param.Value = value;
    }

    inline static void setNoteParamValue(float& param, vmfloat value) {
        param = value;
    }

    void VMChangeSynthParamFunction::checkArgs(VMFnArgs* args,
                                               std::function<void(String)> err,
                                               std::function<void(String)> wrn)
    {
        // super class checks
        Super::checkArgs(args, err, wrn);

        // own checks ...
        if (m_unit && m_unit != VM_BEL && args->argsCount() >= 2) {
            VMNumberExpr* arg = args->arg(1)->asNumber();
            if (arg && arg->unitType() && !arg->isFinal()) {
                wrn("Argument 2 implies 'final' value when unit type " +
                    unitTypeStr(arg->unitType()) + " is used.");
            }
        }
    }

    // Arbitrarily chosen constant value symbolizing "no limit".
    #define NO_LIMIT 1315916909

    template<class T_NoteParamType, T_NoteParamType NoteBase::_Override::*T_noteParam,
             vmint T_synthParam,
             vmint T_minValueNorm, vmint T_maxValueNorm, bool T_normalizeNorm,
             vmint T_minValueUnit, vmint T_maxValueUnit,
             MetricPrefix_t T_unitPrefix0, MetricPrefix_t ... T_unitPrefixN>
    VMFnResult* VMChangeSynthParamFunction::execTemplate(VMFnArgs* args, const char* functionName)
    {
        const StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        const bool isFinal =
            (m_unit && m_unit != VM_BEL && unit) ?
                true : // imply 'final' value if unit type is used (except of 'Bel' which may be relative)
                args->arg(1)->asNumber()->isFinal();
        vmint value =
            (m_acceptUnitPrefix && ((m_unit && unit) || (!m_unit && args->arg(1)->asNumber()->hasUnitFactorNow())))
                ? args->arg(1)->asNumber()->evalCastInt(T_unitPrefix0, T_unitPrefixN ...)
                : args->arg(1)->asNumber()->evalCastInt();

        // check if passed value is in allowed range
        if (unit && m_unit) {
            if (T_maxValueUnit != NO_LIMIT && value > T_maxValueUnit) {
                wrnMsg(String(functionName) + "(): argument 2 may not be larger than " + ToString(T_maxValueUnit));
                value = T_maxValueUnit;
            } else if (T_minValueUnit != NO_LIMIT && value < T_minValueUnit) {
                if (T_minValueUnit == 0)
                    wrnMsg(String(functionName) + "(): argument 2 may not be negative");
                else
                    wrnMsg(String(functionName) + "(): argument 2 may not be smaller than " + ToString(T_minValueUnit));
                value = T_minValueUnit;
            }
        } else { // value was passed to this function without a unit ...
            if (T_maxValueNorm != NO_LIMIT && value > T_maxValueNorm) {
                wrnMsg(String(functionName) + "(): argument 2 may not be larger than " + ToString(T_maxValueNorm));
                value = T_maxValueNorm;
            } else if (T_minValueNorm != NO_LIMIT && value < T_minValueNorm) {
                if (T_minValueNorm == 0)
                    wrnMsg(String(functionName) + "(): argument 2 may not be negative");
                else
                    wrnMsg(String(functionName) + "(): argument 2 may not be smaller than " + ToString(T_minValueNorm));
                value = T_minValueNorm;
            }
        }

        // convert passed argument value to engine internal expected value range (i.e. 0.0 .. 1.0)
        const float fValue =
            (unit && m_unit) ?
                (unit == VM_BEL) ?
                    RTMath::DecibelToLinRatio(float(value) * float(T_unitPrefix0) /*i.e. mdB -> dB*/) :
                    float(value) * VMUnit::unitFactor(T_unitPrefix0, T_unitPrefixN ...) /*i.e. us -> s*/ :
                (T_normalizeNorm) ?
                    float(value) / ((T_maxValueNorm != NO_LIMIT) ? float(T_maxValueNorm) : 1000000.f/* fallback: value range used for most */) :
                    float(value) /* as is */;

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg(String(functionName) + "(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg(String(functionName) + "(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if this change_*() script function was called immediately after
            // note was triggered then immediately apply the synth parameter
            // change to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                setNoteParamValue(pNote->Override.*T_noteParam, fValue);
                setNoteParamScopeBy_FinalUnit(
                    (pNote->Override.*T_noteParam),
                    isFinal, unit
                );
            } else { // otherwise schedule this synth parameter change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = (Event::synth_param_t) T_synthParam;
                e.Param.NoteSynthParam.Delta    = fValue;
                e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                    isFinal, false, unit
                );
                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if this change_*() script function was called immediately after
                // note was triggered then immediately apply the synth parameter
                // change to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    setNoteParamValue(pNote->Override.*T_noteParam, fValue);
                    setNoteParamScopeBy_FinalUnit(
                        (pNote->Override.*T_noteParam),
                        isFinal, unit
                    );
                } else { // otherwise schedule this synth parameter change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = (Event::synth_param_t) T_synthParam;
                    e.Param.NoteSynthParam.Delta    = fValue;
                    e.Param.NoteSynthParam.Scope = Event::scopeBy_FinalRelativeUnit(
                        isFinal, false, unit
                    );
                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_sustain() function

    VMFnResult* InstrumentScriptVMFunction_change_sustain::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::Sustain),
                    &NoteBase::_Override::Sustain,
                    Event::synth_param_sustain,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed WITH 'Bel' unit */
                    NO_LIMIT, NO_LIMIT, VM_MILLI, VM_DECI>( args, "change_sustain" );
    }

    // change_cutoff_attack() function

    VMFnResult* InstrumentScriptVMFunction_change_cutoff_attack::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::CutoffAttack),
                    &NoteBase::_Override::CutoffAttack,
                    Event::synth_param_cutoff_attack,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed with 'seconds' unit */
                    0, NO_LIMIT, VM_MICRO>( args, "change_cutoff_attack" );
    }

    // change_cutoff_decay() function

    VMFnResult* InstrumentScriptVMFunction_change_cutoff_decay::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::CutoffDecay),
                    &NoteBase::_Override::CutoffDecay,
                    Event::synth_param_cutoff_decay,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed with 'seconds' unit */
                    0, NO_LIMIT, VM_MICRO>( args, "change_cutoff_decay" );
    }

    // change_cutoff_sustain() function

    VMFnResult* InstrumentScriptVMFunction_change_cutoff_sustain::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::CutoffSustain),
                    &NoteBase::_Override::CutoffSustain,
                    Event::synth_param_cutoff_sustain,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed WITH 'Bel' unit */
                    NO_LIMIT, NO_LIMIT, VM_MILLI, VM_DECI>( args, "change_cutoff_sustain" );
    }

    // change_cutoff_release() function

    VMFnResult* InstrumentScriptVMFunction_change_cutoff_release::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::CutoffRelease),
                    &NoteBase::_Override::CutoffRelease,
                    Event::synth_param_cutoff_release,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed with 'seconds' unit */
                    0, NO_LIMIT, VM_MICRO>( args, "change_cutoff_release" );
    }

    // change_amp_lfo_depth() function

    VMFnResult* InstrumentScriptVMFunction_change_amp_lfo_depth::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::AmpLFODepth),
                    &NoteBase::_Override::AmpLFODepth,
                    Event::synth_param_amp_lfo_depth,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* not used (since this function does not accept unit) */
                    NO_LIMIT, NO_LIMIT, VM_NO_PREFIX>( args, "change_amp_lfo_depth" );
    }

    // change_amp_lfo_freq() function

    VMFnResult* InstrumentScriptVMFunction_change_amp_lfo_freq::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::AmpLFOFreq),
                    &NoteBase::_Override::AmpLFOFreq,
                    Event::synth_param_amp_lfo_freq,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed with 'Hz' unit */
                    0, 30000, VM_NO_PREFIX>( args, "change_amp_lfo_freq" );
    }

    // change_cutoff_lfo_depth() function

    VMFnResult* InstrumentScriptVMFunction_change_cutoff_lfo_depth::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::CutoffLFODepth),
                    &NoteBase::_Override::CutoffLFODepth,
                    Event::synth_param_cutoff_lfo_depth,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* not used (since this function does not accept unit) */
                    NO_LIMIT, NO_LIMIT, VM_NO_PREFIX>( args, "change_cutoff_lfo_depth" );
    }

    // change_cutoff_lfo_freq() function

    VMFnResult* InstrumentScriptVMFunction_change_cutoff_lfo_freq::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::CutoffLFOFreq),
                    &NoteBase::_Override::CutoffLFOFreq,
                    Event::synth_param_cutoff_lfo_freq,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed with 'Hz' unit */
                    0, 30000, VM_NO_PREFIX>( args, "change_cutoff_lfo_freq" );
    }

    // change_pitch_lfo_depth() function

    VMFnResult* InstrumentScriptVMFunction_change_pitch_lfo_depth::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::PitchLFODepth),
                    &NoteBase::_Override::PitchLFODepth,
                    Event::synth_param_pitch_lfo_depth,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* not used (since this function does not accept unit) */
                    NO_LIMIT, NO_LIMIT, VM_NO_PREFIX>( args, "change_pitch_lfo_depth" );
    }

    // change_pitch_lfo_freq() function

    VMFnResult* InstrumentScriptVMFunction_change_pitch_lfo_freq::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::PitchLFOFreq),
                    &NoteBase::_Override::PitchLFOFreq,
                    Event::synth_param_pitch_lfo_freq,
                    /* if value passed without unit */
                    0, NO_LIMIT, true,
                    /* if value passed with 'Hz' unit */
                    0, 30000, VM_NO_PREFIX>( args, "change_pitch_lfo_freq" );
    }

    // change_vol_time() function

    VMFnResult* InstrumentScriptVMFunction_change_vol_time::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::VolumeTime),
                    &NoteBase::_Override::VolumeTime,
                    Event::synth_param_volume_time,
                    /* if value passed without unit (implying 'us' unit) */
                    0, NO_LIMIT, true,
                    /* if value passed with 'seconds' unit */
                    0, NO_LIMIT, VM_MICRO>( args, "change_vol_time" );
    }

    // change_tune_time() function

    VMFnResult* InstrumentScriptVMFunction_change_tune_time::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::PitchTime),
                    &NoteBase::_Override::PitchTime,
                    Event::synth_param_pitch_time,
                    /* if value passed without unit (implying 'us' unit) */
                    0, NO_LIMIT, true,
                    /* if value passed with 'seconds' unit */
                    0, NO_LIMIT, VM_MICRO>( args, "change_tune_time" );
    }

    // change_pan_time() function

    VMFnResult* InstrumentScriptVMFunction_change_pan_time::exec(VMFnArgs* args) {
        return VMChangeSynthParamFunction::execTemplate<
                    decltype(NoteBase::_Override::PanTime),
                    &NoteBase::_Override::PanTime,
                    Event::synth_param_pan_time,
                    /* if value passed without unit (implying 'us' unit) */
                    0, NO_LIMIT, true,
                    /* if value passed with 'seconds' unit */
                    0, NO_LIMIT, VM_MICRO>( args, "change_pan_time" );
    }

    // template for change_*_curve() functions

    bool VMChangeFadeCurveFunction::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR;
    }

    template<fade_curve_t NoteBase::_Override::*T_noteParam, vmint T_synthParam>
    VMFnResult* VMChangeFadeCurveFunction::execTemplate(VMFnArgs* args, const char* functionName) {
        vmint value = args->arg(1)->asInt()->evalInt();
        switch (value) {
            case FADE_CURVE_LINEAR:
            case FADE_CURVE_EASE_IN_EASE_OUT:
                break;
            default:
                wrnMsg(String(functionName) + "(): invalid curve type passed as argument 2");
                return successResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg(String(functionName) + "(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg(String(functionName) + "(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if this change_*_curve() script function was called immediately after
            // note was triggered then immediately apply the synth parameter
            // change to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.*T_noteParam = (fade_curve_t) value;
            } else { // otherwise schedule this synth parameter change ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = (Event::synth_param_t) T_synthParam;
                e.Param.NoteSynthParam.Delta    = value;
                e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if this change_*_curve() script function was called immediately after
                // note was triggered then immediately apply the synth parameter
                // change to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.*T_noteParam = (fade_curve_t) value;
                } else { // otherwise schedule this synth parameter change ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = (Event::synth_param_t) T_synthParam;
                    e.Param.NoteSynthParam.Delta    = value;
                    e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
            }
        }

        return successResult();
    }

    // change_vol_curve() function

    VMFnResult* InstrumentScriptVMFunction_change_vol_curve::exec(VMFnArgs* args) {
        return VMChangeFadeCurveFunction::execTemplate<
                    &NoteBase::_Override::VolumeCurve,
                    Event::synth_param_volume_curve>( args, "change_vol_curve" );
    }

    // change_tune_curve() function

    VMFnResult* InstrumentScriptVMFunction_change_tune_curve::exec(VMFnArgs* args) {
        return VMChangeFadeCurveFunction::execTemplate<
                    &NoteBase::_Override::PitchCurve,
                    Event::synth_param_pitch_curve>( args, "change_tune_curve" );
    }

    // change_pan_curve() function

    VMFnResult* InstrumentScriptVMFunction_change_pan_curve::exec(VMFnArgs* args) {
        return VMChangeFadeCurveFunction::execTemplate<
        &NoteBase::_Override::PanCurve,
        Event::synth_param_pan_curve>( args, "change_pan_curve" );
    }

    // fade_in() function

    InstrumentScriptVMFunction_fade_in::InstrumentScriptVMFunction_fade_in(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_fade_in::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR || type == REAL_EXPR;
    }

    bool InstrumentScriptVMFunction_fade_in::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_fade_in::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
    }

    VMFnResult* InstrumentScriptVMFunction_fade_in::exec(VMFnArgs* args) {
        StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint duration =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MICRO) :
                args->arg(1)->asNumber()->evalCastInt();
        if (duration < 0) {
            wrnMsg("fade_in(): argument 2 may not be negative");
            duration = 0;
        }
        const float fDuration = float(duration) / 1000000.f; // convert microseconds to seconds

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("fade_in(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("fade_in(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if fade_in() was called immediately after note was triggered
            // then immediately apply a start volume of zero to Note object,
            // as well as the fade in duration
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.Volume.Value = 0.f;
                pNote->Override.VolumeTime = fDuration;
            } else { // otherwise schedule a "volume time" change with the requested fade in duration ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_volume_time;
                e.Param.NoteSynthParam.Delta    = fDuration;
                e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
            // and finally schedule a "volume" change, simply one time slice
            // ahead, with the final fade in volume (1.0)
            {
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_volume;
                e.Param.NoteSynthParam.Delta    = 1.f;
                e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                // scheduling with 0 delay would also work here, but +1 is more
                // safe regarding potential future implementation changes of the
                // scheduler (see API comments of RTAVLTree::insert())
                pEngineChannel->ScheduleEventMicroSec(&e, 1);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if fade_in() was called immediately after note was triggered
                // then immediately apply a start volume of zero to Note object,
                // as well as the fade in duration
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.Volume.Value = 0.f;
                    pNote->Override.VolumeTime = fDuration;
                } else { // otherwise schedule a "volume time" change with the requested fade in duration ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_volume_time;
                    e.Param.NoteSynthParam.Delta    = fDuration;
                    e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
                // and finally schedule a "volume" change, simply one time slice
                // ahead, with the final fade in volume (1.0)
                {
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_volume;
                    e.Param.NoteSynthParam.Delta    = 1.f;
                    e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                    // scheduling with 0 delay would also work here, but +1 is more
                    // safe regarding potential future implementation changes of the
                    // scheduler (see API comments of RTAVLTree::insert())
                    pEngineChannel->ScheduleEventMicroSec(&e, 1);
                }
            }
        }

        return successResult();
    }

    // fade_out() function

    InstrumentScriptVMFunction_fade_out::InstrumentScriptVMFunction_fade_out(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_fade_out::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR || type == INT_ARR_EXPR;
        else
            return type == INT_EXPR || type == REAL_EXPR;
    }

    bool InstrumentScriptVMFunction_fade_out::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_fade_out::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
    }

    VMFnResult* InstrumentScriptVMFunction_fade_out::exec(VMFnArgs* args) {
        StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        vmint duration =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MICRO) :
                args->arg(1)->asNumber()->evalCastInt();
        if (duration < 0) {
            wrnMsg("fade_out(): argument 2 may not be negative");
            duration = 0;
        }
        const float fDuration = float(duration) / 1000000.f; // convert microseconds to seconds

        bool stop = (args->argsCount() >= 3) ? (args->arg(2)->asInt()->evalInt() & 1) : true;

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (args->arg(0)->exprType() == INT_EXPR) {
            const ScriptID id = args->arg(0)->asInt()->evalInt();
            if (!id) {
                wrnMsg("fade_out(): note ID for argument 1 may not be zero");
                return successResult();
            }
            if (!id.isNoteID()) {
                wrnMsg("fade_out(): argument 1 is not a note ID");
                return successResult();
            }

            NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
            if (!pNote) return successResult();

            // if fade_out() was called immediately after note was triggered
            // then immediately apply fade out duration to Note object
            if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                pNote->Override.VolumeTime = fDuration;
            } else { // otherwise schedule a "volume time" change with the requested fade out duration ...
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_volume_time;
                e.Param.NoteSynthParam.Delta    = fDuration;
                e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                pEngineChannel->ScheduleEventMicroSec(&e, 0);
            }
            // now schedule a "volume" change, simply one time slice ahead, with
            // the final fade out volume (0.0)
            {
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_note_synth_param;
                e.Param.NoteSynthParam.NoteID   = id.noteID();
                e.Param.NoteSynthParam.Type     = Event::synth_param_volume;
                e.Param.NoteSynthParam.Delta    = 0.f;
                e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                // scheduling with 0 delay would also work here, but +1 is more
                // safe regarding potential future implementation changes of the
                // scheduler (see API comments of RTAVLTree::insert())
                pEngineChannel->ScheduleEventMicroSec(&e, 1);
            }
            // and finally if stopping the note was requested after the fade out
            // completed, then schedule to kill the voice after the requested
            // duration
            if (stop) {
                Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                e.Init(); // clear IDs
                e.Type = Event::type_kill_note;
                e.Param.Note.ID = id.noteID();
                e.Param.Note.Key = pNote->hostKey;

                pEngineChannel->ScheduleEventMicroSec(&e, duration + 1);
            }
        } else if (args->arg(0)->exprType() == INT_ARR_EXPR) {
            VMIntArrayExpr* ids = args->arg(0)->asIntArray();
            for (vmint i = 0; i < ids->arraySize(); ++i) {
                const ScriptID id = ids->evalIntElement(i);
                if (!id || !id.isNoteID()) continue;

                NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
                if (!pNote) continue;

                // if fade_out() was called immediately after note was triggered
                // then immediately apply fade out duration to Note object
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->Override.VolumeTime = fDuration;
                } else { // otherwise schedule a "volume time" change with the requested fade out duration ...
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_volume_time;
                    e.Param.NoteSynthParam.Delta    = fDuration;
                    e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                    pEngineChannel->ScheduleEventMicroSec(&e, 0);
                }
                // now schedule a "volume" change, simply one time slice ahead, with
                // the final fade out volume (0.0)
                {
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_note_synth_param;
                    e.Param.NoteSynthParam.NoteID   = id.noteID();
                    e.Param.NoteSynthParam.Type     = Event::synth_param_volume;
                    e.Param.NoteSynthParam.Delta    = 0.f;
                    e.Param.NoteSynthParam.Scope = Event::ValueScope::RELATIVE; // actually ignored

                    // scheduling with 0 delay would also work here, but +1 is more
                    // safe regarding potential future implementation changes of the
                    // scheduler (see API comments of RTAVLTree::insert())
                    pEngineChannel->ScheduleEventMicroSec(&e, 1);
                }
                // and finally if stopping the note was requested after the fade out
                // completed, then schedule to kill the voice after the requested
                // duration
                if (stop) {
                    Event e = m_vm->m_event->cause; // copy to get fragment time for "now"
                    e.Init(); // clear IDs
                    e.Type = Event::type_kill_note;
                    e.Param.Note.ID = id.noteID();
                    e.Param.Note.Key = pNote->hostKey;

                    pEngineChannel->ScheduleEventMicroSec(&e, duration + 1);
                }
            }
        }

        return successResult();
    }

    // get_event_par() function

    InstrumentScriptVMFunction_get_event_par::InstrumentScriptVMFunction_get_event_par(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_get_event_par::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        const ScriptID id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("get_event_par(): note ID for argument 1 may not be zero");
            return successResult(0);
        }
        if (!id.isNoteID()) {
            wrnMsg("get_event_par(): argument 1 is not a note ID");
            return successResult(0);
        }

        NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
        if (!pNote) {
            wrnMsg("get_event_par(): no note alive with that note ID of argument 1");
            return successResult(0);
        }

        const vmint parameter = args->arg(1)->asInt()->evalInt();
        switch (parameter) {
            case EVENT_PAR_NOTE:
                return successResult(pNote->cause.Param.Note.Key);
            case EVENT_PAR_VELOCITY:
                return successResult(pNote->cause.Param.Note.Velocity);
            case EVENT_PAR_VOLUME:
                return successResult(
                    RTMath::LinRatioToDecibel(pNote->Override.Volume.Value) * 1000.f
                );
            case EVENT_PAR_TUNE:
                return successResult(
                     RTMath::FreqRatioToCents(pNote->Override.Pitch.Value) * 1000.f
                );
            case EVENT_PAR_0:
                return successResult(pNote->userPar[0]);
            case EVENT_PAR_1:
                return successResult(pNote->userPar[1]);
            case EVENT_PAR_2:
                return successResult(pNote->userPar[2]);
            case EVENT_PAR_3:
                return successResult(pNote->userPar[3]);
        }

        wrnMsg("get_event_par(): argument 2 is an invalid event parameter");
        return successResult(0);
    }

    // set_event_par() function

    InstrumentScriptVMFunction_set_event_par::InstrumentScriptVMFunction_set_event_par(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_set_event_par::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        const ScriptID id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("set_event_par(): note ID for argument 1 may not be zero");
            return successResult();
        }
        if (!id.isNoteID()) {
            wrnMsg("set_event_par(): argument 1 is not a note ID");
            return successResult();
        }

        NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
        if (!pNote) return successResult();

        const vmint parameter = args->arg(1)->asInt()->evalInt();
        const vmint value     = args->arg(2)->asInt()->evalInt();

        switch (parameter) {
            case EVENT_PAR_NOTE:
                if (value < 0 || value > 127) {
                    wrnMsg("set_event_par(): note number of argument 3 is out of range");
                    return successResult();
                }
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->cause.Param.Note.Key = value;
                    m_vm->m_event->cause.Param.Note.Key = value;
                } else {
                    wrnMsg("set_event_par(): note number can only be changed when note is new");
                }
                return successResult();
            case EVENT_PAR_VELOCITY:
                if (value < 0 || value > 127) {
                    wrnMsg("set_event_par(): velocity of argument 3 is out of range");
                    return successResult();
                }
                if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
                    pNote->cause.Param.Note.Velocity = value;
                    m_vm->m_event->cause.Param.Note.Velocity = value;
                } else {
                    wrnMsg("set_event_par(): velocity can only be changed when note is new");
                }
                return successResult();
            case EVENT_PAR_VOLUME:
                wrnMsg("set_event_par(): changing volume by this function is currently not supported, use change_vol() instead");
                return successResult();
            case EVENT_PAR_TUNE:
                wrnMsg("set_event_par(): changing tune by this function is currently not supported, use change_tune() instead");
                return successResult();
            case EVENT_PAR_0:
                pNote->userPar[0] = value;
                return successResult();
            case EVENT_PAR_1:
                pNote->userPar[1] = value;
                return successResult();
            case EVENT_PAR_2:
                pNote->userPar[2] = value;
                return successResult();
            case EVENT_PAR_3:
                pNote->userPar[3] = value;
                return successResult();
        }

        wrnMsg("set_event_par(): argument 2 is an invalid event parameter");
        return successResult();
    }

    // change_note() function

    InstrumentScriptVMFunction_change_note::InstrumentScriptVMFunction_change_note(InstrumentScriptVM* parent)
    : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_change_note::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        const ScriptID id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("change_note(): note ID for argument 1 may not be zero");
            return successResult();
        }
        if (!id.isNoteID()) {
            wrnMsg("change_note(): argument 1 is not a note ID");
            return successResult();
        }

        NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
        if (!pNote) return successResult();

        const vmint value = args->arg(1)->asInt()->evalInt();
        if (value < 0 || value > 127) {
            wrnMsg("change_note(): note number of argument 2 is out of range");
            return successResult();
        }

        if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
            pNote->cause.Param.Note.Key = value;
            m_vm->m_event->cause.Param.Note.Key = value;
        } else {
            wrnMsg("change_note(): note number can only be changed when note is new");
        }

        return successResult();
    }

    // change_velo() function

    InstrumentScriptVMFunction_change_velo::InstrumentScriptVMFunction_change_velo(InstrumentScriptVM* parent)
    : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_change_velo::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        const ScriptID id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("change_velo(): note ID for argument 1 may not be zero");
            return successResult();
        }
        if (!id.isNoteID()) {
            wrnMsg("change_velo(): argument 1 is not a note ID");
            return successResult();
        }

        NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
        if (!pNote) return successResult();

        const vmint value = args->arg(1)->asInt()->evalInt();
        if (value < 0 || value > 127) {
            wrnMsg("change_velo(): velocity of argument 2 is out of range");
            return successResult();
        }

        if (m_vm->m_event->scheduleTime == pNote->triggerSchedTime) {
            pNote->cause.Param.Note.Velocity = value;
            m_vm->m_event->cause.Param.Note.Velocity = value;
        } else {
            wrnMsg("change_velo(): velocity can only be changed when note is new");
        }

        return successResult();
    }

    // change_play_pos() function

    InstrumentScriptVMFunction_change_play_pos::InstrumentScriptVMFunction_change_play_pos(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    bool InstrumentScriptVMFunction_change_play_pos::acceptsArgType(vmint iArg, ExprType_t type) const {
        if (iArg == 0)
            return type == INT_EXPR;
        else
            return type == INT_EXPR || type == REAL_EXPR;
    }

    bool InstrumentScriptVMFunction_change_play_pos::acceptsArgUnitType(vmint iArg, StdUnit_t type) const {
        if (iArg == 1)
            return type == VM_NO_UNIT || type == VM_SECOND;
        else
            return type == VM_NO_UNIT;
    }

    bool InstrumentScriptVMFunction_change_play_pos::acceptsArgUnitPrefix(vmint iArg, StdUnit_t type) const {
        return iArg == 1 && type == VM_SECOND; // only allow metric prefix(es) if 'seconds' is used as unit type
    }

    VMFnResult* InstrumentScriptVMFunction_change_play_pos::exec(VMFnArgs* args) {
        const ScriptID id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("change_play_pos(): note ID for argument 1 may not be zero");
            return successResult();
        }
        if (!id.isNoteID()) {
            wrnMsg("change_play_pos(): argument 1 is not a note ID");
            return successResult();
        }

        StdUnit_t unit = args->arg(1)->asNumber()->unitType();
        const vmint pos =
            (unit) ?
                args->arg(1)->asNumber()->evalCastInt(VM_MICRO) :
                args->arg(1)->asNumber()->evalCastInt();
        if (pos < 0) {
            wrnMsg("change_play_pos(): playback position of argument 2 may not be negative");
            return successResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
        if (!pNote) return successResult();

        pNote->Override.SampleOffset =
            (decltype(pNote->Override.SampleOffset)) pos;

        return successResult();
    }

    // event_status() function

    InstrumentScriptVMFunction_event_status::InstrumentScriptVMFunction_event_status(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_event_status::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        const ScriptID id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("event_status(): note ID for argument 1 may not be zero");
            return successResult(EVENT_STATUS_INACTIVE);
        }
        if (!id.isNoteID()) {
            wrnMsg("event_status(): argument 1 is not a note ID");
            return successResult(EVENT_STATUS_INACTIVE);
        }

        NoteBase* pNote = pEngineChannel->pEngine->NoteByID( id.noteID() );
        return successResult(pNote ? EVENT_STATUS_NOTE_QUEUE : EVENT_STATUS_INACTIVE);
    }

    // callback_status() function

    InstrumentScriptVMFunction_callback_status::InstrumentScriptVMFunction_callback_status(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_callback_status::exec(VMFnArgs* args) {
        const script_callback_id_t id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("callback_status(): callback ID for argument 1 may not be zero");
            return successResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        RTList<ScriptEvent>::Iterator itCallback = pEngineChannel->ScriptCallbackByID(id);
        if (!itCallback)
            return successResult(CALLBACK_STATUS_TERMINATED);

        return successResult(
            (m_vm->m_event->execCtx == itCallback->execCtx) ?
                CALLBACK_STATUS_RUNNING : CALLBACK_STATUS_QUEUE
        );
    }

    // wait() function (overrides core wait() implementation)

    InstrumentScriptVMFunction_wait::InstrumentScriptVMFunction_wait(InstrumentScriptVM* parent)
        : CoreVMFunction_wait(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_wait::exec(VMFnArgs* args) {
        InstrumentScriptVM* m_vm = (InstrumentScriptVM*) vm;

        // this might be set by passing 1 with the 2nd argument of built-in stop_wait() function
        if (m_vm->m_event->ignoreAllWaitCalls) return successResult();

        return CoreVMFunction_wait::exec(args);
    }

    // stop_wait() function

    InstrumentScriptVMFunction_stop_wait::InstrumentScriptVMFunction_stop_wait(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_stop_wait::exec(VMFnArgs* args) {
        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        const script_callback_id_t id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("stop_wait(): callback ID for argument 1 may not be zero");
            return successResult();
        }

        RTList<ScriptEvent>::Iterator itCallback = pEngineChannel->ScriptCallbackByID(id);
        if (!itCallback) return successResult(); // ignore if callback is i.e. not alive anymore

        const bool disableWaitForever =
            (args->argsCount() >= 2) ? (args->arg(1)->asInt()->evalInt() == 1) : false;

        pEngineChannel->ScheduleResumeOfScriptCallback(
            itCallback, m_vm->m_event->scheduleTime, disableWaitForever
        );

        return successResult();
    }

    // abort() function

    InstrumentScriptVMFunction_abort::InstrumentScriptVMFunction_abort(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_abort::exec(VMFnArgs* args) {
        const script_callback_id_t id = args->arg(0)->asInt()->evalInt();
        if (!id) {
            wrnMsg("abort(): callback ID for argument 1 may not be zero");
            return successResult();
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        RTList<ScriptEvent>::Iterator itCallback = pEngineChannel->ScriptCallbackByID(id);
        if (!itCallback) return successResult(); // ignore if callback is i.e. not alive anymore

        itCallback->execCtx->signalAbort();

        return successResult();
    }

    // fork() function

    InstrumentScriptVMFunction_fork::InstrumentScriptVMFunction_fork(InstrumentScriptVM* parent)
        : m_vm(parent)
    {
    }

    VMFnResult* InstrumentScriptVMFunction_fork::exec(VMFnArgs* args) {
        // check if this is actually the parent going to fork, or rather one of
        // the children which is already forked
        if (m_vm->m_event->forkIndex != 0) { // this is the entry point for a child ...
            int forkResult = m_vm->m_event->forkIndex;
            // reset so that this child may i.e. also call fork() later on
            m_vm->m_event->forkIndex = 0;
            return successResult(forkResult);
        }

        // if we are here, then this is the parent, so we must fork this parent

        const vmint n =
            (args->argsCount() >= 1) ? args->arg(0)->asInt()->evalInt() : 1;
        const bool bAutoAbort =
            (args->argsCount() >= 2) ? args->arg(1)->asInt()->evalInt() : true;

        if (m_vm->m_event->countChildHandlers() + n > MAX_FORK_PER_SCRIPT_HANDLER) {
            wrnMsg("fork(): requested amount would exceed allowed limit per event handler");
            return successResult(-1);
        }

        AbstractEngineChannel* pEngineChannel =
            static_cast<AbstractEngineChannel*>(m_vm->m_event->cause.pEngineChannel);

        if (!pEngineChannel->hasFreeScriptCallbacks(n)) {
            wrnMsg("fork(): global limit of event handlers exceeded");
            return successResult(-1);
        }

        for (int iChild = 0; iChild < n; ++iChild) {
            RTList<ScriptEvent>::Iterator itChild =
                pEngineChannel->forkScriptCallback(m_vm->m_event, bAutoAbort);
            if (!itChild) { // should never happen, otherwise its a bug ...
                errMsg("fork(): internal error while allocating child");
                return errorResult(-1); // terminate script
            }
            // since both parent, as well all child script execution instances
            // all land in this exec() method, the following is (more or less)
            // the only feature that lets us distinguish the parent and
            // respective children from each other in this exec() method
            itChild->forkIndex = iChild + 1;
        }

        return successResult(0);
    }

} // namespace LinuxSampler
