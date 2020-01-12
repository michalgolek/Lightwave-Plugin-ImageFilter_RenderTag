#ifndef _MAIN_H
#define _MAIN_H

// Declare this in header.
//http://weseetips.com/2008/06/17/how-to-detect-memory-leaks-by-using-crt/
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

extern "C"
{
	#include <lwhost.h>
	#include <lwmaster.h>
	#include <lwserver.h>
	#include <lwfilter.h>
	#include <lwmonitor.h>
	#include <lwdisplay.h>
	#include <lwpanel.h>
	#include <lwframbuf.h>
	#include <lwmodeler.h>
	#include <lwcmdseq.h>
}

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define LWCOMMANDFUNC_GLOBAL "LW Command Interface"
typedef int LWCommandFunc ( const char *cmd );

#define PLUGIN_IMG_FILTER_NAME "MiG_RenderTag"
#define PLUGIN_MASTER_NAME "MiG_RenderTagM"

XCALL_( int ) MasterActivation(int version, GlobalFunc *global, LWMasterHandler *local, void *serverData);
XCALL_( int ) MasterInterface(int version, GlobalFunc *global, LWInterface *local, void *serverData);

#endif