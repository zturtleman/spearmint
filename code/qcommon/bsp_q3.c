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
// bsp_q3.c -- Q3/RTCW/ET/EF BSP Level Loading

#include "q_shared.h"
#include "qcommon.h"
#include "bsp.h"

#define BSP_IDENT	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
		// little-endian "IBSP"

#define Q3_BSP_VERSION			46 // Quake III / Team Arena
#define WOLF_BSP_VERSION		47 // RTCW / WolfET
#define DARKS_BSP_VERSION		666 // Dark Salvation

typedef struct {
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_SHADERS		1
#define	LUMP_PLANES			2
#define	LUMP_NODES			3
#define	LUMP_LEAFS			4
#define	LUMP_LEAFSURFACES	5
#define	LUMP_LEAFBRUSHES	6
#define	LUMP_MODELS			7
#define	LUMP_BRUSHES		8
#define	LUMP_BRUSHSIDES		9
#define	LUMP_DRAWVERTS		10
#define	LUMP_DRAWINDEXES	11
#define	LUMP_FOGS			12
#define	LUMP_SURFACES		13
#define	LUMP_LIGHTMAPS		14
#define	LUMP_LIGHTGRID		15
#define	LUMP_VISIBILITY		16
#define	HEADER_LUMPS		17

typedef struct {
	int			ident;
	int			version;

	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
	float		mins[3], maxs[3];
	int			firstSurface, numSurfaces;
	int			firstBrush, numBrushes;
} realDmodel_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			surfaceFlags;
	int			contentFlags;
} realDshader_t;

// planes x^1 is allways the opposite of plane x

typedef struct {
	float		normal[3];
	float		dist;
} realDplane_t;

typedef struct {
	int			planeNum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	int			mins[3];		// for frustom culling
	int			maxs[3];
} realDnode_t;

typedef struct {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;

	int			mins[3];			// for frustum culling
	int			maxs[3];

	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;
} realDleaf_t;

typedef struct {
	int			planeNum;			// positive plane side faces out of the leaf
	int			shaderNum;
} realDbrushside_t;

typedef struct {
	int			firstSide;
	int			numSides;
	int			shaderNum;		// the shader that determines the contents flags
} realDbrush_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} realDfog_t;

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} realDrawVert_t;

// When adding a new BSP format make sure the surfaceType variables mean the same thing as Q3 or remap them on load!
#if 0
typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE,
	MST_FOLIAGE
} mapSurfaceType_t;
#endif

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
} realDsurface_t;

#define VIS_HEADER 8

#define LIGHTING_GRIDSIZE_X 64
#define LIGHTING_GRIDSIZE_Y 64
#define LIGHTING_GRIDSIZE_Z 128

#define	SUBDIVIDE_DISTANCE	16	//4	// never more than this units away from curve

/****************************************************
*/

static int GetLumpElements( dheader_t *header, int lump, int size ) {
	/* check for odd size */
	if ( header->lumps[ lump ].filelen % size ) {
		Com_Printf( "GetLumpElements: odd lump size (%d) in lump %d\n", header->lumps[ lump ].filelen, lump );
		return 0;
	}

	/* return element count */
	return header->lumps[ lump ].filelen / size;
}

// Read data from lump
static void CopyLump( dheader_t *header, int lump, const void *src, void *dest, int size, qboolean swap ) {
	int length;

	length = GetLumpElements( header, lump, size ) * size;

	/* handle erroneous cases */
	if ( length <= 0 ) {
		return;
	}

	if ( swap ) {
		BSP_SwapBlock( dest, (int *)((byte*) src + header->lumps[lump].fileofs), length );
	} else {
		Com_Memcpy( dest, (byte*) src + header->lumps[lump].fileofs, length );
	}
}

// Write data to lump
static void WriteLump( dheader_t *header, int lump, void *dest, const void *src, int size, qboolean swap ) {
	int length;

	length = GetLumpElements( header, lump, size ) * size;

	/* handle erroneous cases */
	if ( length <= 0 ) {
		return;
	}

	if ( swap ) {
		BSP_SwapBlock( (int *)((byte*) dest + header->lumps[lump].fileofs), src, length );
	} else {
		Com_Memcpy( (byte*) dest + header->lumps[lump].fileofs, src, length );
	}
}

