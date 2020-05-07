#include "stdafx.h"
#include "../src/common/global.h"

#define BUILD_IDENT "$Id$"

String repoBuildNumber()
{
   return String(BUILD_IDENT).substr(5, 40);
}

#if 0
#include "cflexasio.h"
#pragma comment(lib, "FlexASIO_cflexasio")
#pragma comment(lib, "FlexASIO_flexasio")
#pragma comment(lib, "FlexASIO_config")
#pragma comment(lib, "FlexASIO_portaudio")
#pragma comment(lib, "FlexASIOutil_portaudio")
#pragma comment(lib, "FlexASIOutil_shell")
#pragma comment(lib, "FlexASIO_log")
#pragma comment(lib, "portaudio_static_x64")
#pragma comment(lib, "dechamps_CMakeUtils_version")
#pragma comment(lib, "asio")
#pragma comment(lib, "log")
void proba()
{
    auto x = CreateFlexASIO();
}
#endif