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

#ifndef __MINT_BSP__
#define __MINT_BSP__

/*

	Abstract BSP structures

*/

typedef struct {
	float		mins[3], maxs[3];
	int			firstSurface, numSurfaces;
	int			firstBrush, numBrushes;
} dmodel_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			surfaceFlags;
	int			contentFlags;
} dshader_t;

// planes x^1 is allways the opposite of plane x

typedef struct {
	float		normal[3];
	float		dist;
} dplane_t;

typedef struct {
	int			planeNum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	int			mins[3];		// for frustom culling
	int			maxs[3];
} dnode_t;

typedef struct {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;

	int			mins[3];			// for frustum culling
	int			maxs[3];

	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;
} dleaf_t;

typedef struct {
	int			planeNum;			// positive plane side faces out of the leaf
	int			shaderNum;
	int			surfaceNum;
} dbrushside_t;

typedef struct {
	int			firstSide;
	int			numSides;
	int			shaderNum;		// the shader that determines the contents flags
} dbrush_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} drawVert_t;

#define drawVert_t_cleared(x) drawVert_t (x) = {{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0, 0}}

typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE,
	MST_FOLIAGE, // 5 in ET
	MST_TERRAIN, // 5 in EF2 // triangle soup that is solid
	MST_MAX
} mapSurfaceType_t;

typedef struct {
	int			shaderNum;
	int			fogNum;
	int			surfaceType;

	int			firstVert;
	int			numVerts; // ydnar: num verts + foliage origins (for cleaner lighting code in q3map)

	int			firstIndex;
	int			numIndexes;

	int			lightmapNum;
	int			lightmapX, lightmapY;
	int			lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	vec3_t		lightmapVecs[3];	// for patches, [0] and [1] are lodbounds

	int			patchWidth; // ydnar: num foliage instances
	int			patchHeight; // ydnar: num foliage mesh verts

	float		subdivisions; // patch collision subdivisions
} dsurface_t;

typedef struct {
	char			name[MAX_QPATH];
	int				checksum;
	int				references;

	char			*entityString;
	int				entityStringLength;

	dshader_t		*shaders;
	int				numShaders;

	dplane_t		*planes;
	int				numPlanes;

	dnode_t			*nodes;
	int				numNodes;

	dleaf_t			*leafs;
	int				numLeafs;

	int				*leafSurfaces;
	int				numLeafSurfaces;

	int				*leafBrushes;
	int				numLeafBrushes;

	dmodel_t		*submodels;
	int				numSubmodels;

	dbrush_t		*brushes;
	int				numBrushes;

	dbrushside_t	*brushSides;
	int				numBrushSides;

	drawVert_t		*drawVerts;
	int				numDrawVerts;

	int				*drawIndexes;
	int				numDrawIndexes;

	dfog_t			*fogs;
	int				numFogs;

	dsurface_t		*surfaces;
	int				numSurfaces;

	byte			*lightmapData;
	int				numLightmaps;

	float			defaultLightGridSize[3]; // might be overriden in entities string
	byte			*lightGridData; // each point is 8 bytes
	int				numGridPoints;

	unsigned short	*lightGridArray;
	int				numGridArrayPoints;

	int				numClusters;
	int				clusterBytes;
	byte			*visibility;
	int				visibilityLength;

} bspFile_t;

//
bspFile_t *BSP_Load( const char *name );
void BSP_Free( bspFile_t *bspFile );
void BSP_Shutdown( void );
void BSP_SwapBlock( int *dest, const int *src, int size );


/*

	BSP Formats

*/

typedef struct bspFormat_s {
	const char *gameName;
	int			ident;
	int			version;
	bspFile_t	*(*loadFunction)( const struct bspFormat_s *format, const char *name, const void *data, int length );
	int			(*saveFunction)( const struct bspFormat_s *format, const char *name, const bspFile_t *bsp, void **dataOut );
} bspFormat_t;

// bsp_q3.c
extern bspFormat_t quake3BspFormat;
extern bspFormat_t wolfBspFormat;
extern bspFormat_t darksBspFormat;

// bsp_q3test106.c
extern bspFormat_t q3Test106BspFormat;

// bsp_fakk.c
extern bspFormat_t fakkBspFormat;
extern bspFormat_t aliceBspFormat;

// bsp_sof2.c
extern bspFormat_t sof2BspFormat;

// bsp_ef2.c
extern bspFormat_t ef2BspFormat;

// bsp_mohaa.c
extern bspFormat_t mohaaBspFormat;

#endif // __MINT_BSP__

