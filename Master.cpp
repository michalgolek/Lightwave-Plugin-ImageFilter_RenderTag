#include "Main.h"
#include <mmsystem.h>


#pragma comment(lib, "Winmm.lib")

static GlobalFunc *g_pGlobalFuncs=NULL;
static LWMessageFuncs *lwMsgFunc=NULL;
static LWPanelFuncs *lwPanelFuncs=NULL;
static LWPanelID lwPanelID=NULL;
static LWSceneInfo *lwSceneInfo=NULL;
static LWInterfaceInfo *lwInterfaceInfo=NULL;
static LWCommandFunc *lwCommandFunc=NULL;

double masterStartFrameTime=0.0;

double masterRenderTime=0.0;

extern bool pluginImgFilterCreated;
extern bool pluginMasterCreated;
bool loadedFromScene=false;

int curSecond=0, curMinute=0, curHour=0;

XCALL_( LWInstance ) Master_Create( void *priv, void *id, LWError *err )
{
	if(pluginMasterCreated)
		return NULL;
	
	pluginMasterCreated=true;

// 	if(!pluginImgFilterCreated /*&& !loadedFromScene*/)
// 	{
// 		char temp[255]={0};
// 		sprintf_s(temp, "ApplyServer %s %s", "ImageFilterHandler", PLUGIN_IMG_FILTER_NAME);
// 		lwCommandFunc(temp);
// 
// 		pluginImgFilterCreated=true;
// 	}

	return ( LWInstance ) PLUGIN_MASTER_NAME;
}


XCALL_( void ) Master_Destroy ( LWInstance inst )
{
	if(pluginImgFilterCreated)
	{
		//pluginImgFilterCreated=false; 
		//char temp[255]={0};
		//sprintf_s(temp, "RemoveServer %s %i", "ImageFilterHandler", 1);
		//lwCommandFunc(temp);
	}

	pluginMasterCreated=false;
}

XCALL_( LWError ) Master_Copy ( LWInstance to, LWInstance from )
{
	return NULL;
}

XCALL_( LWError ) Master_Load( LWInstance inst, const LWLoadState *state)
{
	return NULL;
}

XCALL_( LWError ) Master_Save( LWInstance inst, const LWSaveState *state)
{
	return NULL;
}

XCALL_(double) Master_Event(LWInstance inst, const LWMasterAccess *ma)
{
// 	if(ma->eventCode==LWEVNT_NOTIFY_SCENE_LOAD_STARTING)
// 	{
// 		loadedFromScene=true;
// 	}
// 
// 	if(ma->eventCode==LWEVNT_NOTIFY_SCENE_LOAD_COMPLETE)
// 	{
// 		loadedFromScene=false;
// 	}

	if(ma->eventCode==LWEVNT_NOTIFY_SCENE_CLEAR_COMPLETE )
		lwMsgFunc->info("clear scene complete", 0);

	if(ma->eventCode==LWEVNT_NOTIFY_SCENE_LOAD_STARTING)
	{
		int s=0;
	}
	

	if(ma->eventCode==LWEVNT_NOTIFY_RENDER_FRAME_STARTING)
	{	
		//lwMsgFunc->info("start", 0);
		masterStartFrameTime=timeGetTime();
	}

	if(ma->eventCode==LWEVNT_NOTIFY_RENDER_FRAME_COMPLETE)
	{
		double masterCompleteFrameTime=timeGetTime();
		masterRenderTime=masterCompleteFrameTime-masterStartFrameTime;
		masterRenderTime/=1000;		
	}

	if(ma->eventCode==LWEVNT_NOTIFY_PLUGIN_CHANGED)
	{
		LWEventNotifyPluginChange *evtNotifyPluginChange=(LWEventNotifyPluginChange*) ma->eventData;

		if(evtNotifyPluginChange->pluginevent==LWEVNT_PLUGIN_CREATED)
		{
			if(!pluginMasterCreated && pluginImgFilterCreated)
			{
// 				char temp[255]={0};
// 				sprintf_s(temp, "ApplyServer %s %s", "MasterHandler", PLUGIN_MASTER_NAME);
// 				lwCommandFunc(temp);
// 
// 				pluginMasterCreated=true;

				//

				// przenosimy Mig_RenderTag na koniec listy
				//lwMsgFunc->info("LWEVNT_NOTIFY_PLUGIN_CHANGED", 0);

				//ApplyServer 
			}
		}	

		if(evtNotifyPluginChange->pluginevent==LWEVNT_PLUGIN_DESTROYED)
		{
		}
	}

	return 0.0;
}

XCALL_( static unsigned int ) Master_Flags(LWInstance inst)
{
	return LWMASTF_RECEIVE_NOTIFICATIONS;
}

XCALL_( static LWError ) Master_Options(void *data)
{
	//UI_ExportCallback();

	return NULL;
}

XCALL_( int ) MasterActivation(int version, GlobalFunc *global,
								 LWMasterHandler *local, void *serverData)
{	
	//if ( version != LWCUSTOMOBJ_VERSION )
	//	return AFUNC_BADVERSION;

	g_pGlobalFuncs=global;

	lwPanelFuncs = (LWPanelFuncs *) global( LWPANELFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwPanelFuncs ) return AFUNC_BADGLOBAL;

	lwMsgFunc = (LWMessageFuncs *) global( LWMESSAGEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwMsgFunc ) return AFUNC_BADGLOBAL;

	lwSceneInfo = (LWSceneInfo *) global( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwSceneInfo ) return AFUNC_BADGLOBAL;

	lwInterfaceInfo = (LWInterfaceInfo *) global( LWINTERFACEINFO_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwInterfaceInfo ) return AFUNC_BADGLOBAL;

	lwCommandFunc = (LWCommandFunc*) global( LWCOMMANDFUNC_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwCommandFunc) return AFUNC_BADGLOBAL;

	local->inst->create  = Master_Create;
	local->inst->destroy = Master_Destroy;
	local->inst->copy    = Master_Copy;
	local->inst->load    = Master_Load;
	local->inst->save    = Master_Save;
	local->inst->descln  = NULL;

	if ( local->item )
	{
		local->item->useItems = NULL;
		local->item->changeID = NULL;
	}

	local->event = Master_Event;
	local->flags = Master_Flags;

	return AFUNC_OK;
}

XCALL_( int ) MasterInterface( int version, GlobalFunc *global,
								LWInterface *local, void *serverData )
{
	if ( version != LWINTERFACE_VERSION )
		return AFUNC_BADVERSION;

	local->panel   = NULL;
	local->options = NULL;/*Exporter_Options*/;
	local->command = NULL;

	return AFUNC_OK;
}