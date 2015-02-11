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
// bsp_mohaa.c -- MoHAA BSP Level Loading, based on bsp_fakk.c and OpenMoHAA

#include "q_shared.h"
#include "qcommon.h"
#include "bsp.h"

// Implementation notes
// - Missing light grid support
// - Missing terrain, sphere lights, static models (supported by OpenMoHAA)

#define BSP_IDENT	(('5'<<24)+('1'<<16)+('0'<<8)+'2')
		// little-endian "2015"

#define BSP_VERSION	19

typedef struct {
	int		fileofs, filelen;
} lump_t;

#define	LUMP_SHADERS		0
#define	LUMP_PLANES			1
#define	LUMP_LIGHTMAPS		2
#define	LUMP_SURFACES		3
#define	LUMP_DRAWVERTS		4
#define	LUMP_DRAWINDEXES	5
#define	LUMP_LEAFBRUSHES	6
#define	LUMP_LEAFSURFACES	7
#define	LUMP_LEAFS			8
#define	LUMP_NODES			9
//#define	LUMP_SIDEEQUATIONS	10
#define	LUMP_BRUSHSIDES		11
#define	LUMP_BRUSHES		12
#define	LUMP_MODELS			13
#define	LUMP_ENTITIES		14
#define	LUMP_VISIBILITY		15
//#define LUMP_LIGHTGRIDPALETTE	16
//#define LUMP_LIGHTGRIDOFFSETS	17
//#define LUMP_LIGHTGRIDDATA		18
//#define LUMP_SPHERELIGHTS		19
//#define LUMP_SPHERELIGHTVIS		20
#define LUMP_LIGHTDEFS			21
#define LUMP_TERRAIN			22
//#define LUMP_TERRAININDEXES		23
//#define LUMP_STATICMODELDATA	24
//#define LUMP_STATICMODELDEF		25
//#define LUMP_STATICMODELINDEXES	26
//#define LUMP_DUMMY10			27
#define	HEADER_LUMPS		28

typedef struct {
	int			ident;
	int			version;
	int			checksum;

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
	int			subdivisions;
	char		fenceMaskImage[MAX_QPATH]; // added for mohaa
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

	//added for mohaa
	int			dummy1;
	int			dummy2;
	int			firstStaticModel;
	int			numStaticModels;
} realDleaf_t;

#if 0
// su44: It seems that sideEquations
// are somehow related to fencemasks...
// MoHAA loads them only in CM (CM_LoadMap).
typedef struct {
  float fSeq[4];
  float fTeq[4];
} realDsideEquation_t;
#endif

typedef struct {
	int			planeNum;			// positive plane side faces out of the leaf
	int			shaderNum;
	int			equationNum;		// dsideEquation_t index
} realDbrushside_t;

typedef struct {
	int			firstSide;
	int			numSides;
	int			shaderNum;		// the shader that determines the contents flags
} realDbrush_t;

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

	float		subdivisions;
} realDsurface_t;

// IneQuation was here
typedef struct dterPatch_s {
	byte			flags;
	byte			scale;
	byte			lmCoords[2];
	float			texCoords[8];
	char			x;
	char			y;
	short			baseZ;
	unsigned short	shader;
	short			lightmap;
	short			dummy[4];
	short			vertFlags[2][63];
	byte			heightmap[9][9];
} realDterPatch_t;

#if 0
// su44 was here
typedef struct dstaticModel_s {
	char model[128];
	vec3_t origin;
	vec3_t angles;
	float scale;
	int firstVertexData;
	short numVertexData;
} dstaticModel_t;

typedef struct {
	float origin[3];
	float color[3];
	float intensity;
	int leaf;
	qboolean needs_trace;
	qboolean spot_light;
	float spot_dir[3];
	float spot_radiusbydistance;
} dspherel_t;

typedef struct {
	float origin[3];
	float axis[3];
	int bounds[3];
} dlightGrid_t;

