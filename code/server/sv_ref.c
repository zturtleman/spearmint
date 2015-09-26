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

// glue code for dedicated server model loading

#if defined DEDICATED && !defined USE_RENDERER_DLOPEN

#include "../renderergl1/tr_local.h"

qboolean refHeadless;

refimport_t ri;
trGlobals_t tr;
glconfig_t glConfig;
backEndState_t backEnd;
shaderCommands_t	tess;

shader_t localShader;

cvar_t *r_shadows = NULL;

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp, qboolean headless ) {

	static refexport_t	re;

	ri = *rimp;

	refHeadless = headless;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", 
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;

	re.RegisterModel = RE_RegisterModel;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	return &re;
}

void R_Init( void ) {
	Com_Memset( &tr, 0, sizeof (trGlobals_t) );
	Com_Memset( &glConfig, 0, sizeof (glconfig_t) );
	Com_Memset( &backEnd, 0, sizeof (backEndState_t) );
	Com_Memset( &tess, 0, sizeof (shaderCommands_t) );

	// dummy shader
	Q_strncpyz(localShader.name, "<default>", MAX_QPATH);
	localShader.defaultShader = qtrue;
	tr.defaultShader = &localShader;

	R_ModelInit();
}

shader_t *R_FindShader(const char *name, int lightmapIndex, imgFlags_t rawImageFlags ) {
	return tr.defaultShader;
}

shader_t * R_GetShaderByHandle(qhandle_t handle) {
	return tr.defaultShader;
}

int R_CullLocalBox (vec3_t bounds[2]) {
	return CULL_CLIP;
}

int R_PointFogNum( const trRefdef_t *refdef, vec3_t point, float radius ) {
	return 0;
}

void RE_Shutdown( qboolean destroyWindow ) {
	tr.registered = qfalse;
}

void RB_CheckOverflow( int verts, int indexes ) {

}

void R_AddEntDrawSurf( trRefEntity_t *ent, surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap, int sortLevel ) {

}

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap ) {

}

void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent ) {

}

void R_ClearFlares( void ) {

}

void RE_ClearScene( void ) {

}

void R_IssuePendingRenderCommands( void ) {

}

#endif

