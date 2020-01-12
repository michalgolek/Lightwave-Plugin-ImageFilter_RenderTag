#include "Main.h"
#include <SimpleIni.h>

#include <cmath>

HostDisplayInfo *hostDisplayInfo=NULL;
LWDirInfoFunc *lwDirInfoFunc=NULL;
LWSceneInfo *sceneInfo=NULL;
LWMessageFuncs *lwMsgFunc=NULL;
LWPanelFuncs *lwPanelFuncs=NULL;
LWFontListFuncs *lwFontListFunc=NULL;
LWTimeInfo *lwTimeInfo=NULL;
ContextMenuFuncs *contextMenuFuncs=NULL;
LWCommandFunc *lwCommandFunc=NULL;
LWColorActivateFunc *lwColorActivateFunc=NULL;

//////////////////////////////////////////////////////////////////////////
enum EControlsList
{
	RS_BK_COLOR,
	RS_TEXT_COLOR,
	RS_TEXT_FONT,
	RS_TEXT_SIZE,
	RS_TEXT_BOLD,
	RS_TEXT_ITALIC,
	RS_TEXT_UNDERLINE,
	RS_TEXT_HALIGN,
	RS_TEXT_VALIGN,
	RS_TEXT_HSPACE,
	RS_TEXT_VSPACE,
	RS_COMMAND_LINE,
	RS_COMMANDS,
	RS_PREVIEW,
	RS_LOAD,
	RS_SAVE,
	RS_ABOUT
};

LWPanelID lwPanelID;
LWControl *controls[RS_ABOUT];

LWPanControlDesc desc;

//////////////////////////////////////////////////////////////////////////

SIZE textAreaSize;

unsigned char *buffer=NULL;

void CalculateTextSize(HWND hwnd, int fontSize, int bold, int italic, int underline, char *str, std::string fontname, int w, int h, SIZE &finalTextSize);

// globalne
struct SInstanceData
{
	int textAreaBkColor[3];
	int textColor[3];
	std::string textFontName;
	int textSize;
	int textBold;
	int textItalic;
	int textUnderline;
	int textHAlign;
	int textVAlign;
	int textHSpace;
	int textVSpace;
	char strCommandLine[255];

	std::string strParsedCommandLine;
};

SInstanceData instanceData={{0,0,0}, {255,255,255}, "Arial", 12, 0, 0, 0, 2, 1, 5, 5, "", ""};
//

static LWValue
ival    = { LWT_INTEGER },
ivecval = { LWT_VINT },
fval    = { LWT_FLOAT },
fvecval = { LWT_VFLOAT },
sval    = { LWT_STRING };

std::vector<std::string> fontsLists;

extern double masterRenderTime;
extern double masterStartFrameTime;

LWContextMenuID menu;

typedef struct st_MyMenuData
{
	int    count;
	char **name;
} MyMenuData;

char *cmdList[] = {"$RENDERTIME$", "$DATE$", "$RENDERSIZE$", /*"$ANTIALIASING$",*/ NULL };
char *cmdListWithoutTokens[] = {"RENDERTIME", "DATE", "RENDERSIZE", /*"ANTIALIASING"*/};

MyMenuData menudata = { 3, cmdList };

bool pluginImgFilterCreated=false;
bool pluginMasterCreated=false;

bool SaveBMP(char *filename,int width,int height,unsigned char *sdata)
{
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;

	FILE *pFile=fopen(filename,"wb");
	if(!pFile)
		return false;

	bitmapFileHeader.bfSize		= sizeof(BITMAPFILEHEADER);
	bitmapFileHeader.bfType		= 0x4D42;
	bitmapFileHeader.bfReserved1= 0;
	bitmapFileHeader.bfReserved2= 0;
	bitmapFileHeader.bfOffBits  = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

	bitmapInfoHeader.biSize			= sizeof(BITMAPINFOHEADER);
	bitmapInfoHeader.biPlanes		= 1;
	bitmapInfoHeader.biBitCount		= 24;
	bitmapInfoHeader.biCompression	= BI_RGB;
	bitmapInfoHeader.biSizeImage	= width*height*3;
	bitmapInfoHeader.biXPelsPerMeter= 0;
	bitmapInfoHeader.biYPelsPerMeter= 0;
	bitmapInfoHeader.biClrUsed		= 0;
	bitmapInfoHeader.biClrImportant = 0;
	bitmapInfoHeader.biWidth		= width;
	bitmapInfoHeader.biHeight		= height;

	fwrite(&bitmapFileHeader,1,sizeof(BITMAPFILEHEADER),pFile);

	fwrite(&bitmapInfoHeader,1,sizeof(BITMAPINFOHEADER),pFile);

	fwrite(sdata,1, bitmapInfoHeader.biSizeImage,pFile);

	fclose(pFile);

	return true;
}

