#include "stdafx.h"
#include "../src/common/global.h"

#define BUILD_IDENT "$Id$"

String repoBuildNumber()
{
   return String(BUILD_IDENT).substr(5, 40);
}