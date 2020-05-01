#pragma once

#define _USE_MATH_DEFINES
#define _CRT_NONSTDC_NO_WARNINGS
#pragma warning(disable: 4290 4250 4018 4244 4576 4065 5040)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#define NOMINMAX
#define __STDC_VERSION__ __cplusplus

#include <windows.h>
#undef RELATIVE

#include <type_traits>
#include <algorithm>
#include <strings.h>
#include <stdlib.h>
#include <math.h>
#include <io.h>
#include <cmath>
#include <string>
#include <atomic>

#include <unistd.h>
#include <sys/time.h>
#include "config.h"

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
#define bcopy(b1,b2,len) (memmove((b2), (b1), (len)), (void) 0)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define typeof(x) decltype(x)

#define ssize_t ptrdiff_t
#define atomic_t std::atomic_uint32_t

#ifndef _DEBUG
#include <../src/common/global.h>
#include <../src/common/global_private.h>
#include <../src/common/optional.h>
#include <../src/common/File.h>
#include <../src/common/SynchronizedConfig.h>
#include <../src/common/RTMath.h>
#include <../src/common/atomic.h>

#include <../src/engines/AbstractEngine.h>
#include <../src/engines/AbstractEngineChannel.h>
#include <gig.h>

#define GIT_BUILD() repoBuildNumber()
String repoBuildNumber();


#endif