void RenderFont(HWND hwnd, int x, int y, int fontSize, int bold, int italic, int underline, std::string str, std::string fontname,int w, int h)
{
	buffer=new unsigned char[w*h*3];

	HDC hdc=GetWindowDC(hwnd);
	if(!hdc)return;

	HDC mdc = CreateCompatibleDC(hdc);
	if(!mdc)return;

	HBITMAP bm = CreateCompatibleBitmap(hdc,w,h);     
	if(!bm)return;

	HFONT hf=CreateFont(fontSize, 
		0, 
		0, 
		0, 
		bold==0?FW_NORMAL:FW_BOLD, 
		italic, 
		underline, 
		FALSE,
		DEFAULT_CHARSET , 
		OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY, 
		FF_DONTCARE|DEFAULT_PITCH, 
		fontname.c_str());        
	if(!hf)
	{
		DeleteObject(bm);    
		return;
	}

	RECT rc;
	if(instanceData.textHAlign==0)
	{
		rc.left=instanceData.textHSpace;
		rc.right=w;
	}
	else
		if(instanceData.textHAlign==2)
		{
			rc.left=0;
			rc.right=w-instanceData.textHSpace;
		}
		else
		{
			rc.left=0;
			rc.right=w;
		}

	if(instanceData.textVAlign==0)
	{
		rc.top=0;
		rc.bottom=h-instanceData.textVSpace;
	}
	else
		if(instanceData.textVAlign==2)
		{
			rc.top=instanceData.textVSpace;
			rc.bottom=h;

			
		}
		else
			{
				rc.top=0;
				rc.bottom=h;
			}

	SelectObject(mdc, bm);
	RECT rcFill;
	rcFill.left=0;
	rcFill.top=0;
	rcFill.right=w;
	rcFill.bottom=h;
	FillRect(mdc, &rcFill, (HBRUSH)CreateSolidBrush(RGB(instanceData.textAreaBkColor[0], instanceData.textAreaBkColor[1], instanceData.textAreaBkColor[2])));

	SelectObject(mdc, hf);      
	SetBkMode(mdc, OPAQUE);
	SetBkColor(mdc, RGB(instanceData.textAreaBkColor[0], instanceData.textAreaBkColor[1], instanceData.textAreaBkColor[2]));
	SetTextColor(mdc, RGB(instanceData.textColor[0], instanceData.textColor[1], instanceData.textColor[2]));

 	switch(instanceData.textHAlign)
 	{
 	case 0:
 		switch(instanceData.textVAlign)
 		{
 		case 0:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_LEFT|DT_TOP|DT_SINGLELINE);
 			break;
 		case 1:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE);
 			break;
 		case 2:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_LEFT|DT_BOTTOM|DT_SINGLELINE);
 			break;
 		}	
 		break;
 	case 1:
 		switch(instanceData.textVAlign)
 		{
 		case 0:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_CENTER|DT_TOP|DT_SINGLELINE);
 			break;
 		case 1:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 			break;
 		case 2:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_CENTER|DT_BOTTOM|DT_SINGLELINE);
 			break;
 		}
 		break;
 	case 2:
 		switch(instanceData.textVAlign)
 		{
 		case 0:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_RIGHT|DT_TOP|DT_SINGLELINE);
 			break;
 		case 1:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
 			break;
 		case 2:
 			DrawText(mdc, (char*) str.c_str(), str.size(), &rc, DT_RIGHT|DT_BOTTOM|DT_SINGLELINE);
 			break;
 		}
 		break;
 	}

	BITMAPINFO bmi;   
	ZeroMemory(&bmi, sizeof(BITMAPINFO));

	bmi.bmiHeader.biSize=sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth=w;
	bmi.bmiHeader.biHeight=h;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=24;
	bmi.bmiHeader.biCompression=BI_RGB;

	GetDIBits(mdc, bm, 0, h, buffer, &bmi, DIB_RGB_COLORS);

	DeleteObject(hf);
	DeleteObject(bm);  

	DeleteDC(mdc);
	ReleaseDC(hwnd, hdc);
}

void CalculateTextSize(HWND hwnd, int fontSize, int bold, int italic, int underline, std::string str, std::string fontname, int w, int h, SIZE &finalTextSize)
{
	HDC hdc=GetWindowDC(hwnd);
	if(!hdc)return;

	HDC mdc = CreateCompatibleDC(hdc);
	if(!mdc)return;

	HBITMAP bm = CreateCompatibleBitmap(hdc,w ,h);     
	if(!bm)return;

	HFONT hf=CreateFont(fontSize, 
		0, 
		0, 
		0, 
		bold==0?FW_NORMAL:FW_BOLD, 
		italic, 
		underline, 
		FALSE,
		DEFAULT_CHARSET , 
		OUT_DEFAULT_PRECIS, 
		CLIP_DEFAULT_PRECIS, 
		ANTIALIASED_QUALITY, 
		FF_DONTCARE|DEFAULT_PITCH, 
		fontname.c_str());         
	if(!hf)
	{
		DeleteObject(bm);
		DeleteDC(mdc);
		ReleaseDC(hwnd, hdc);
		return;
	}

	SelectObject(mdc, hf);  
	GetTextExtentPoint32(mdc, str.c_str(), str.size(), &finalTextSize);

	DeleteObject(hf);
	DeleteObject(bm);

	DeleteDC(mdc);
	ReleaseDC(hwnd, hdc);
}

XCALL_( LWInstance ) RenderTag_Create( void *priv, void *context, LWError *err )
{
	if(pluginImgFilterCreated)
		return NULL;

	pluginImgFilterCreated=true;

	if(!pluginMasterCreated)
	{
		char temp[255]={0};
		sprintf_s(temp, "ApplyServer %s %s", "MasterHandler", PLUGIN_MASTER_NAME);
		lwCommandFunc(temp);

		pluginMasterCreated=true;
	}

	return ( LWInstance ) PLUGIN_IMG_FILTER_NAME;
}


XCALL_( void ) RenderTag_Destroy ( LWInstance inst )
{
	instanceData.textAreaBkColor[0]=instanceData.textAreaBkColor[1]=instanceData.textAreaBkColor[2]=0;
	instanceData.textColor[0]=instanceData.textColor[1]=instanceData.textColor[2]=255;
	instanceData.textFontName=fontsLists[0];
	instanceData.textSize=12;
	instanceData.textBold=0;
	instanceData.textItalic=0;
	instanceData.textUnderline=0;
	instanceData.textHAlign=2;
	instanceData.textVAlign=1;
	instanceData.textHSpace=5;
	instanceData.textVSpace=5;
	strcpy(instanceData.strCommandLine, "");

	pluginImgFilterCreated=false;

	if(pluginMasterCreated)
	{
		//pluginMasterCreated=false; 
 		//char temp[255]={0};
 		//sprintf_s(temp, "RemoveServer %s %i", "MasterHandler", 1);
 		//lwCommandFunc(temp);
	}
}

XCALL_( LWError ) RenderTag_Copy ( LWInstance to, LWInstance from )
{
	return NULL;
}

XCALL_( LWError ) RenderTag_Load( LWInstance to, const LWLoadState *state)
{
	char temp[255]={0};

	LWLOAD_I4( state, instanceData.textAreaBkColor, 3);
	
	LWLOAD_I4( state, instanceData.textColor, 3);

	LWLOAD_STR( state, temp, 255);
	instanceData.textFontName=std::string(temp);

	LWLOAD_I4( state, &instanceData.textSize, 1);

	LWLOAD_I4( state, &instanceData.textBold, 1);

	LWLOAD_I4( state, &instanceData.textItalic, 1);

	LWLOAD_I4( state, &instanceData.textUnderline, 1);

	LWLOAD_I4( state, &instanceData.textHAlign, 1);

	LWLOAD_I4( state, &instanceData.textVAlign, 1);

	LWLOAD_I4( state, &instanceData.textHSpace, 1);

	LWLOAD_I4( state, &instanceData.textVSpace, 1);

	LWLOAD_STR( state, instanceData.strCommandLine, 255);

	return NULL;
}