static void AddLump( dheader_t *header, int *filePos, int lump, int elements, int size ) {
	header->lumps[lump].fileofs = *filePos;
	header->lumps[lump].filelen = elements * size;

	*filePos += elements * size;
}

static void *GetLump( dheader_t *header, const void *src, int lump ) {
	return (void*)( (byte*) src + header->lumps[ lump ].fileofs );
}

/****************************************************
*/

bspFile_t *BSP_LoadQ3( const bspFormat_t *format, const char *name, const void *data, int length ) {
	int				i, j, k;
	dheader_t		header;
	bspFile_t		*bsp;

	BSP_SwapBlock( (int *) &header, (int *)data, sizeof ( dheader_t ) );

	if ( header.ident != format->ident || header.version != format->version ) {
		return NULL;
	}

	bsp = malloc( sizeof ( bspFile_t ) );
	Com_Memset( bsp, 0, sizeof ( bspFile_t ) );

	// ...
	bsp->checksum = LittleLong (Com_BlockChecksum (data, length));
	bsp->defaultLightGridSize[0] = LIGHTING_GRIDSIZE_X;
	bsp->defaultLightGridSize[1] = LIGHTING_GRIDSIZE_Y;
	bsp->defaultLightGridSize[2] = LIGHTING_GRIDSIZE_Z;


	//
	// count and alloc
	//
	bsp->entityStringLength = GetLumpElements( &header, LUMP_ENTITIES, 1 );
	bsp->entityString = malloc( bsp->entityStringLength );

	bsp->numShaders = GetLumpElements( &header, LUMP_SHADERS, sizeof ( realDshader_t ) );
	bsp->shaders = malloc( bsp->numShaders * sizeof ( *bsp->shaders ) );

	bsp->numPlanes = GetLumpElements( &header, LUMP_PLANES, sizeof ( realDplane_t ) );
	bsp->planes = malloc( bsp->numPlanes * sizeof ( *bsp->planes ) );

	bsp->numNodes = GetLumpElements( &header, LUMP_NODES, sizeof ( realDnode_t ) );
	bsp->nodes = malloc( bsp->numNodes * sizeof ( *bsp->nodes ) );

	bsp->numLeafs = GetLumpElements( &header, LUMP_LEAFS, sizeof ( realDleaf_t ) );
	bsp->leafs = malloc( bsp->numLeafs * sizeof ( *bsp->leafs ) );

	bsp->numLeafSurfaces = GetLumpElements( &header, LUMP_LEAFSURFACES, sizeof ( int ) );
	bsp->leafSurfaces = malloc( bsp->numLeafSurfaces * sizeof ( *bsp->leafSurfaces ) );

	bsp->numLeafBrushes = GetLumpElements( &header, LUMP_LEAFBRUSHES, sizeof ( int ) );
	bsp->leafBrushes = malloc( bsp->numLeafBrushes * sizeof ( *bsp->leafBrushes ) );

	bsp->numSubmodels = GetLumpElements( &header, LUMP_MODELS, sizeof ( realDmodel_t ) );
	bsp->submodels = malloc( bsp->numSubmodels * sizeof ( *bsp->submodels ) );

	bsp->numBrushes = GetLumpElements( &header, LUMP_BRUSHES, sizeof ( realDbrush_t ) );
	bsp->brushes = malloc( bsp->numBrushes * sizeof ( *bsp->brushes ) );

	bsp->numBrushSides = GetLumpElements( &header, LUMP_BRUSHSIDES, sizeof ( realDbrushside_t ) );
	bsp->brushSides = malloc( bsp->numBrushSides * sizeof ( *bsp->brushSides ) );

	bsp->numDrawVerts = GetLumpElements( &header, LUMP_DRAWVERTS, sizeof ( realDrawVert_t ) );
	bsp->drawVerts = malloc( bsp->numDrawVerts * sizeof ( *bsp->drawVerts ) );

	bsp->numDrawIndexes = GetLumpElements( &header, LUMP_DRAWINDEXES, sizeof ( int ) );
	bsp->drawIndexes = malloc( bsp->numDrawIndexes * sizeof ( *bsp->drawIndexes ) );

	bsp->numFogs = GetLumpElements( &header, LUMP_FOGS, sizeof ( realDfog_t ) );
	bsp->fogs = malloc( bsp->numFogs * sizeof ( *bsp->fogs ) );

	bsp->numSurfaces = GetLumpElements( &header, LUMP_SURFACES, sizeof ( realDsurface_t ) );
	bsp->surfaces = malloc( bsp->numSurfaces * sizeof ( *bsp->surfaces ) );

	bsp->numLightmaps = GetLumpElements( &header, LUMP_LIGHTMAPS, 128 * 128 * 3 );
	bsp->lightmapData = malloc( bsp->numLightmaps * 128 * 128 * 3 );

	bsp->numGridPoints = GetLumpElements( &header, LUMP_LIGHTGRID, 8 );
	bsp->lightGridData = malloc( bsp->numGridPoints * 8 );

	bsp->visibilityLength = GetLumpElements( &header, LUMP_VISIBILITY, 1 ) - VIS_HEADER;
	if ( bsp->visibilityLength > 0 )
		bsp->visibility = malloc( bsp->visibilityLength );
	else
		bsp->visibilityLength = 0;

	//
	// copy and swap and convert data
	//
	CopyLump( &header, LUMP_ENTITIES, data, (void *) bsp->entityString, sizeof ( *bsp->entityString ), qfalse ); /* NO SWAP */

	{
		realDshader_t *in = GetLump( &header, data, LUMP_SHADERS );
		dshader_t *out = bsp->shaders;

		for ( i = 0; i < bsp->numShaders; i++, in++, out++ ) {
			Q_strncpyz( out->shader, in->shader, sizeof ( out->shader ) );
			out->contentFlags = LittleLong( in->contentFlags );
			out->surfaceFlags = LittleLong( in->surfaceFlags );
		}
	}

	{
		realDplane_t *in = GetLump( &header, data, LUMP_PLANES );
		dplane_t *out = bsp->planes;

		for ( i = 0; i < bsp->numPlanes; i++, in++, out++) {
			for (j=0 ; j<3 ; j++) {
				out->normal[j] = LittleFloat (in->normal[j]);
			}

			out->dist = LittleFloat (in->dist);
		}
	}

	{
		realDnode_t *in = GetLump( &header, data, LUMP_NODES );
		dnode_t *out = bsp->nodes;

		for ( i = 0; i < bsp->numNodes; i++, in++, out++ ) {
			out->planeNum = LittleLong( in->planeNum );

			for ( j = 0; j < 2; j++ ) {
				out->children[j] = LittleLong( in->children[j] );
			}

			for ( j = 0; j < 3; j++ ) {
				out->mins[j] = LittleLong( in->mins[j] );
				out->maxs[j] = LittleLong( in->maxs[j] );
			}
		}
	}

	{
		realDleaf_t *in = GetLump( &header, data, LUMP_LEAFS );
		dleaf_t *out = bsp->leafs;

		for ( i = 0; i < bsp->numLeafs; i++, in++, out++ ) {
			out->cluster = LittleLong (in->cluster);
			out->area = LittleLong (in->area);

			for ( j = 0; j < 3; j++ ) {
				out->mins[j] = LittleLong( in->mins[j] );
				out->maxs[j] = LittleLong( in->maxs[j] );
			}

			out->firstLeafBrush = LittleLong (in->firstLeafBrush);
			out->numLeafBrushes = LittleLong (in->numLeafBrushes);
			out->firstLeafSurface = LittleLong (in->firstLeafSurface);
			out->numLeafSurfaces = LittleLong (in->numLeafSurfaces);
		}
	}

	CopyLump( &header, LUMP_LEAFSURFACES, data, (void *) bsp->leafSurfaces, sizeof ( *bsp->leafSurfaces ), qtrue );
	CopyLump( &header, LUMP_LEAFBRUSHES, data, (void *) bsp->leafBrushes, sizeof ( *bsp->leafBrushes ), qtrue );

	{
		realDmodel_t *in = GetLump( &header, data, LUMP_MODELS );
		dmodel_t *out = bsp->submodels;

		for ( i = 0; i < bsp->numSubmodels; i++, in++, out++ ) {
			for ( j = 0; j < 3; j++ ) {
				out->mins[j] = LittleFloat( in->mins[j] );
				out->maxs[j] = LittleFloat( in->maxs[j] );
			}

			out->firstSurface = LittleLong (in->firstSurface);
			out->numSurfaces = LittleLong (in->numSurfaces);
			out->firstBrush = LittleLong (in->firstBrush);
			out->numBrushes = LittleLong (in->numBrushes);
		}
	}

	{
		realDbrush_t *in = GetLump( &header, data, LUMP_BRUSHES );
		dbrush_t *out = bsp->brushes;

		for ( i = 0; i < bsp->numBrushes; i++, in++, out++ )
		{
			out->firstSide = LittleLong (in->firstSide);
			out->numSides = LittleLong (in->numSides);
			out->shaderNum = LittleLong (in->shaderNum);
		}
	}

	{
		realDbrushside_t *in = GetLump( &header, data, LUMP_BRUSHSIDES );
		dbrushside_t *out = bsp->brushSides;

		for ( i = 0; i < bsp->numBrushSides; i++, in++, out++ ) {
			out->planeNum = LittleLong (in->planeNum);
			out->shaderNum = LittleLong (in->shaderNum);
			out->surfaceNum = -1;
		}
	}

	{
		realDrawVert_t *in = GetLump( &header, data, LUMP_DRAWVERTS );
		drawVert_t *out = bsp->drawVerts;

		for ( i = 0; i < bsp->numDrawVerts; i++, in++, out++ ) {
			for ( j = 0 ; j < 3 ; j++ ) {
				out->xyz[j] = LittleFloat( in->xyz[j] );
				out->normal[j] = LittleFloat( in->normal[j] );
			}
			for ( j = 0 ; j < 2 ; j++ ) {
				out->st[j] = LittleFloat( in->st[j] );
				out->lightmap[j] = LittleFloat( in->lightmap[j] );
			}

			/* NO SWAP */
			for ( j = 0; j < 4; j++ ) {
				out->color[j] = in->color[j];
			}
		}
	}

	CopyLump( &header, LUMP_DRAWINDEXES, data, (void *) bsp->drawIndexes, sizeof ( *bsp->drawIndexes ), qtrue );

	{
		realDfog_t *in = GetLump( &header, data, LUMP_FOGS );
		dfog_t *out = bsp->fogs;

		for ( i = 0; i < bsp->numFogs; i++, in++, out++ ) {
			Q_strncpyz( out->shader, in->shader, sizeof ( out->shader ) );
			out->brushNum = LittleLong (in->brushNum);
			out->visibleSide = LittleLong (in->visibleSide);
		}
	}

	{
		realDsurface_t *in = GetLump( &header, data, LUMP_SURFACES );
		dsurface_t *out = bsp->surfaces;

		for ( i = 0; i < bsp->numSurfaces; i++, in++, out++ ) {
			out->shaderNum = LittleLong (in->shaderNum);
			out->fogNum = LittleLong (in->fogNum);
			out->surfaceType = LittleLong (in->surfaceType);
			out->firstVert = LittleLong (in->firstVert);
			out->numVerts = LittleLong (in->numVerts);
			out->firstIndex = LittleLong (in->firstIndex);
			out->numIndexes = LittleLong (in->numIndexes);
			out->lightmapNum = LittleLong (in->lightmapNum);
			out->lightmapX = LittleLong (in->lightmapX);
			out->lightmapY = LittleLong (in->lightmapY);
			out->lightmapWidth = LittleLong (in->lightmapWidth);
			out->lightmapHeight = LittleLong (in->lightmapHeight);

			for ( j = 0; j < 3; j++ ) {
				out->lightmapOrigin[j] = LittleFloat( in->lightmapOrigin[j] );
				for ( k = 0; k < 3; k++ ) {
					out->lightmapVecs[j][k] = LittleFloat( in->lightmapVecs[j][k] );
				}
			}

			out->patchWidth = LittleLong (in->patchWidth);
			out->patchHeight = LittleLong (in->patchHeight);

			out->subdivisions = SUBDIVIDE_DISTANCE;
		}
	}

	CopyLump( &header, LUMP_LIGHTMAPS, data, (void *) bsp->lightmapData, sizeof ( *bsp->lightmapData ), qfalse ); /* NO SWAP */
	CopyLump( &header, LUMP_LIGHTGRID, data, (void *) bsp->lightGridData, sizeof ( *bsp->lightGridData ), qfalse ); /* NO SWAP */

	if ( bsp->visibilityLength )
	{
		byte *in = GetLump( &header, data, LUMP_VISIBILITY );

		bsp->numClusters = LittleLong( ((int *)in)[0] );
		bsp->clusterBytes = LittleLong( ((int *)in)[1] );

		Com_Memcpy( bsp->visibility, in + VIS_HEADER, bsp->visibilityLength ); /* NO SWAP */
	}

	return bsp;
}

