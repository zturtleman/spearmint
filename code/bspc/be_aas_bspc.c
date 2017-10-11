/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../bspc/l_log.h"
#include "../bspc/l_qfiles.h"
#include "../botlib/l_memory.h"
#include "../botlib/l_script.h"
#include "../botlib/l_precomp.h"
#include "../botlib/l_struct.h"
#include "../botlib/aasfile.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_aas_def.h"
#include "../qcommon/cm_public.h"

//#define BSPC

extern botlib_import_t botimport;
extern	qboolean capsule_collision;

botlib_import_t botimport;
clipHandle_t worldmodel;

void Error (char *error, ...);

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Error(char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	Error(text);
} //end of the function AAS_Error
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Sys_MilliSeconds(void)
{
	return clock() * 1000 / CLOCKS_PER_SEC;
} //end of the function Sys_MilliSeconds
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_DebugLine(vec3_t start, vec3_t end, int color)
{
} //end of the function AAS_DebugLine
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ClearShownDebugLines(void)
{
} //end of the function AAS_ClearShownDebugLines
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BotImport_GetEntityToken( int *offset, char *buffer, int size ) {
	return CM_GetEntityToken( offset, buffer, size );
} //end of the function BotImport_GetEntityToken
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotImport_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask)
{
	CM_BoxTrace(bsptrace, start, end, mins, maxs, worldmodel, contentmask, capsule_collision ? TT_CAPSULE : TT_AABB);
} //end of the function BotImport_Trace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotImport_PointContents(vec3_t p)
{
	return CM_PointContents(p, worldmodel);
} //end of the function BotImport_PointContents
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *BotImport_GetMemory(int size)
{
	return GetMemory(size);
} //end of the function BotImport_GetMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
__attribute__ ((format (printf, 2, 3))) void BotImport_Print(int type, char *fmt, ...)
{
	va_list argptr;
	char buf[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(buf, sizeof (buf), fmt, argptr);
	printf("%s", buf);
	if (buf[0] != '\r') Log_Write("%s", buf);
	va_end(argptr);
} //end of the function BotImport_Print
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotImport_BSPModelMinsMaxsOrigin(int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin)
{
	clipHandle_t h;
	vec3_t mins, maxs;
	float max;
	int	i;

	h = CM_InlineModel(modelnum);
	CM_ModelBounds(h, mins, maxs);
	//if the model is rotated
	if ((angles[0] || angles[1] || angles[2]))
	{	// expand for rotation

		max = RadiusFromBounds(mins, maxs);
		for (i = 0; i < 3; i++)
		{
			mins[i] = (mins[i] + maxs[i]) * 0.5 - max;
			maxs[i] = (mins[i] + maxs[i]) * 0.5 + max;
		} //end for
	} //end if
	if (outmins) VectorCopy(mins, outmins);
	if (outmaxs) VectorCopy(maxs, outmaxs);
	if (origin) VectorClear(origin);
} //end of the function BotImport_BSPModelMinsMaxsOrigin
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Com_DPrintf(const char *fmt, ...)
{
	va_list argptr;
	char buf[1024];

	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr);
	printf("%s", buf);
	if (buf[0] != '\r') Log_Write("%s", buf);
	va_end(argptr);
} //end of the function Com_DPrintf
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitBotImport(void)
{
	botimport.MilliSeconds = Sys_MilliSeconds;
	botimport.Print = BotImport_Print;
	botimport.Trace = BotImport_Trace;
	botimport.EntityTrace = NULL;
	botimport.PointContents = BotImport_PointContents;
	botimport.inPVS = NULL;
	botimport.GetEntityToken = BotImport_GetEntityToken;
	botimport.BSPModelMinsMaxsOrigin = BotImport_BSPModelMinsMaxsOrigin;
	botimport.BotClientCommand = NULL;
	botimport.GetMemory = BotImport_GetMemory;
	botimport.FreeMemory = FreeMemory;
	botimport.AvailableMemory = NULL;
	botimport.HunkAlloc = NULL;
	botimport.FS_FOpenFile = NULL;
	botimport.FS_Read = NULL;
	botimport.FS_Write = NULL;
	botimport.FS_FCloseFile = NULL;
	botimport.FS_Seek = NULL;
	botimport.DebugLineCreate = NULL;
	botimport.DebugLineDelete = NULL;
	botimport.DebugLineShow = NULL;
	botimport.DebugPolygonCreate = NULL;
	botimport.DebugPolygonDelete = NULL;
} //end of the function AAS_InitBotImport
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CalcReachAndClusters(struct quakefile_s *qf)
{
	float time;

	Log_Print("loading collision map...\n");
	//
	if (!qf->pakfile[0]) strcpy(qf->pakfile, qf->filename);
	//load the map
	CM_LoadMap((char *) qf, qfalse, &aasworld.bspchecksum);
	//get a handle to the world model
	worldmodel = CM_InlineModel(0);		// 0 = world, 1 + are bmodels
	//initialize bot import structure
	AAS_InitBotImport();
	//load the BSP entity string
	AAS_LoadBSPFile();
	//init physics settings
	AAS_InitSettings();
	//initialize AAS link heap
	AAS_InitAASLinkHeap();
	//initialize the AAS linked entities for the new map
	AAS_InitAASLinkedEntities();
	//reset all reachabilities and clusters
	aasworld.reachabilitysize = 0;
	aasworld.numclusters = 0;
	//set all view portals as cluster portals in case we re-calculate the reachabilities and clusters (with -reach)
	AAS_SetViewPortalsAsClusterPortals();
	//calculate reachabilities
	AAS_InitReachability();
	time = 0;
	while(AAS_ContinueInitReachability(time)) time++;
	//calculate clusters
	AAS_InitClustering();
} //end of the function AAS_CalcReachAndClusters
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RecalcClusters(void)
{
	aasworld.numclusters = 0;
	AAS_InitBotImport();
	AAS_InitClustering();
} //end of the function AAS_RecalcClusters