XCALL_( LWError ) RenderTag_Save( LWInstance to, const LWSaveState *state)
{
	char temp[255]={0};

	LWSAVE_I4( state, instanceData.textAreaBkColor, 3);

	LWSAVE_I4( state, instanceData.textColor, 3);

	strcpy(temp, instanceData.textFontName.c_str());
	LWSAVE_STR( state, temp);

	LWSAVE_I4( state, &instanceData.textSize, 1 );

	LWSAVE_I4( state, &instanceData.textBold, 1);

	LWSAVE_I4( state, &instanceData.textItalic, 1);

	LWSAVE_I4( state, &instanceData.textUnderline, 1);

	LWSAVE_I4( state, &instanceData.textHAlign, 1);

	LWSAVE_I4( state, &instanceData.textVAlign, 1);

	LWSAVE_I4( state, &instanceData.textHSpace, 1);

	LWSAVE_I4( state, &instanceData.textVSpace, 1);

	LWSAVE_STR( state, instanceData.strCommandLine);

	return NULL;
}

XCALL_( static unsigned int )RenderTag_Flags( LWInstance inst )
{	
	return 0;
}

void ParseCommandLine()
{
	int curCharacter=0, variableCharacters=0, skipChars=0;

	masterRenderTime=(timeGetTime()-masterStartFrameTime)/1000.0;

	instanceData.strParsedCommandLine="";
	curCharacter=0; 
	variableCharacters=0;
	skipChars=0;

	while(curCharacter<strlen(instanceData.strCommandLine))
	{
		if(instanceData.strCommandLine[curCharacter]=='$')
		{
			variableCharacters=curCharacter+1;

			//parseVariable=true;

			std::string variable;
			bool variableHasEnd=false;
			while(variableCharacters<strlen(instanceData.strCommandLine))
			{
				if(instanceData.strCommandLine[variableCharacters]=='$')
				{
					variableHasEnd=true;
					break;
				}

				variable+=instanceData.strCommandLine[variableCharacters];

				variableCharacters++;
			}

			char strTemp[255]={0};

			if(variableHasEnd) //  jesli znaleziono koniec zmiennej - $ to sprawdz jaki ma typ i dodaj do wyjsciowego stringa
			{
				if(!strcmp(variable.c_str(), cmdListWithoutTokens[0]))
				{


					if(masterRenderTime<1)
					{
						sprintf(strTemp, "%.1fs", masterRenderTime);
					}
					else
					{
						int curSecond=0, curMinute=0, curHour=0;

						masterRenderTime+=0.5;

						curSecond= (int)masterRenderTime % 60;
						curMinute= (int)floor( (double)(((int)masterRenderTime % 3600 ) / 60));
						curHour= (int)floor((double)(int)masterRenderTime / (60 * 60));

						char secStr[5], minStr[5],  hourStr[5];

						sprintf_s(secStr, "%i", (int)curSecond);

						sprintf_s(minStr, "%i", (int) curMinute);

						sprintf_s(hourStr, "%i", (int) curHour);

						if(curHour==0 && curMinute==0)
							sprintf(strTemp, "%ss (%i seconds)", secStr, (int)masterRenderTime);
						else
							if(curHour==0 && curMinute>0)
								sprintf(strTemp, "%sm %ss (%i seconds)", minStr, secStr, (int)masterRenderTime);
							else
								if(curHour>0 && curMinute>0)
									sprintf(strTemp, "%sh %sm %ss (%i seconds)", hourStr, minStr, secStr, (int)masterRenderTime);
					}


					instanceData.strParsedCommandLine+=strTemp;
				}

				if(!strcmp(variable.c_str(), cmdListWithoutTokens[1])) // date
				{
					SYSTEMTIME sysTime;
					GetSystemTime(&sysTime);

					sprintf(strTemp, "%i.%i.%i", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
					instanceData.strParsedCommandLine+=strTemp;
				}

				if(!strcmp(variable.c_str(), cmdListWithoutTokens[2])) // render size
				{
					sprintf(strTemp, "%ix%i", sceneInfo->frameWidth, sceneInfo->frameHeight);
					instanceData.strParsedCommandLine+=strTemp;
				}

				/*if(!strcmp(variable.c_str(), cmdListWithoutTokens[3])) // antialiasing
				{
				sprintf(strTemp, "%i", sceneInfo->adaptiveThreshold);
				strParsedCommandLine+=strTemp;
				}*/

				//if(!strcmp(variable.c_str(), cmdListWithoutTokens[4])) // system info
				//{					
				//	std::string cpuName;

				//	GetProcessorName(cpuName);

				//	strParsedCommandLine+=cpuName;
				//	strParsedCommandLine+=":";
				//	strParsedCommandLine+=gfx;

				//}
			}

			curCharacter+=(variableCharacters-curCharacter)+1;

		}
		else
			//if(!parseVariable)
		{
			instanceData.strParsedCommandLine+=instanceData.strCommandLine[curCharacter];

			curCharacter++;
		}		
	}
}

int readLine(std::string line, std::string cmd) // 0 false, 1 end of line, 2 ok
{
	int result=0;

	if(line.size()<cmd.size())
		return 1;

	bool parseOK=true;
	for (int n=0;n<cmd.size();n++)
	{
		if(cmd[n]!=line[n])
			parseOK=false;
	}
	if(parseOK)
		result=2;

	return result;
}

XCALL_( static void ) RenderTag_Process( LWInstance inst, const LWFilterAccess *fa )
{
	if(!pluginMasterCreated)
		return;

	LWFVector out;
	float *r, *g, *b, *a;
	int x, y;

	MON_INIT( fa->monitor, fa->height / 8 );

	ParseCommandLine();
	
	CalculateTextSize(hostDisplayInfo->window, instanceData.textSize, instanceData.textBold, instanceData.textItalic, instanceData.textUnderline, instanceData.strParsedCommandLine, instanceData.textFontName, 500, 100, textAreaSize);
	textAreaSize.cx=sceneInfo->frameWidth;
	textAreaSize.cy+=instanceData.textVSpace;

	RenderFont(hostDisplayInfo->window, 0, 0, instanceData.textSize, instanceData.textBold, instanceData.textItalic, instanceData.textUnderline, instanceData.strParsedCommandLine, instanceData.textFontName, textAreaSize.cx,textAreaSize.cy);

	for ( y = 0; y < fa->height; y++ ) {

		//get scanline
		r = fa->getLine( LWBUF_RED, y );
		g = fa->getLine( LWBUF_GREEN, y );
		b = fa->getLine( LWBUF_BLUE, y );
		a = fa->getLine( LWBUF_ALPHA, y );

		for ( x = 0; x < fa->width; x++ )
		{
			out[ 0 ] = r[ x ];
			out[ 1 ] = g[ x ];
			out[ 2 ] = b[ x ];
				
			fa->setRGB( x, y, out );
			fa->setAlpha( x, y, a[ x ] );
		}

		// once every 8 lines, step the monitor and check for abort
		if (( y & 7 ) == 7 )
			if ( MON_STEP( fa->monitor )) return;
	}

	for (int x=0;x<textAreaSize.cx;x++)
	{
		for (int y=0;y<textAreaSize.cy;y++)
		{
			out[0]= (float) buffer[((y * textAreaSize.cx) + x) * 3+2]/255;
			out[1]= (float) buffer[((y * textAreaSize.cx) + x) * 3+1]/255;
			out[2]= (float) buffer[((y * textAreaSize.cx) + x) * 3+0]/255;

			a = fa->getLine( LWBUF_ALPHA, y );

			fa->setRGB( 0+x, fa->height-y, out );
			//fa->setAlpha( 0+x, fa->height-y, 1.0 );
		}
	}

	delete []buffer;
	MON_DONE( fa->monitor );
}

XCALL_( int ) MiG_RenderTag_Activation( int version, GlobalFunc *global, LWImageFilterHandler *local, void *serverData )
{
	hostDisplayInfo = (HostDisplayInfo*) global( LWHOSTDISPLAYINFO_GLOBAL, GFUSE_TRANSIENT );
	if(!hostDisplayInfo) return AFUNC_BADGLOBAL;

	sceneInfo = (LWSceneInfo*) global( LWSCENEINFO_GLOBAL, GFUSE_TRANSIENT );
	if(!sceneInfo) return AFUNC_BADGLOBAL;

	lwTimeInfo = (LWTimeInfo*) global( LWTIMEINFO_GLOBAL, GFUSE_TRANSIENT );
	if(!lwTimeInfo) return AFUNC_BADGLOBAL;

	lwMsgFunc = (LWMessageFuncs *) global( LWMESSAGEFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if(!lwMsgFunc) return AFUNC_BADGLOBAL;
	
	lwFontListFunc = (LWFontListFuncs*) global( LWFONTLISTFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if(!lwFontListFunc) return AFUNC_BADGLOBAL;

	contextMenuFuncs = (ContextMenuFuncs*) global( LWCONTEXTMENU_GLOBAL, GFUSE_TRANSIENT );
	if(!contextMenuFuncs) return AFUNC_BADGLOBAL;

	lwCommandFunc = (LWCommandFunc*) global( LWCOMMANDFUNC_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwCommandFunc) return AFUNC_BADGLOBAL;

	lwDirInfoFunc = ( LWDirInfoFunc*) global( LWDIRINFOFUNC_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwDirInfoFunc) return AFUNC_BADGLOBAL;

	lwColorActivateFunc = (LWColorActivateFunc *) global( LWCOLORACTIVATEFUNC_GLOBAL, GFUSE_TRANSIENT );
	if ( !lwColorActivateFunc ) return AFUNC_BADGLOBAL;

	if ( version != LWIMAGEFILTER_VERSION ) return AFUNC_BADVERSION;

	//////////////////////////////////////////////////////////////////////////

	instanceData.textFontName="Arial";

	fontsLists.clear();

	for (int i=0;i<lwFontListFunc->count();i++)
	{
		char strFinalFont[255]={0};
		char strTempFont[255]={0};

		std::vector<std::string> fontTokens;

		strcpy(strTempFont, (char*)lwFontListFunc->name(i));
		char *token=strtok( strTempFont, " ");

		while( token != NULL )
		{
			fontTokens.push_back(token);

			token = strtok( NULL, " " );	
		}

		for (int n=0;n<fontTokens.size()-1;n++)
		{
			strcat(strFinalFont, fontTokens[n].c_str());
			if(n<fontTokens.size()-2)
				strcat(strFinalFont, " ");
		}

		fontsLists.push_back(strFinalFont);
	}
	

	local->inst->create  = RenderTag_Create;
	local->inst->destroy = RenderTag_Destroy;
	local->inst->copy    = RenderTag_Copy;
	local->inst->load    = RenderTag_Load;
	local->inst->save    = RenderTag_Save;
	local->inst->descln  = NULL;
	
	if ( local->item ) {
		local->item->useItems = NULL;
		local->item->changeID = NULL;
	}

	local->process = RenderTag_Process;
	local->flags   = RenderTag_Flags;

	return AFUNC_OK;
}


int fontFamilyListsEvent(void *data)
{
	GET_INT(controls[RS_TEXT_FONT], ival.intv.value);
	
	/*char strFinalFont[255]={0};
	char strTempFont[255]={0};

	std::vector<std::string> fontTokens;

	strcpy(strTempFont, (char*)lwFontListFunc->name(ival.intv.value));
	char *token=strtok( strTempFont, " ");

	while( token != NULL )
	{
		fontTokens.push_back(token);
		
		token = strtok( NULL, " " );	
	}
	
	for (int n=0;n<fontTokens.size()-1;n++)
	{
		strcat(strFinalFont, fontTokens[n].c_str());
		if(n<fontTokens.size()-1)
			strcat(strFinalFont, " ");
	}*/

	return 1;
}

void colorcb( void *data, float r, float g, float b )
{
	/* redraw my display with the current color */	
}

void colorPickerEvent(LWPanelID panelID, void *usr, int button, int x, int y)  /* see input qualifier bits     */   
{
	//if(x>0 && x<130 && y>0 && y<30 && button==MOUSE_LEFT)
	int bk_y=CON_HOTY(controls[RS_BK_COLOR]);
	int bk_h=CON_HOTH(controls[RS_BK_COLOR]);

	int tx_y=CON_HOTY(controls[RS_TEXT_COLOR]);
	int tx_h=CON_HOTH(controls[RS_TEXT_COLOR]);

	if(x>0 && x<133 && y>=bk_y && y<=bk_y+bk_h && button==MOUSE_LEFT)
	{
		LWColorPickLocal clrloc;
		int result;
		clrloc.title   = "Background Color";
		clrloc.red     = instanceData.textAreaBkColor[0];
		clrloc.green   = instanceData.textAreaBkColor[1];
		clrloc.blue    = instanceData.textAreaBkColor[2];
		clrloc.data    = NULL;//&myhotdata;
		clrloc.hotFunc = colorcb;

		result = lwColorActivateFunc( LWCOLORPICK_VERSION, &clrloc );
		if ( result == AFUNC_OK && clrloc.result > 0 ) 
		{
			instanceData.textAreaBkColor[0] = clrloc.red*255;
			instanceData.textAreaBkColor[1] = clrloc.green*255;
			instanceData.textAreaBkColor[2] = clrloc.blue*255;

			SET_IVEC(controls[RS_BK_COLOR], instanceData.textAreaBkColor[0], instanceData.textAreaBkColor[1], instanceData.textAreaBkColor[2]);
		}
	}

	//if(x>0 && x<130 && y>35 && y<55 && button==MOUSE_LEFT)
	if(x>0 && x<133 && y>=tx_y && y<=tx_y+tx_h && button==MOUSE_LEFT)
	{
		LWColorPickLocal clrloc;
		int result;
		clrloc.title   = "Text Color";
		clrloc.red     = instanceData.textColor[0];
		clrloc.green   = instanceData.textColor[1];
		clrloc.blue    = instanceData.textColor[2];
		clrloc.data    = NULL;//&myhotdata;
		clrloc.hotFunc = colorcb;

		result = lwColorActivateFunc( LWCOLORPICK_VERSION, &clrloc );
		if ( result == AFUNC_OK && clrloc.result > 0 ) 
		{
			instanceData.textColor[0] = clrloc.red*255;
			instanceData.textColor[1] = clrloc.green*255;
			instanceData.textColor[2] = clrloc.blue*255;

			SET_IVEC(controls[RS_TEXT_COLOR], instanceData.textColor[0], instanceData.textColor[1], instanceData.textColor[2]);
		}
	}
}

void CommandsEvent(void *usr)
{
	int select=0, current=0;

	select = contextMenuFuncs->cmenuDeploy( menu, lwPanelID	, current );
	if (select != -1 )
	{
		current = select;		

		GET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, 255);

		strcat(instanceData.strCommandLine, cmdList[current]);

		SET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, strlen(instanceData.strCommandLine));

		//ACTIVATE_CON(controls[RS_COMMAND_LINE]);
	}
}

void ReadString(FILE *f, char *string)
{
	int i=0;
	char c;
	do
	{
		c=fgetc(f);
		string[i]=c;
		i++;
	} while (c!='\0');

	return;

}
void ButtonsEvent(LWControlID ctrlID, int controlIndex)
{
	OPENFILENAME ofn = {0};
	char currentFolder[MAX_PATH] = {0};
	char fileName[MAX_PATH] = {0};

	switch (controlIndex)
	{
	case 0: // preview
		{
			// lightwave configuration path 
			char *previewPathPath=0;
			previewPathPath=(char*) lwDirInfoFunc(LWFTYPE_SETTING);
			if(previewPathPath)
			{
				char temp[255]={0};
				sprintf_s(temp, "%s\\mig_rendertag.bmp", previewPathPath);

				//////////////////////////////////////////////////////////////////////////
				// get components values
				GET_IVEC(controls[RS_TEXT_COLOR], ivecval.ivec.val[0], ivecval.ivec.val[1], ivecval.ivec.val[2]);
				instanceData.textColor[0]=ivecval.ivec.val[0];
				instanceData.textColor[1]=ivecval.ivec.val[1];
				instanceData.textColor[2]=ivecval.ivec.val[2];	

				GET_IVEC(controls[RS_BK_COLOR], ivecval.ivec.val[0], ivecval.ivec.val[1], ivecval.ivec.val[2]);
				instanceData.textAreaBkColor[0]=ivecval.ivec.val[0];
				instanceData.textAreaBkColor[1]=ivecval.ivec.val[1];
				instanceData.textAreaBkColor[2]=ivecval.ivec.val[2];

				GET_INT(controls[RS_TEXT_FONT], ival.intv.value);
				instanceData.textFontName=fontsLists[ival.intv.value];

				GET_INT(controls[RS_TEXT_SIZE], ival.intv.value);
				instanceData.textSize=ival.intv.value;

				GET_INT(controls[RS_TEXT_BOLD], ival.intv.value);
				instanceData.textBold=ival.intv.value;

				GET_INT(controls[RS_TEXT_ITALIC], ival.intv.value);
				instanceData.textItalic=ival.intv.value;

				GET_INT(controls[RS_TEXT_UNDERLINE], ival.intv.value);
				instanceData.textUnderline=ival.intv.value;

				GET_INT(controls[RS_TEXT_HALIGN], ival.intv.value);
				instanceData.textHAlign=ival.intv.value;

				GET_INT(controls[RS_TEXT_VALIGN], ival.intv.value);
				instanceData.textVAlign=ival.intv.value;

				GET_INT(controls[RS_TEXT_HSPACE], ival.intv.value);
				instanceData.textHSpace=ival.intv.value;

				GET_INT(controls[RS_TEXT_VSPACE], ival.intv.value);
				instanceData.textVSpace=ival.intv.value;

				GET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, 255); //@@@@@@@@@@@@@ W PRZYPADKU GUBIENIA STRINGA ZASTAPIC 255 > strlen
				//////////////////////////////////////////////////////////////////////////

				masterStartFrameTime=timeGetTime();

				ParseCommandLine();

				CalculateTextSize(hostDisplayInfo->window, instanceData.textSize, instanceData.textBold, instanceData.textItalic, instanceData.textUnderline, instanceData.strParsedCommandLine, instanceData.textFontName, 500, 100, textAreaSize);
				textAreaSize.cx=sceneInfo->frameWidth;
				textAreaSize.cy+=instanceData.textVSpace; //przestrzen pionowa od paska do textu

				RenderFont(hostDisplayInfo->window, 0, 0, instanceData.textSize, instanceData.textBold, instanceData.textItalic, instanceData.textUnderline, instanceData.strParsedCommandLine, instanceData.textFontName, textAreaSize.cx,textAreaSize.cy);

				SaveBMP(temp, textAreaSize.cx, textAreaSize.cy, buffer);
				
				ShellExecute(NULL, "open", temp, "", "", SW_SHOWNORMAL);

				delete []buffer;
				buffer=NULL;
			}
			else
			{
				//ShellExecute(NULL, "open", "notepad.exe", "", "", SW_SHOWNORMAL);
			}
		}
		break;
	case 1: // load	
		GetCurrentDirectory(MAX_PATH, currentFolder);

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hostDisplayInfo->window;
		ofn.lpstrFilter="*.ini";
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = currentFolder;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_PATHMUSTEXIST;

		if(GetOpenFileName(&ofn))
		{
			FILE *f=fopen(fileName, "r");
			if(f)
			{
				fclose(f);

				CSimpleIniA ini;
				ini.SetUnicode();
				ini.SetMultiLine(1);
				ini.SetMultiKey(1);
				ini.LoadFile(fileName);

				instanceData.textColor[0]=ini.GetLongValue("MiG_RenderTag", "textColorR", 255);
				instanceData.textColor[1]=ini.GetLongValue("MiG_RenderTag", "textColorG", 255);
				instanceData.textColor[2]=ini.GetLongValue("MiG_RenderTag", "textColorB", 255);

				instanceData.textAreaBkColor[0]=ini.GetLongValue("MiG_RenderTag", "backgroundColorR", 0);
				instanceData.textAreaBkColor[1]=ini.GetLongValue("MiG_RenderTag", "backgroundColorG", 0);
				instanceData.textAreaBkColor[2]=ini.GetLongValue("MiG_RenderTag", "backgroundColorB", 0);

				instanceData.textFontName=ini.GetValue("MiG_RenderTag", "fontName", "Arial");

				instanceData.textSize=ini.GetLongValue("MiG_RenderTag", "fontSize", 12);

				instanceData.textBold=ini.GetBoolValue("MiG_RenderTag", "bold", false);
				instanceData.textItalic=ini.GetBoolValue("MiG_RenderTag", "italic", false);
				instanceData.textUnderline=ini.GetBoolValue("MiG_RenderTag", "underline", false);

				instanceData.textHAlign=ini.GetLongValue("MiG_RenderTag", "alignH", 2);

				instanceData.textHAlign=ini.GetLongValue("MiG_RenderTag", "alignV", 1);

				instanceData.textHSpace=ini.GetLongValue("MiG_RenderTag", "spaceH", 5);

				instanceData.textVSpace=ini.GetLongValue("MiG_RenderTag", "spaceV", 5);

				strcpy(instanceData.strCommandLine, ini.GetValue("MiG_RenderTag", "commandLine", ""));

				//

				SET_IVEC(controls[RS_BK_COLOR], instanceData.textAreaBkColor[0], instanceData.textAreaBkColor[1], instanceData.textAreaBkColor[2]);
				SET_IVEC(controls[RS_TEXT_COLOR], instanceData.textColor[0], instanceData.textColor[1], instanceData.textColor[2]);
				
				int findex=-1;
				for (int i=0;i<fontsLists.size();i++)
					if(fontsLists[i]==instanceData.textFontName)
						findex=i;
				
				if(findex==-1)				
					SET_INT(controls[RS_TEXT_FONT], 0);
				else
					SET_INT(controls[RS_TEXT_FONT], findex);

				SET_INT(controls[RS_TEXT_SIZE], instanceData.textSize);
				SET_INT(controls[RS_TEXT_BOLD], instanceData.textBold);
				SET_INT(controls[RS_TEXT_ITALIC], instanceData.textItalic);
				SET_INT(controls[RS_TEXT_UNDERLINE], instanceData.textUnderline);
				SET_INT(controls[RS_TEXT_HALIGN], instanceData.textHAlign);
				SET_INT(controls[RS_TEXT_VALIGN], instanceData.textVAlign);
				SET_INT(controls[RS_TEXT_HSPACE], instanceData.textHSpace);
				SET_INT(controls[RS_TEXT_VSPACE], instanceData.textVSpace);
				SET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, strlen(instanceData.strCommandLine));				
			}
		}
			
		break;
	case 2: // save
		GetCurrentDirectory(MAX_PATH, currentFolder);

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hostDisplayInfo->window;
		ofn.lpstrFilter="*.ini";
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = currentFolder;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_PATHMUSTEXIST;
		if(GetSaveFileName(&ofn))
		{
			FILE *f=fopen(fileName, "w");
			if(f)
			{
				fclose(f);

				// get components values
				GET_IVEC(controls[RS_TEXT_COLOR], ivecval.ivec.val[0], ivecval.ivec.val[1], ivecval.ivec.val[2]);
				instanceData.textColor[0]=ivecval.ivec.val[0];
				instanceData.textColor[1]=ivecval.ivec.val[1];
				instanceData.textColor[2]=ivecval.ivec.val[2];	

				GET_IVEC(controls[RS_BK_COLOR], ivecval.ivec.val[0], ivecval.ivec.val[1], ivecval.ivec.val[2]);
				instanceData.textAreaBkColor[0]=ivecval.ivec.val[0];
				instanceData.textAreaBkColor[1]=ivecval.ivec.val[1];
				instanceData.textAreaBkColor[2]=ivecval.ivec.val[2];

				GET_INT(controls[RS_TEXT_FONT], ival.intv.value);
				instanceData.textFontName=fontsLists[ival.intv.value];

				GET_INT(controls[RS_TEXT_SIZE], ival.intv.value);
				instanceData.textSize=ival.intv.value;

				GET_INT(controls[RS_TEXT_BOLD], ival.intv.value);
				instanceData.textBold=ival.intv.value;

				GET_INT(controls[RS_TEXT_ITALIC], ival.intv.value);
				instanceData.textItalic=ival.intv.value;

				GET_INT(controls[RS_TEXT_UNDERLINE], ival.intv.value);
				instanceData.textUnderline=ival.intv.value;

				GET_INT(controls[RS_TEXT_HALIGN], ival.intv.value);
				instanceData.textHAlign=ival.intv.value;

				GET_INT(controls[RS_TEXT_VALIGN], ival.intv.value);
				instanceData.textVAlign=ival.intv.value;

				GET_INT(controls[RS_TEXT_HSPACE], ival.intv.value);
				instanceData.textHSpace=ival.intv.value;

				GET_INT(controls[RS_TEXT_VSPACE], ival.intv.value);
				instanceData.textVSpace=ival.intv.value;

				GET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, 255); //@@@@@@@@@@@@@ W PRZYPADKU GUBIENIA STRINGA ZASTAPIC 255 > strlen

				CSimpleIniA ini;
				ini.SetUnicode();
				ini.SetMultiLine(1);
				ini.SetMultiKey(1);

				ini.SetLongValue("MiG_RenderTag", "textColorR", (int) instanceData.textColor[0]);
				ini.SetLongValue("MiG_RenderTag", "textColorG", (int) instanceData.textColor[1]);
				ini.SetLongValue("MiG_RenderTag", "textColorB", (int) instanceData.textColor[2]);

				ini.SetLongValue("MiG_RenderTag", "backgroundColorR", (int) instanceData.textAreaBkColor[0]);
				ini.SetLongValue("MiG_RenderTag", "backgroundColorG", (int) instanceData.textAreaBkColor[1]);
				ini.SetLongValue("MiG_RenderTag", "backgroundColorB", (int) instanceData.textAreaBkColor[2]);

				ini.SetValue("MiG_RenderTag", "fontName", instanceData.textFontName.c_str());

				ini.SetLongValue("MiG_RenderTag", "fontSize", (int) instanceData.textSize);

				ini.SetBoolValue("MiG_RenderTag", "bold", (int) instanceData.textBold);
				ini.SetBoolValue("MiG_RenderTag", "italic", (int) instanceData.textItalic);
				ini.SetBoolValue("MiG_RenderTag", "underline", (int) instanceData.textUnderline);

				ini.SetLongValue("MiG_RenderTag", "alignH", (int) instanceData.textHAlign);

				ini.SetLongValue("MiG_RenderTag", "alignV", (int) instanceData.textVAlign);

				ini.SetLongValue("MiG_RenderTag", "spaceH", (int) instanceData.textHSpace);

				ini.SetLongValue("MiG_RenderTag", "spaceV", (int) instanceData.textVSpace);

				ini.SetValue("MiG_RenderTag", "commandLine", instanceData.strCommandLine);

				if(ini.SaveFile(fileName)!=SI_OK)				
				{
					//lwMsgFunc->error("Cannot load configuration file !", 0);
					return;
				}				
			}	
		}
		break;
	case 3: // about							  
		
		lwMsgFunc->info("MiG_RenderTag\nVersion 0.6.1\nPlease send bug reports, comments, suggestions, and feature requests via email\nMichal Golek\nmichalgolek@o2.pl", 0);
		
		break;
	}
}

