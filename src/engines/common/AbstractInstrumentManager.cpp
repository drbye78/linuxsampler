/*
 * Copyright (c) 2014 - 2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "AbstractInstrumentManager.h"
#include "../AbstractEngine.h"
#include "../AbstractEngineChannel.h"

namespace LinuxSampler {

    VMParserContext* AbstractInstrumentManager::ScriptResourceManager::Create(ScriptKey key, InstrumentScriptConsumer* pConsumer, void*& pArg) {
        AbstractEngineChannel* pEngineChannel = dynamic_cast<AbstractEngineChannel*>(pConsumer);
        return pEngineChannel->pEngine->pScriptVM->loadScript(key.code, key.patchVars);
    }

    void AbstractInstrumentManager::ScriptResourceManager::Destroy(VMParserContext* pResource, void* pArg) {
        delete pResource;
    }

} // namespace LinuxSampler
