/*
 * Copyright (c) 2016 - 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "CoreVMDynVars.h"

#include "tree.h"
#include "ScriptVM.h"
#include "../common/RTMath.h"

namespace LinuxSampler {

vmint CoreVMDynVar_NKSP_REAL_TIMER::evalInt() {
    return (vmint) RTMath::unsafeMicroSeconds(RTMath::real_clock);
}

vmint CoreVMDynVar_NKSP_PERF_TIMER::evalInt() {
    return (vmint) RTMath::unsafeMicroSeconds(RTMath::thread_clock);
}

} // namespace LinuxSampler