char *name( void *data, int index )
{
	if ( index >= 0 && index < lwFontListFunc->count() )
		//return (char*)lwFontListFunc->name(index);
		return (char*)fontsLists[index].c_str();
	else
		return NULL;
}

static int count( void *data )
{
	return lwFontListFunc->count();
}

int menuCount( MyMenuData *data )
{
	return data->count;
}

int menuName( MyMenuData *data, int index )
{
	if ( index >= 0 && index < data->count )
		return (int)data->name[ index ];

	return NULL;
}

void CreateControls()
{
	controls[RS_BK_COLOR] = MINIRGB_CTL(lwPanelFuncs, lwPanelID,"Background Color");
	SET_IVEC(controls[RS_BK_COLOR], instanceData.textAreaBkColor[0], instanceData.textAreaBkColor[1], instanceData.textAreaBkColor[2]);
	
	controls[RS_TEXT_COLOR] = RGBVEC_CTL(lwPanelFuncs, lwPanelID,"Text Color");
	SET_IVEC(controls[RS_TEXT_COLOR], instanceData.textColor[0], instanceData.textColor[1], instanceData.textColor[2]);
	MOVE_CON(controls[RS_TEXT_COLOR], 41,30);

	controls[RS_TEXT_FONT] = CUSTPOPUP_CTL( lwPanelFuncs, lwPanelID, "Font family", 138, name, count );

	int fontIndex=-1;
	int fnum=lwFontListFunc->count();
	for (int i=0;i<fnum;i++)
	{
		if(instanceData.textFontName.compare(fontsLists[i])==0)
		{
			fontIndex=i;
			break;
		}
	}
	if(fontIndex==-1)
	{
		fontIndex=0;
		instanceData.textFontName=fontsLists[fontIndex]; // strcpy(textFontName, (char*)lwFontListFunc->name(0)); // 
	}
	
	SET_INT(controls[RS_TEXT_FONT], fontIndex);
	MOVE_CON(controls[RS_TEXT_FONT], 5, CON_Y(controls[RS_TEXT_FONT]));	
	CON_SETEVENT(controls[RS_TEXT_FONT], fontFamilyListsEvent, 0);

	controls[RS_TEXT_SIZE] = INT_CTL( lwPanelFuncs, lwPanelID, "Font size");
	SET_INT(controls[RS_TEXT_SIZE], instanceData.textSize);
	MOVE_CON(controls[RS_TEXT_SIZE], 211, 54);



	controls[RS_TEXT_BOLD] = BOOL_CTL( lwPanelFuncs, lwPanelID, "Bold");
	SET_INT(controls[RS_TEXT_BOLD], instanceData.textBold);
	MOVE_CON(controls[RS_TEXT_BOLD], 60, 79);

	controls[RS_TEXT_ITALIC] = BOOL_CTL( lwPanelFuncs, lwPanelID, "Italic");
	SET_INT(controls[RS_TEXT_ITALIC], instanceData.textItalic);
	MOVE_CON(controls[RS_TEXT_ITALIC], 125, 79);

	controls[RS_TEXT_UNDERLINE] = BOOL_CTL( lwPanelFuncs, lwPanelID, "Underline");
	SET_INT(controls[RS_TEXT_UNDERLINE], instanceData.textUnderline);
	MOVE_CON(controls[RS_TEXT_UNDERLINE], 190, 79);




	const char *hAlignList[]={"L", "C", "R", NULL};
	const char *vAlignList[]={"T", "C", "B", NULL};

	controls[RS_TEXT_HALIGN] = HCHOICE_CTL( lwPanelFuncs, lwPanelID, "Align", hAlignList);
	SET_INT(controls[RS_TEXT_HALIGN], instanceData.textHAlign);
	MOVE_CON(controls[RS_TEXT_HALIGN], 5, 104);

	controls[RS_TEXT_VALIGN] = HCHOICE_CTL( lwPanelFuncs, lwPanelID, "", vAlignList);
	SET_INT(controls[RS_TEXT_VALIGN], instanceData.textVAlign);
	MOVE_CON(controls[RS_TEXT_VALIGN], 87, 104);
	

	controls[RS_TEXT_HSPACE] = INT_CTL( lwPanelFuncs, lwPanelID, "Space");
	SET_INT(controls[RS_TEXT_HSPACE], instanceData.textHSpace);
	MOVE_CON(controls[RS_TEXT_HSPACE], 155, 104);
	
	controls[RS_TEXT_VSPACE] = INT_CTL( lwPanelFuncs, lwPanelID, "");
	SET_INT(controls[RS_TEXT_VSPACE], instanceData.textVSpace);
	MOVE_CON(controls[RS_TEXT_VSPACE], 251, 104);
	

	controls[RS_COMMAND_LINE] = STR_CTL(lwPanelFuncs, lwPanelID, "", 57);
	SET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, strlen(instanceData.strCommandLine));
	MOVE_CON(controls[RS_COMMAND_LINE], CON_X(controls[RS_COMMAND_LINE]), 129);

	controls[RS_COMMANDS] = WBUTTON_CTL(lwPanelFuncs, lwPanelID, "<", 20);
	MOVE_CON(controls[RS_COMMANDS], 303, 129);
	CON_SETEVENT(controls[RS_COMMANDS], CommandsEvent, 0);

	controls[RS_PREVIEW] = BUTTON_CTL(lwPanelFuncs, lwPanelID, "Preview");
	MOVE_CON(controls[RS_PREVIEW], 19, 154);
	CON_SETEVENT(controls[RS_PREVIEW], ButtonsEvent, 0);

	controls[RS_LOAD] = BUTTON_CTL(lwPanelFuncs, lwPanelID, "Load");
	MOVE_CON(controls[RS_LOAD], 114, CON_Y(controls[RS_PREVIEW]));	
	CON_SETEVENT(controls[RS_LOAD], ButtonsEvent, 1);

	controls[RS_SAVE] = BUTTON_CTL(lwPanelFuncs, lwPanelID, "Save");
	MOVE_CON(controls[RS_SAVE], 209, CON_Y(controls[RS_PREVIEW]));	
	CON_SETEVENT(controls[RS_SAVE], ButtonsEvent, 2);

	controls[RS_ABOUT] = WBUTTON_CTL(lwPanelFuncs, lwPanelID, "?", 20);
	MOVE_CON(controls[RS_ABOUT], 303, CON_Y(controls[RS_PREVIEW]));
	CON_SETEVENT(controls[RS_ABOUT], ButtonsEvent, 3);


	LWPanPopupDesc desc;

	desc.type    = LWT_POPUP;
	desc.width   = 200;
	desc.countFn = (int (__cdecl *)(void *))menuCount;
	desc.nameFn  = (char *(__cdecl *)(void *,int))menuName;

	menu = (LWContextMenuID)contextMenuFuncs->cmenuCreate( &desc, &menudata );	
}

