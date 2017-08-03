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
#ifndef TR_COMMON_H
#define TR_COMMON_H

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
#include "qgl.h"

typedef enum
{
	IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
	IMGTYPE_NORMAL,
	IMGTYPE_NORMALHEIGHT,
	IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
	IMGFLAG_NONE           = 0x0000,
	IMGFLAG_MIPMAP         = 0x0001,
	IMGFLAG_PICMIP         = 0x0002,
	IMGFLAG_CUBEMAP        = 0x0004,
	IMGFLAG_NO_COMPRESSION = 0x0010,
	IMGFLAG_NOLIGHTSCALE   = 0x0020,
	IMGFLAG_CLAMPTOEDGE    = 0x0040,
	IMGFLAG_GENNORMALMAP   = 0x0080,
	IMGFLAG_LIGHTMAP       = 0x0100,
	IMGFLAG_PICMIP2        = 0x0200,
} imgFlags_t;

#define MIP_RAW_IMAGE ( IMGFLAG_MIPMAP | IMGFLAG_PICMIP )

typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
	int			width, height;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint		texnum;					// gl texture binding

	int			frameUsed;			// for texture usage in frame statistics

	int			internalFormat;
	int			TMU;

	imgType_t   type;
	imgFlags_t  flags;

	struct image_s*	next;
} image_t;

#ifdef DEDICATED
typedef int GLenum;
typedef int GLsizei;
#endif
typedef struct textureLevel_s {
	GLenum  format;
	int     width, height;
	GLsizei size;
	void    *data;
} textureLevel_t;

extern	refimport_t		ri;
extern glconfig_t	glConfig;		// outside of TR since it shouldn't be cleared during ref re-init

// used by shader functions, including noise in renderercommon
#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)

// dlight flags
#define REF_ADDITIVE_DLIGHT	0x01 // texture detail is lost tho when the lightmap is dark
#define REF_GRID_DLIGHT		0x02 // affect dynamic light grid
#define REF_SURFACE_DLIGHT	0x04 // affect world surfaces
#define REF_DIRECTED_DLIGHT	0x08 // global directional light, origin should be interpreted as a normal vector
#define REF_VERTEX_DLIGHT	0x10 // ET style spherical dlight using vertex light rendering
#define REF_FORCE_DLIGHT	0x20 // force this dlight under all conditions

// surfaceParm flags (these are only used internally, not to be confused with surfaceFlags values in BSPs)
#define	SURF_SKY				0x01	// lighting from environment map
#define	SURF_NOIMPACT			0x02	// don't make missile explosions
#define	SURF_NOMARKS			0x04	// don't leave missile marks
#define	SURF_NOLIGHTMAP			0x08	// surface doesn't need a lightmap
#define	SURF_POINTLIGHT			0x10	// generate lighting info at vertexes
#define	SURF_NODLIGHT			0x20	// don't dlight even if solid (solid lava, skies)
#define	SURF_FOG				0x40

//
// cvars
//
extern cvar_t *r_stencilbits;			// number of desired stencil bits
extern cvar_t *r_depthbits;			// number of desired depth bits
extern cvar_t *r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern cvar_t *r_texturebits;			// number of desired texture bits
extern cvar_t *r_ext_multisample;
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error

extern cvar_t *r_mode;				// video mode
extern cvar_t *r_noborder;
extern cvar_t *r_fullscreen;
extern cvar_t *r_ignorehwgamma;		// overrides hardware gamma capabilities
extern cvar_t *r_drawBuffer;
extern cvar_t *r_swapInterval;

extern cvar_t *r_allowExtensions;				// global enable/disable of OpenGL extensions
extern cvar_t *r_ext_compressed_textures;		// these control use of specific extensions
extern cvar_t *r_ext_multitexture;
extern cvar_t *r_ext_compiled_vertex_array;
extern cvar_t *r_ext_texture_env_add;

extern cvar_t *r_ext_texture_filter_anisotropic;
extern cvar_t *r_ext_max_anisotropy;

extern cvar_t *r_stereoEnabled;

extern	cvar_t	*r_saveFontData;
extern	cvar_t	*r_fontBorderWidth;
extern	cvar_t	*r_fontForceAutoHint;

qboolean	R_GetModeInfo( int *width, int *height, float *windowAspect, int mode );

float R_NoiseGet4f( float x, float y, float z, double t );
int R_RandomOn( double t );
void  R_NoiseInit( void );

void	R_LoadImage( const char *name, int *numLevels, textureLevel_t **pic );
image_t	*R_FindImageFile( const char *name, imgType_t type, imgFlags_t flags );
image_t *R_CreateImage( const char *name, byte *pic, int width, int height, imgType_t type, imgFlags_t flags, int internalFormat );
image_t *R_CreateImage2( const char *name, int numTexLevels, const textureLevel_t *pic, imgType_t type, imgFlags_t flags, int internalFormat );

void R_IssuePendingRenderCommands( void );
qhandle_t		 RE_RegisterShaderEx( const char *name, int lightmapIndex, qboolean mipRawImage );
qhandle_t		 RE_RegisterShader( const char *name );
qhandle_t		 RE_RegisterShaderNoMip( const char *name );
qhandle_t		 RE_RegisterShaderNoPicMip( const char *name );
qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage);

// font stuff
void R_InitFreeType( void );
void R_DoneFreeType( void );
void RE_RegisterFont(const char *fontName, int pointSize, float borderWidth, qboolean forceAutoHint, fontInfo_t *vmFont, int vmFontBufSize);
void R_FontList_f( void );

/*
=============================================================

IMAGE LOADERS

=============================================================
*/

// pic points to an array of numLevels textureLevel_t structs that describe
// the individual mip levels, the format is either GL_RGBA8 or a compressed
// internalformat if the data is precompressed.

void R_LoadBMP( const char *name, int *numLevels, textureLevel_t **pic );
void R_LoadDDS( const char *name, int *numLevels, textureLevel_t **pic );
void R_LoadFTX( const char *name, int *numLevels, textureLevel_t **pic );
void R_LoadJPG( const char *name, int *numLevels, textureLevel_t **pic );
void R_LoadPCX( const char *name, int *numLevels, textureLevel_t **pic );
void R_LoadPNG( const char *name, int *numLevels, textureLevel_t **pic );
void R_LoadTGA( const char *name, int *numLevels, textureLevel_t **pic );

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_Init( qboolean );
void		GLimp_Shutdown( void );
void		GLimp_EndFrame( void );

void		GLimp_LogComment( char *comment );
void		GLimp_Minimize(void);

void		GLimp_SetGamma( unsigned char red[256],
		unsigned char green[256],
		unsigned char blue[256] );

qboolean	GLimp_ResizeWindow( int width, int height );

#endif