typedef struct {
	int lightIntensity;
	int lightAngle;
	int lightmapResolution;
	qboolean twoSided;
	qboolean lightLinear;
	vec3_t lightColor;
	float lightFalloff;
	float backsplashFraction;
	float backsplashDistance;
	float lightSubdivide;
	qboolean autosprite;
} dlightdef_t;
#endif

#define VIS_HEADER 8

#define LIGHTING_GRIDSIZE_X 32
#define LIGHTING_GRIDSIZE_Y 32
#define LIGHTING_GRIDSIZE_Z 32

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

static void *GetLump( dheader_t *header, const void *src, int lump ) {
	return (void*)( (byte*) src + header->lumps[ lump ].fileofs );
}

bspFile_t *BSP_LoadMOHAA( const bspFormat_t *format, const char *name, const void *data, int length ) {
	int				i, j, k;
	dheader_t		header;
	bspFile_t		*bsp;
	int				numTerSurfaces, numTerVerts, numTerIndexes;

	BSP_SwapBlock( (int *) &header, (int *)data, sizeof ( dheader_t ) );

	if ( header.ident != format->ident || header.version != format->version ) {
		return NULL;
	}

	bsp = malloc( sizeof ( bspFile_t ) );
	Com_Memset( bsp, 0, sizeof ( bspFile_t ) );

	// ...
	bsp->checksum = header.checksum;
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

	numTerSurfaces = GetLumpElements( &header, LUMP_TERRAIN, sizeof ( realDterPatch_t ) );
	numTerVerts = numTerSurfaces * 9 * 9;
	numTerIndexes = numTerSurfaces * 8 * 8 * 6;

	bsp->numDrawVerts = GetLumpElements( &header, LUMP_DRAWVERTS, sizeof ( realDrawVert_t ) );
	bsp->drawVerts = malloc( ( bsp->numDrawVerts + numTerVerts ) * sizeof ( *bsp->drawVerts ) );

	bsp->numDrawIndexes = GetLumpElements( &header, LUMP_DRAWINDEXES, sizeof ( int ) );
	bsp->drawIndexes = malloc( ( bsp->numDrawIndexes + numTerIndexes ) * sizeof ( *bsp->drawIndexes ) );

	bsp->numFogs = 0; //GetLumpElements( &header, LUMP_FOGS, sizeof ( realDfog_t ) );
	bsp->fogs = NULL; //malloc( bsp->numFogs * sizeof ( *bsp->fogs ) );

	bsp->numSurfaces = GetLumpElements( &header, LUMP_SURFACES, sizeof ( realDsurface_t ) );
	bsp->surfaces = malloc( ( bsp->numSurfaces + numTerSurfaces ) * sizeof ( *bsp->surfaces ) );

	bsp->numLightmaps = GetLumpElements( &header, LUMP_LIGHTMAPS, 128 * 128 * 3 );
	bsp->lightmapData = malloc( bsp->numLightmaps * 128 * 128 * 3 );

#if 0 // ZTM: TODO: get light grid code from OpenMoHAA
	bsp->numGridPoints = GetLumpElements( &header, LUMP_LIGHTGRID, 8 );
	bsp->lightGridData = malloc( bsp->numGridPoints * 8 );
#endif

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

#if 0
	{
		realDfog_t *in = GetLump( &header, data, LUMP_FOGS );
		dfog_t *out = bsp->fogs;

		for ( i = 0; i < bsp->numFogs; i++, in++, out++ ) {
			Q_strncpyz( out->shader, in->shader, sizeof ( out->shader ) );
			out->brushNum = LittleLong (in->brushNum);
			out->visibleSide = LittleLong (in->visibleSide);
		}
	}
#endif

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

			out->subdivisions = LittleFloat( in->subdivisions );
		}
	}

	// convert MOHAA terrain into a draw surface
	// ZTM: FIXME: Add to nodes(?) so that they will actually draw/collide