XCALL_( static LWError ) MiG_RenderTag_Options(void *data)
{
	lwPanelID = PAN_CREATE(lwPanelFuncs, "MiG_RenderTag" );
	if (!lwPanelID)
		return AFUNC_OK;

	CreateControls();

	lwPanelFuncs->set(lwPanelID, PAN_MOUSEBUTTON, colorPickerEvent);

	lwPanelFuncs->open( lwPanelID, PANF_BLOCKING );
	

	GET_IVEC(controls[RS_TEXT_COLOR], ivecval.ivec.val[0], ivecval.ivec.val[1], ivecval.ivec.val[2]);
	instanceData.textColor[0]=ivecval.ivec.val[0];
	instanceData.textColor[1]=ivecval.ivec.val[1];
	instanceData.textColor[2]=ivecval.ivec.val[2];	

	GET_IVEC(controls[RS_BK_COLOR], ivecval.ivec.val[0], ivecval.ivec.val[1], ivecval.ivec.val[2]);
	instanceData.textAreaBkColor[0]=ivecval.ivec.val[0];
	instanceData.textAreaBkColor[1]=ivecval.ivec.val[1];
	instanceData.textAreaBkColor[2]=ivecval.ivec.val[2];

	GET_INT(controls[RS_TEXT_FONT], ival.intv.value);
	instanceData.textFontName=fontsLists[ival.intv.value];

	GET_INT(controls[RS_TEXT_SIZE], ival.intv.value);
	instanceData.textSize=ival.intv.value;

	GET_INT(controls[RS_TEXT_BOLD], ival.intv.value);
	instanceData.textBold=ival.intv.value;

	GET_INT(controls[RS_TEXT_ITALIC], ival.intv.value);
	instanceData.textItalic=ival.intv.value;

	GET_INT(controls[RS_TEXT_UNDERLINE], ival.intv.value);
	instanceData.textUnderline=ival.intv.value;

	GET_INT(controls[RS_TEXT_HALIGN], ival.intv.value);
	instanceData.textHAlign=ival.intv.value;

	GET_INT(controls[RS_TEXT_VALIGN], ival.intv.value);
	instanceData.textVAlign=ival.intv.value;

	GET_INT(controls[RS_TEXT_HSPACE], ival.intv.value);
	instanceData.textHSpace=ival.intv.value;

	GET_INT(controls[RS_TEXT_VSPACE], ival.intv.value);
	instanceData.textVSpace=ival.intv.value;

	GET_STR(controls[RS_COMMAND_LINE], instanceData.strCommandLine, 255); //@@@@@@@@@@@@@ W PRZYPADKU GUBIENIA STRINGA ZASTAPIC 255 > strlen

	PAN_KILL(lwPanelFuncs, lwPanelID );

	contextMenuFuncs->cmenuDestroy( menu );

	if(!pluginMasterCreated)
	{
		char temp[255]={0};
		sprintf_s(temp, "ApplyServer %s %s", "MasterHandler", PLUGIN_MASTER_NAME);
		lwCommandFunc(temp);

		pluginMasterCreated=true;
	}

	return NULL;
}

