/*
 * Copyright (c) 2014-2020 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_ABSTRACTINSTRUMENTMANAGER_H
#define LS_ABSTRACTINSTRUMENTMANAGER_H

#include "../InstrumentManager.h"
#include "../../common/ResourceManager.h"
#include "../../common/global_private.h"
#include "InstrumentScriptVM.h"

namespace LinuxSampler {

    typedef ResourceConsumer<VMParserContext> InstrumentScriptConsumer;

    /// Identifies uniquely a compiled script.
    struct ScriptKey {
        String code; ///< Script's source code.
        std::map<String,String> patchVars; ///< Patch variables being overridden by instrument.
        bool wildcardPatchVars; ///< Seldom use: Allows lookup for consumers of a specific script by ignoring all (overridden) patch variables.

        inline bool operator<(const ScriptKey& o) const {
            if (wildcardPatchVars)
                return code < o.code;
            else
                return code < o.code || (code == o.code && patchVars < o.patchVars);
        }

        inline bool operator>(const ScriptKey& o) const {
            if (wildcardPatchVars)
                return code > o.code;
            else
                return code > o.code || (code == o.code && patchVars > o.patchVars);
        }

        inline bool operator==(const ScriptKey& o) const {
            if (wildcardPatchVars)
                return code == o.code;
            else
                return code == o.code && patchVars == o.patchVars;
        }

        inline bool operator!=(const ScriptKey& o) const {
            return !(operator==(o));
        }
    };

    class AbstractInstrumentManager : public InstrumentManager {
    public:
        AbstractInstrumentManager() { }
        virtual ~AbstractInstrumentManager() { }

        /**
         * Resource manager for loading and sharing the parsed (executable) VM
         * presentation of real-time instrument scripts. The key used here, and
         * associated with each script resource, is not as one might expect the
         * script name or something equivalent, instead the key used is
         * actually the entire script's source code text (and additionally
         * potentially patched variables). The value (the actual resource) is of
         * type @c VMParserContext, which is the parsed (executable) VM
         * representation of the respective script.
         */
        class ScriptResourceManager : public ResourceManager<ScriptKey, VMParserContext> {
        public:
            ScriptResourceManager() {}
            virtual ~ScriptResourceManager() {}
        protected:
            // implementation of derived abstract methods from 'ResourceManager'
            virtual VMParserContext* Create(ScriptKey key, InstrumentScriptConsumer* pConsumer, void*& pArg);
            virtual void Destroy(VMParserContext* pResource, void* pArg);
            virtual void OnBorrow(VMParserContext* pResource, InstrumentScriptConsumer* pConsumer, void*& pArg) {} // ignore
        } scripts;
    };
    
} // namespace LinuxSampler

#endif // LS_ABSTRACTINSTRUMENTMANAGER_H