#define LIGHTMAP_SIZE 128
#define TERRAIN_LM_LENGTH		(16.f / LIGHTMAP_SIZE)
	{
		realDterPatch_t *in = GetLump( &header, data, LUMP_TERRAIN );
		dsurface_t *out = &bsp->surfaces[ bsp->numSurfaces ];
		drawVert_t *vert;
		int x, y, ndx;
		float f, distU, distV, texCoords[8];

		for ( i = 0; i < numTerSurfaces; i++, in++, out++ ) {
			out->shaderNum = LittleShort (in->shader); // ZTM: FIXME: will this mess up unsigned short on big endian?
			out->fogNum = -1;
			out->surfaceType = MST_TERRAIN;	// solid triangle mesh
			out->firstVert = bsp->numDrawVerts + i * 9 * 9;
			out->numVerts = 9 * 9;
			out->firstIndex = bsp->numDrawIndexes + i * 8 * 8 * 6;
			out->numIndexes = 8 * 8 * 6;
			out->lightmapNum = LittleShort (in->lightmap);

			// lightmap x,y,width,height aren't used for anything
			out->lightmapX = 0;
			out->lightmapY = 0;
			out->lightmapWidth = 0;
			out->lightmapHeight = 0;

			out->lightmapOrigin[0] = in->x * 64.f;
			out->lightmapOrigin[1] = in->y * 64.f;
			out->lightmapOrigin[2] = LittleShort(in->baseZ);

			// ZTM: I don't think [0] and [1] will be used.
			VectorSet( out->lightmapVecs[0], 0, 0, 0 );
			VectorSet( out->lightmapVecs[1], 0, 0, 0 );
			VectorSet( out->lightmapVecs[2], 0, 0, 1 ); // normal direction, up

			// not used
			out->patchWidth = 9;
			out->patchHeight = 9;
			out->subdivisions = 16;

			// fill the even indices (texture coords) only, odd ones (lightmap coords) are filled below
			for (x = 0; x < 8; x += 2) {
				texCoords[x] = LittleFloat(in->texCoords[x]);
			}
			// the person who invented the following should be hanged!
			texCoords[1] = ((float)in->lmCoords[0] + 0.5) / LIGHTMAP_SIZE;
			texCoords[3] = ((float)in->lmCoords[1] + 0.5) / LIGHTMAP_SIZE;
			texCoords[5] = ((float)in->lmCoords[0] + 16.5) / LIGHTMAP_SIZE;
			texCoords[7] = ((float)in->lmCoords[1] + 16.5) / LIGHTMAP_SIZE;

			distU = texCoords[4] - texCoords[0];
			distV = texCoords[2] - texCoords[6];

			// setup vertexes
			vert = &bsp->drawVerts[ out->firstVert ];
			for (y = 0; y < 9; y++) {
				for (x = 0; x < 9; x++, vert++) {
					VectorSet(vert->normal, 0, 0, 1); // ZTM: FIXME
					VectorCopy(out->lightmapOrigin, vert->xyz);
					vert->xyz[0] += x * 64;
					vert->xyz[1] += y * 64;
					/*if (x == 0)
						vert->xyz[0] += 16;
					else if (x == 8)
						vert->xyz[0] -= 16;
					if (y == 0)
						vert->xyz[1] += 16;
					else if (y == 8)
						vert->xyz[1] -= 16;*/
					vert->xyz[2] += in->heightmap[y][x] * 2.f;

					f = x / 8.f;
					vert->st[0] = texCoords[0] + f * distU;
					vert->lightmap[0] = texCoords[1] + f * TERRAIN_LM_LENGTH;
					f = y / 8.f;
					vert->st[1] = texCoords[6] + f * distV;
					vert->lightmap[1] = texCoords[3] + f * TERRAIN_LM_LENGTH;

					for ( j = 0; j < 4; j++ ) {
						vert->color[j] = 0;
					}
				}
			}

			ndx = out->firstIndex;
			int verts = 9, quads = 8;
			// setup triangles - actually iterate the quads and make appropriate triangles
			for (y = 0; y < quads; y++) {
				for (x = 0; x < quads; x++, ndx += 6) {
					// The XOR below requires some explanation. Basically, it's done to reflect MoHAA's behaviour.
					// This can be well observed by making a LOD terrain consisting of a single patch in Radiant,
					// but in case you don't have it at hand, here's how it looks (at the highest level of detail):
					//  _______________________________
					// | \ | / | \ | / | \ | / | \ | / |
					// |-------------------------------|
					// | / | \ | / | \ | / | \ | / | \ |
					// |-------------------------------|
					// | \ | / | \ | / | \ | / | \ | / |
					// |-------------------------------|
					// | / | \ | / | \ | / | \ | / | \ |
					// |-------------------------------|
					// | \ | / | \ | / | \ | / | \ | / |
					// |-------------------------------|
					// | / | \ | / | \ | / | \ | / | \ |
					// |-------------------------------|
					// | \ | / | \ | / | \ | / | \ | / |
					// |-------------------------------|
					// | / | \ | / | \ | / | \ | / | \ |
					//  -------------------------------
					// One segment with a slash inside is one quad divided into 2 triangles.
					// Now if you pay attention, you'll notice that considering the diagonals' directions, the
					// quads make a pattern similar to a chessboard. So this is exactly what the XOR is for
					// - to create this pattern quickly. :)
					if ((x % 2) ^ (y % 2)) {
						bsp->drawIndexes[ndx + 0] = y * verts + x;
						bsp->drawIndexes[ndx + 1] = (y + 1) * verts + x;
						bsp->drawIndexes[ndx + 2] = y * verts + x + 1;

						bsp->drawIndexes[ndx + 3] = bsp->drawIndexes[ndx + 2];
						bsp->drawIndexes[ndx + 4] = bsp->drawIndexes[ndx + 1];
						bsp->drawIndexes[ndx + 5] = bsp->drawIndexes[ndx + 1] + 1;
					} else {
						bsp->drawIndexes[ndx + 0] = y * verts + x;
						bsp->drawIndexes[ndx + 1] = (y + 1) * verts + x + 1;
						bsp->drawIndexes[ndx + 2] = y * verts + x + 1;

						bsp->drawIndexes[ndx + 3] = bsp->drawIndexes[ndx + 0];
						bsp->drawIndexes[ndx + 4] = bsp->drawIndexes[ndx + 1] - 1;
						bsp->drawIndexes[ndx + 5] = bsp->drawIndexes[ndx + 1];
					}
				}
			}
		}

		bsp->numSurfaces += numTerSurfaces;
		bsp->numDrawVerts += numTerSurfaces * 9 * 9;
		bsp->numDrawIndexes += numTerSurfaces * 8 * 8 * 6;
	}

	CopyLump( &header, LUMP_LIGHTMAPS, data, (void *) bsp->lightmapData, sizeof ( *bsp->lightmapData ), qfalse ); /* NO SWAP */
#if 0
	CopyLump( &header, LUMP_LIGHTGRID, data, (void *) bsp->lightGridData, sizeof ( *bsp->lightGridData ), qfalse ); /* NO SWAP */
#endif

	if ( bsp->visibilityLength )
	{
		byte *in = GetLump( &header, data, LUMP_VISIBILITY );

		bsp->numClusters = LittleLong( ((int *)in)[0] );
		bsp->clusterBytes = LittleLong( ((int *)in)[1] );

		Com_Memcpy( bsp->visibility, in + VIS_HEADER, bsp->visibilityLength ); /* NO SWAP */
	}

	return bsp;
}


/****************************************************
*/

bspFormat_t mohaaBspFormat = {
	"MOHAA",
	BSP_IDENT,
	BSP_VERSION,
	BSP_LoadMOHAA,
};