XCALL_( int ) MiG_RenderTag_Interface( int version, GlobalFunc *global,
						  LWInterface *local, void *serverData )
{
	if ( version != LWINTERFACE_VERSION )
		return AFUNC_BADVERSION;

	lwPanelFuncs = (LWPanelFuncs* )global(LWPANELFUNCS_GLOBAL, GFUSE_TRANSIENT );
	if (!lwPanelFuncs) return AFUNC_BADGLOBAL;
	lwPanelFuncs->globalFun = global;

	local->panel   = NULL;
	local->options = MiG_RenderTag_Options;
	local->command = NULL;

	return AFUNC_OK;

}
extern "C"
{
	ServerRecord ServerDesc[] = {
		{LWMASTER_HCLASS, PLUGIN_MASTER_NAME, (ActivateFunc (__cdecl *)) MasterActivation},
		{LWMASTER_ICLASS, PLUGIN_MASTER_NAME, (ActivateFunc (__cdecl *)) MasterInterface},

		{LWIMAGEFILTER_HCLASS, PLUGIN_IMG_FILTER_NAME, (ActivateFunc (__cdecl *))MiG_RenderTag_Activation},
		{LWIMAGEFILTER_ICLASS, PLUGIN_IMG_FILTER_NAME, (ActivateFunc (__cdecl *))MiG_RenderTag_Interface},

		{NULL}
	};
}