// convert internal BSP format to BSP for saving to disk
// ZTM: TODO: convert ET foliage surfaces if Q3 format? how to check if Q3 or RTCW and not ET?
// ZTM: TODO: handle different default light grid sizes?
int BSP_SaveQ3( const bspFormat_t *format, const char *name, const bspFile_t *bsp, void **dataOut ) {
	int				i, j, k;
	dheader_t		header;
	byte			*data;
	int				dataLength;
	int				numGridPoints;

	*dataOut = NULL;

#if 0
	// ...
	bsp->checksum = LittleLong (Com_BlockChecksum (data, length));
	bsp->defaultLightGridSize[0] = LIGHTING_GRIDSIZE_X;
	bsp->defaultLightGridSize[1] = LIGHTING_GRIDSIZE_Y;
	bsp->defaultLightGridSize[2] = LIGHTING_GRIDSIZE_Z;
#endif

	if ( bsp->numGridArrayPoints ) {
		numGridPoints = bsp->numGridArrayPoints;
	} else {
		numGridPoints = bsp->numGridPoints;
	}

	//
	// setup header
	//
	Com_Memset( &header, 0, sizeof ( dheader_t ) );
	header.ident = format->ident;
	header.version = format->version;

	dataLength = sizeof( dheader_t );

	AddLump( &header, &dataLength, LUMP_ENTITIES, bsp->entityStringLength, 1 );
	AddLump( &header, &dataLength, LUMP_SHADERS, bsp->numShaders, sizeof ( realDshader_t ) );
	AddLump( &header, &dataLength, LUMP_PLANES, bsp->numPlanes, sizeof ( realDplane_t ) );
	AddLump( &header, &dataLength, LUMP_NODES, bsp->numNodes, sizeof ( realDnode_t ) );
	AddLump( &header, &dataLength, LUMP_LEAFS, bsp->numLeafs, sizeof ( realDleaf_t ) );
	AddLump( &header, &dataLength, LUMP_LEAFSURFACES, bsp->numLeafSurfaces, sizeof ( int ) );
	AddLump( &header, &dataLength, LUMP_LEAFBRUSHES, bsp->numLeafBrushes, sizeof ( int ) );
	AddLump( &header, &dataLength, LUMP_MODELS, bsp->numSubmodels, sizeof ( realDmodel_t ) );
	AddLump( &header, &dataLength, LUMP_BRUSHES, bsp->numBrushes, sizeof ( realDbrush_t ) );
	AddLump( &header, &dataLength, LUMP_BRUSHSIDES, bsp->numBrushSides, sizeof ( realDbrushside_t ) );
	AddLump( &header, &dataLength, LUMP_DRAWVERTS, bsp->numDrawVerts, sizeof ( realDrawVert_t ) );
	AddLump( &header, &dataLength, LUMP_DRAWINDEXES, bsp->numDrawIndexes, sizeof ( int ) );
	AddLump( &header, &dataLength, LUMP_FOGS, bsp->numFogs, sizeof ( realDfog_t ) );
	AddLump( &header, &dataLength, LUMP_SURFACES, bsp->numSurfaces, sizeof ( realDsurface_t ) );
	AddLump( &header, &dataLength, LUMP_LIGHTMAPS, bsp->numLightmaps, 128 * 128 * 3 );
	AddLump( &header, &dataLength, LUMP_LIGHTGRID, numGridPoints, 8 );
	if ( bsp->visibilityLength ) {
		AddLump( &header, &dataLength, LUMP_VISIBILITY, bsp->visibilityLength + VIS_HEADER, 1 );
	}

	data = malloc( dataLength );

	//
	// copy and swap and convert data
	//
	WriteLump( &header, LUMP_ENTITIES, data, (void *) bsp->entityString, sizeof ( *bsp->entityString ), qfalse ); /* NO SWAP */

	{
		realDshader_t *out = GetLump( &header, data, LUMP_SHADERS );
		dshader_t *in = bsp->shaders;

		for ( i = 0; i < bsp->numShaders; i++, in++, out++ ) {
			Q_strncpyz( out->shader, in->shader, sizeof ( out->shader ) );
			out->contentFlags = LittleLong( in->contentFlags );
			out->surfaceFlags = LittleLong( in->surfaceFlags );
		}
	}

	{
		realDplane_t *out = GetLump( &header, data, LUMP_PLANES );
		dplane_t *in = bsp->planes;

		for ( i = 0; i < bsp->numPlanes; i++, in++, out++) {
			for (j=0 ; j<3 ; j++) {
				out->normal[j] = LittleFloat (in->normal[j]);
			}

			out->dist = LittleFloat (in->dist);
		}
	}

	{
		realDnode_t *out = GetLump( &header, data, LUMP_NODES );
		dnode_t *in = bsp->nodes;

		for ( i = 0; i < bsp->numNodes; i++, in++, out++ ) {
			out->planeNum = LittleLong( in->planeNum );

			for ( j = 0; j < 2; j++ ) {
				out->children[j] = LittleLong( in->children[j] );
			}

			for ( j = 0; j < 3; j++ ) {
				out->mins[j] = LittleLong( in->mins[j] );
				out->maxs[j] = LittleLong( in->maxs[j] );
			}
		}
	}

	{
		realDleaf_t *out = GetLump( &header, data, LUMP_LEAFS );
		dleaf_t *in = bsp->leafs;

		for ( i = 0; i < bsp->numLeafs; i++, in++, out++ ) {
			out->cluster = LittleLong (in->cluster);
			out->area = LittleLong (in->area);

			for ( j = 0; j < 3; j++ ) {
				out->mins[j] = LittleLong( in->mins[j] );
				out->maxs[j] = LittleLong( in->maxs[j] );
			}

			out->firstLeafBrush = LittleLong (in->firstLeafBrush);
			out->numLeafBrushes = LittleLong (in->numLeafBrushes);
			out->firstLeafSurface = LittleLong (in->firstLeafSurface);
			out->numLeafSurfaces = LittleLong (in->numLeafSurfaces);
		}
	}

	WriteLump( &header, LUMP_LEAFSURFACES, data, (void *) bsp->leafSurfaces, sizeof ( *bsp->leafSurfaces ), qtrue );
	WriteLump( &header, LUMP_LEAFBRUSHES, data, (void *) bsp->leafBrushes, sizeof ( *bsp->leafBrushes ), qtrue );

	{
		realDmodel_t *out = GetLump( &header, data, LUMP_MODELS );
		dmodel_t *in = bsp->submodels;

		for ( i = 0; i < bsp->numSubmodels; i++, in++, out++ ) {
			for ( j = 0; j < 3; j++ ) {
				out->mins[j] = LittleFloat( in->mins[j] );
				out->maxs[j] = LittleFloat( in->maxs[j] );
			}

			out->firstSurface = LittleLong (in->firstSurface);
			out->numSurfaces = LittleLong (in->numSurfaces);
			out->firstBrush = LittleLong (in->firstBrush);
			out->numBrushes = LittleLong (in->numBrushes);
		}
	}

	{
		realDbrush_t *out = GetLump( &header, data, LUMP_BRUSHES );
		dbrush_t *in = bsp->brushes;

		for ( i = 0; i < bsp->numBrushes; i++, in++, out++ )
		{
			out->firstSide = LittleLong (in->firstSide);
			out->numSides = LittleLong (in->numSides);
			out->shaderNum = LittleLong (in->shaderNum);
		}
	}

	{
		realDbrushside_t *out = GetLump( &header, data, LUMP_BRUSHSIDES );
		dbrushside_t *in = bsp->brushSides;

		for ( i = 0; i < bsp->numBrushSides; i++, in++, out++ ) {
			out->planeNum = LittleLong (in->planeNum);
			out->shaderNum = LittleLong (in->shaderNum);
#if 0 // NOT_SAVED
			out->surfaceNum = -1;
#endif
		}
	}

	{
		realDrawVert_t *out = GetLump( &header, data, LUMP_DRAWVERTS );
		drawVert_t *in = bsp->drawVerts;

		for ( i = 0; i < bsp->numDrawVerts; i++, in++, out++ ) {
			for ( j = 0 ; j < 3 ; j++ ) {
				out->xyz[j] = LittleFloat( in->xyz[j] );
				out->normal[j] = LittleFloat( in->normal[j] );
			}
			for ( j = 0 ; j < 2 ; j++ ) {
				out->st[j] = LittleFloat( in->st[j] );
				out->lightmap[j] = LittleFloat( in->lightmap[j] );
			}

			/* NO SWAP */
			for ( j = 0; j < 4; j++ ) {
				out->color[j] = in->color[j];
			}
		}
	}

	WriteLump( &header, LUMP_DRAWINDEXES, data, (void *) bsp->drawIndexes, sizeof ( *bsp->drawIndexes ), qtrue );

	{
		realDfog_t *out = GetLump( &header, data, LUMP_FOGS );
		dfog_t *in = bsp->fogs;

		for ( i = 0; i < bsp->numFogs; i++, in++, out++ ) {
			Q_strncpyz( out->shader, in->shader, sizeof ( out->shader ) );
			out->brushNum = LittleLong (in->brushNum);
			out->visibleSide = LittleLong (in->visibleSide);
		}
	}

	{
		realDsurface_t *out = GetLump( &header, data, LUMP_SURFACES );
		dsurface_t *in = bsp->surfaces;

		for ( i = 0; i < bsp->numSurfaces; i++, in++, out++ ) {
			out->shaderNum = LittleLong (in->shaderNum);
			out->fogNum = LittleLong (in->fogNum);
			out->surfaceType = LittleLong (in->surfaceType);
			out->firstVert = LittleLong (in->firstVert);
			out->numVerts = LittleLong (in->numVerts);
			out->firstIndex = LittleLong (in->firstIndex);
			out->numIndexes = LittleLong (in->numIndexes);
			out->lightmapNum = LittleLong (in->lightmapNum);
			out->lightmapX = LittleLong (in->lightmapX);
			out->lightmapY = LittleLong (in->lightmapY);
			out->lightmapWidth = LittleLong (in->lightmapWidth);
			out->lightmapHeight = LittleLong (in->lightmapHeight);

			for ( j = 0; j < 3; j++ ) {
				out->lightmapOrigin[j] = LittleFloat( in->lightmapOrigin[j] );
				for ( k = 0; k < 3; k++ ) {
					out->lightmapVecs[j][k] = LittleFloat( in->lightmapVecs[j][k] );
				}
			}

			out->patchWidth = LittleLong (in->patchWidth);
			out->patchHeight = LittleLong (in->patchHeight);

#if 0 // NOT_SAVED
			out->subdivisions = SUBDIVIDE_DISTANCE;
#endif
		}
	}

	WriteLump( &header, LUMP_LIGHTMAPS, data, (void *) bsp->lightmapData, sizeof ( *bsp->lightmapData ), qfalse ); /* NO SWAP */

	if ( bsp->numGridArrayPoints ) {
		byte *out = GetLump( &header, data, LUMP_LIGHTGRID );
		unsigned short *in = bsp->lightGridArray;

		for ( i = 0; i < bsp->numGridArrayPoints; i++, in++, out += 8 ) {
			Com_Memcpy( out, (byte*)&bsp->lightGridData[(*in) * 8], 8 ); /* NO SWAP */
		}
	} else {
		WriteLump( &header, LUMP_LIGHTGRID, data, (void *) bsp->lightGridData, sizeof ( *bsp->lightGridData ), qfalse ); /* NO SWAP */
	}

	if ( bsp->visibilityLength )
	{
		byte *out = GetLump( &header, data, LUMP_VISIBILITY );

		((int *)out)[0] = LittleLong( bsp->numClusters );
		((int *)out)[1] = LittleLong( bsp->clusterBytes );

		Com_Memcpy( out + VIS_HEADER, bsp->visibility, bsp->visibilityLength ); /* NO SWAP */
	}

	BSP_SwapBlock( (int *)data, (int *)&header, sizeof ( dheader_t ) );

	*dataOut = data;
	return dataLength;
}


/****************************************************
*/

// Q3, Elite Force, and other games
bspFormat_t quake3BspFormat = {
	"Quake3",
	BSP_IDENT,
	Q3_BSP_VERSION,
	BSP_LoadQ3,
	BSP_SaveQ3,
};

// RTCW, ET, QuakeLive
bspFormat_t wolfBspFormat = {
	"RTCW/ET",
	BSP_IDENT,
	WOLF_BSP_VERSION,
	BSP_LoadQ3,
	BSP_SaveQ3,
};

// Dark Salvation
bspFormat_t darksBspFormat = {
	"DarkSalvation",
	BSP_IDENT,
	DARKS_BSP_VERSION,
	BSP_LoadQ3,
	BSP_SaveQ3,
};

