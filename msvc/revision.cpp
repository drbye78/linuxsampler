#include "stdafx.h"

#define BUILD_IDENT "$Id$"

String repoBuildNumber()
{
   return String(BUILD_IDENT).substr(5, 40);
}