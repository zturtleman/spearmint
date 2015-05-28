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
#ifndef __QFILES_H__
#define __QFILES_H__

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

//Ignore __attribute__ on non-gcc platforms
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1201 // 1200 + 1 buffer for RB_EndSurface overflow check
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)
#define	SHADER_MAX_TRIANGLES	(SHADER_MAX_INDEXES/3)


// the maximum size of game relative pathnames
#define	MAX_QPATH		64

/*
========================================================================

QVM files

========================================================================
*/

// QVM magics not supported by Spearmint
#define	VM_MAGIC			0x12721444
#define	VM_MAGIC_VER2		0x12721445

// Spearmint QVM magic, it's the same as VM_MAGIC_VER2 but with a different magic number.
#define	VM_MAGIC_VER2_NEO	0x12721443

typedef struct {
	int		vmMagic;

	int		instructionCount;

	int		codeOffset;
	int		codeLength;

	int		dataOffset;
	int		dataLength;
	int		litLength;			// ( dataLength - litLength ) should be byteswapped on load
	int		bssLength;			// zero filled memory appended to datalength

	//!!! below here is VM_MAGIC_VER2 !!!
	int		jtrgLength;			// number of jump table targets
} vmHeader_t;

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		3
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

typedef struct md3Frame_s {
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s {
	char		name[MAX_QPATH];	// tag name
	vec3_t		origin;
	vec3_t		axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct {
	int		ident;				// 

	char	name[MAX_QPATH];	// polyset name

	int		flags;
	int		numFrames;			// all surfaces in a model should have the same

	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;

	int		numTriangles;
	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct {
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct {
	int			indexes[3];
} md3Triangle_t;

typedef struct {
	float		st[2];
} md3St_t;

typedef struct {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;			
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;

/*
==============================================================================

MDC file format

==============================================================================
*/

#define MDC_IDENT           ( ( 'C' << 24 ) + ( 'P' << 16 ) + ( 'D' << 8 ) + 'I' )
#define MDC_VERSION         2

// version history:
// 1 - original
// 2 - changed tag structure so it only lists the names once

typedef struct {
	unsigned int ofsVec;                    // offset direction from the last base frame
//	unsigned short	ofsVec;
} mdcXyzCompressed_t;

typedef struct {
	char name[MAX_QPATH];           // tag name
} mdcTagName_t;

#define MDC_TAG_ANGLE_SCALE ( 360.0 / 32700.0 )

typedef struct {
	short xyz[3];
	short angles[3];
} mdcTag_t;

/*
** mdcSurface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numBaseFrames
** XyzCompressed	sizeof( mdcXyzCompressed ) * numVerts * numCompFrames
** frameBaseFrames	sizeof( short ) * numFrames
** frameCompFrames	sizeof( short ) * numFrames (-1 if frame is a baseFrame)
*/
typedef struct {
	int ident;                  //

	char name[MAX_QPATH];       // polyset name

	int flags;
	int numCompFrames;          // all surfaces in a model should have the same
	int numBaseFrames;          // ditto

	int numShaders;             // all surfaces in a model should have the same
	int numVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsShaders;             // offset from start of md3Surface_t
	int ofsSt;                  // texture coords are common for all frames
	int ofsXyzNormals;          // numVerts * numBaseFrames
	int ofsXyzCompressed;       // numVerts * numCompFrames

	int ofsFrameBaseFrames;     // numFrames
	int ofsFrameCompFrames;     // numFrames

	int ofsEnd;                 // next surface follows
} mdcSurface_t;

typedef struct {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	int flags;

	int numFrames;
	int numTags;
	int numSurfaces;

	int numSkins;

	int ofsFrames;                  // offset for first frame, stores the bounds and localOrigin
	int ofsTagNames;                // numTags
	int ofsTags;                    // numFrames * numTags
	int ofsSurfaces;                // first surface, others follow

	int ofsEnd;                     // end of file
} mdcHeader_t;

/*
==============================================================================

MDR file format

==============================================================================
*/

/*
 * Here are the definitions for Ravensoft's model format of md4. Raven stores their
 * playermodels in .mdr files, in some games, which are pretty much like the md4
 * format implemented by ID soft. It seems like ID's original md4 stuff is not used at all.
 * MDR is being used in EliteForce, JediKnight2 and Soldiers of Fortune2 (I think).
 * So this comes in handy for anyone who wants to make it possible to load player
 * models from these games.
 * This format has bone tags, which is similar to the thing you have in md3 I suppose.
 * Raven has released their version of md3view under GPL enabling me to add support
 * to this codebase. Thanks to Steven Howes aka Skinner for helping with example
 * source code.
 *
 * - Thilo Schulz (arny@ats.s.bawue.de)
 */

#define MDR_IDENT	(('5'<<24)+('M'<<16)+('D'<<8)+'R')
#define MDR_VERSION	2
#define	MDR_MAX_BONES	128

typedef struct {
	int			boneIndex;	// these are indexes into the boneReferences,
	float		   boneWeight;		// not the global per-frame bone list
	vec3_t		offset;
} mdrWeight_t;

typedef struct {
	vec3_t		normal;
	vec2_t		texCoords;
	int			numWeights;
	mdrWeight_t	weights[1];		// variable sized
} mdrVertex_t;

typedef struct {
	int			indexes[3];
} mdrTriangle_t;

typedef struct {
	int			ident;

	char		name[MAX_QPATH];	// polyset name
	char		shader[MAX_QPATH];
	int			shaderIndex;	// for in-game use

	int			ofsHeader;	// this will be a negative number

	int			numVerts;
	int			ofsVerts;

	int			numTriangles;
	int			ofsTriangles;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int			numBoneReferences;
	int			ofsBoneReferences;

	int			ofsEnd;		// next surface follows
} mdrSurface_t;

typedef struct {
	float		matrix[3][4];
} mdrBone_t;

typedef struct {
	vec3_t		bounds[2];		// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;			// dist from localOrigin to corner
	char		name[16];
	mdrBone_t	bones[1];		// [numBones]
} mdrFrame_t;

typedef struct {
        unsigned char Comp[24]; // MC_COMP_BYTES is in MatComp.h, but don't want to couple
} mdrCompBone_t;

typedef struct {
        vec3_t          bounds[2];		// bounds of all surfaces of all LOD's for this frame
        vec3_t          localOrigin;		// midpoint of bounds, used for sphere cull
        float           radius;			// dist from localOrigin to corner
        mdrCompBone_t   bones[1];		// [numBones]
} mdrCompFrame_t;

typedef struct {
	int			numSurfaces;
	int			ofsSurfaces;		// first surface, others follow
	int			ofsEnd;				// next lod follows
} mdrLOD_t;

typedef struct {
        int                     boneIndex;
        char            name[32];
} mdrTag_t;

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	// frames and bones are shared by all levels of detail
	int			numFrames;
	int			numBones;
	int			ofsFrames;			// mdrFrame_t[numFrames]

	// each level of detail has completely separate sets of surfaces
	int			numLODs;
	int			ofsLODs;

        int                     numTags;
        int                     ofsTags;

	int			ofsEnd;				// end of file
} mdrHeader_t;


/*
==============================================================================

MDS file format (Return to Castle Wolfenstein Skeletal Format)

==============================================================================
*/

#define MDS_IDENT           ( ( 'W' << 24 ) + ( 'S' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDS_VERSION         4
#define MDS_MAX_VERTS       6000
#define MDS_MAX_TRIANGLES   8192
#define MDS_MAX_BONES       128
#define MDS_MAX_SURFACES    32
#define MDS_MAX_TAGS        128

#define MDS_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct {
	int boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
} mdsWeight_t;

typedef struct {
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	int fixedParent;            // stay equi-distant from this parent
	float fixedDist;
	mdsWeight_t weights[1];     // variable sized
} mdsVertex_t;

typedef struct {
	int indexes[3];
} mdsTriangle_t;

typedef struct {
	int ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                // for in-game use

	int minLod;

	int ofsHeader;                  // this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsCollapseMap;           // numVerts * int

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next surface follows
} mdsSurface_t;

typedef struct {
	//float		angles[3];
	//float		ofsAngles[2];
	short angles[4];            // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];         // PITCH/YAW, head in this direction from parent to go to the offset position
} mdsBoneFrameCompressed_t;

// NOTE: this only used at run-time
typedef struct {
	float matrix[3][3];             // 3x3 rotation
	vec3_t translation;             // translation vector
} mdsBoneFrame_t;

typedef struct {
	vec3_t bounds[2];               // bounds of all surfaces of all LOD's for this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
	mdsBoneFrameCompressed_t bones[1];              // [numBones]
} mdsFrame_t;

typedef struct {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
} mdsLOD_t;

typedef struct {
	char name[MAX_QPATH];           // name of tag
	float torsoWeight;
	int boneIndex;                  // our index in the bones
} mdsTag_t;

#define MDS_BONEFLAG_TAG        1       // this bone is actually a tag

typedef struct {
	char name[MAX_QPATH];           // name of bone
	int parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int flags;
} mdsBoneInfo_t;

typedef struct {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
	int numFrames;
	int numBones;
	int ofsFrames;                  // (mdsFrame_t + mdsBoneFrameCompressed_t[numBones-1]) * numframes
	int ofsBones;                   // mdsBoneInfo_t[numBones]
	int torsoParent;                // index of bone that is the parent of the torso

	int numSurfaces;
	int ofsSurfaces;

	// tag data
	int numTags;
	int ofsTags;                    // mdsTag_t[numTags]

	int ofsEnd;                     // end of file
} mdsHeader_t;


/*
==============================================================================

MDM file format (Wolfenstein Enemy Territory Skeletal Mesh)

version history:
	2 - initial version
	3 - removed all frame data, this format is pure mesh and bone references now

==============================================================================
*/

#define MDM_IDENT           ( ( 'W' << 24 ) + ( 'M' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDM_VERSION         3
#define MDM_MAX_VERTS       6000
#define MDM_MAX_TRIANGLES   8192
#define MDM_MAX_SURFACES    32
#define MDM_MAX_TAGS        128

#define MDM_TRANSLATION_SCALE   ( 1.0 / 64 )

typedef struct {
	int boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
} mdmWeight_t;

typedef struct {
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	mdmWeight_t weights[1];     // variable sized
} mdmVertex_t;

typedef struct {
	int indexes[3];
} mdmTriangle_t;

typedef struct {
	int ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                // for in-game use

	int minLod;

	int ofsHeader;                  // this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsCollapseMap;           // numVerts * int

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next surface follows
} mdmSurface_t;

/*typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;				// dist from localOrigin to corner
	vec3_t		parentOffset;		// one bone is an ascendant of all other bones, it starts the hierachy at this position
} mdmFrame_t;*/

typedef struct {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
} mdmLOD_t;

/*typedef struct {
	char		name[MAX_QPATH];	// name of tag
	float		torsoWeight;
	int			boneIndex;			// our index in the bones

	int			numBoneReferences;
	int			ofsBoneReferences;

	int			ofsEnd;				// next tag follows
} mdmTag_t;*/

// Tags always only have one parent bone
typedef struct {
	char name[MAX_QPATH];           // name of tag
	vec3_t axis[3];

	int boneIndex;
	vec3_t offset;

	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next tag follows
} mdmTag_t;

typedef struct {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name
/*	char		bonesfile[MAX_QPATH];	// bone file

#ifdef UTILS
	int			skel;
#else
	// dummy in file, set on load to link to MDX
	qhandle_t	skel;
#endif // UTILS
*/
	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
/*	int			numFrames;
	int			ofsFrames;			// mdmFrame_t[numFrames]
*/
	int numSurfaces;
	int ofsSurfaces;

	// tag data
	int numTags;
	int ofsTags;

	int ofsEnd;                     // end of file
} mdmHeader_t;

/*
==============================================================================

MDX file format (Wolfenstein Enemy Territory Skeletal Data)

version history:
	1 - initial version
	2 - moved parentOffset from the mesh to the skeletal data file

==============================================================================
*/

#define MDX_IDENT           ( ( 'W' << 24 ) + ( 'X' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDX_VERSION         2
#define MDX_MAX_BONES       128

typedef struct {
	//float		angles[3];
	//float		ofsAngles[2];
	short angles[4];                // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];             // PITCH/YAW, head in this direction from parent to go to the offset position
} mdxBoneFrameCompressed_t;

// NOTE: this only used at run-time
// FIXME: do we really need this?
typedef struct {
	float matrix[3][3];             // 3x3 rotation
	vec3_t translation;             // translation vector
} mdxBoneFrame_t;

typedef struct {
	vec3_t bounds[2];               // bounds of this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
	mdxBoneFrameCompressed_t bones[1]; // [numBones]
} mdxFrame_t;

#define MDX_BONEFLAG_TAG        1       // this bone is actually a tag

typedef struct {
	char name[MAX_QPATH];           // name of bone
	int parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int flags;
} mdxBoneInfo_t;

typedef struct {
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	// bones are shared by all levels of detail
	int numFrames;
	int numBones;
	int ofsFrames;                  // (mdxFrame_t + mdxBoneFrameCompressed_t[numBones-1]) * numframes
	int ofsBones;                   // mdxBoneInfo_t[numBones]
	int torsoParent;                // index of bone that is the parent of the torso

	int ofsEnd;                     // end of file
} mdxHeader_t;


/*
==============================================================================

.TAN model format (Tiki ANimated model?)

Ritual's modified version of MD3

ZTM: TODO: use header totaltime and totaldelta and frame delta and frametime for something?

==============================================================================
*/

#define TIKI_ANIM_IDENT		((' '<<24)+('N'<<16)+('A'<<8)+'T')
#define TIKI_ANIM_VERSION	2

// limits
#define TIKI_MAX_LODS		4
#define TIKI_MAX_TRIANGLES	4096	// per surface
#define TIKI_MAX_VERTS		1200	// per surface
#define TIKI_MAX_SHADERS	256		// per surface
#define TIKI_MAX_FRAMES		2048	// per model
#define TIKI_MAX_SURFACES	32		// per model
#define TIKI_MAX_TAGS		16		// per frame

typedef struct {
	unsigned short	xyz[3];
	short			normal;
} tanXyzNormal_t;

typedef struct tikiFrame_s {
	vec3_t	bounds[2];
	vec3_t	scale; // multiply by this
	vec3_t	offset; // and add by this
	vec3_t	delta;
	float	radius;
	float	frametime;
} tanFrame_t;

/*
** tanSurface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct {
	int		ident;				// 

	char	name[MAX_QPATH];	// polyset name

	int		numFrames;			// all surfaces in a model should have the same
	int		numVerts;
	int		minLod;

	int		numTriangles;
	int		ofsTriangles;

	int		ofsCollapseMap;		// numVerts * int

	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} tanSurface_t;

typedef struct {
	vec3_t	origin;
	vec3_t	axis[3];
} tanTagData_t;

typedef struct {
	char	name[MAX_QPATH];	// tag name
} tanTag_t;

typedef struct {
	int		ident;
	int		version;

	char	name[MAX_QPATH];	// model name

	int		numFrames;
	int		numTags;
	int		numSurfaces;
	float	totaltime;
	vec3_t	totaldelta;

	int		ofsFrames;			// offset for first frame
	int		ofsSurfaces;		// first surface, others follow
	int		ofsTags[ TIKI_MAX_TAGS ]; // tanTag_t + numFrames * tanTagData_t

	int		ofsEnd;				// end of file
} tanHeader_t;

/*
==============================================================================

  .BSP file format

==============================================================================
*/

// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	MAX_MAP_MODELS		0x400
#define	MAX_MAP_BRUSHES		0x8000 // ZTM: NOTE: Using value from Quake3/RTCW-MP, it's only 0x4000 in WolfET.
#define	MAX_MAP_ENTITIES	0x1000 // ZTM: NOTE: Using value from WolfET, it's only 0x800 in Quake3/RTCW-MP.
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_SHADERS		0x400

#define	MAX_MAP_AREAS		0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
#define	MAX_MAP_FOGS		0x100
#define	MAX_MAP_PLANES		0x40000 // ZTM: NOTE: Using value from WolfET, it's only 0x20000 in Quake3/RTCW-MP.
#define	MAX_MAP_NODES		0x20000
#define	MAX_MAP_BRUSHSIDES	0x100000 // ZTM: NOTE: Using value from WolfET, it's only 0x20000 in Quake3/RTCW-MP.
#define	MAX_MAP_LEAFS		0x20000
#define	MAX_MAP_LEAFFACES	0x20000
#define	MAX_MAP_LEAFBRUSHES 0x40000
#define	MAX_MAP_PORTALS		0x20000
#define	MAX_MAP_LIGHTING	0x800000
#define	MAX_MAP_LIGHTGRID	0x800000
#define	MAX_MAP_VISIBILITY	0x200000

#define	MAX_MAP_DRAW_SURFS	0x20000
#define	MAX_MAP_DRAW_VERTS	0x80000
#define	MAX_MAP_DRAW_INDEXES	0x80000


// key / value pair sizes in the entities lump
#define	MAX_KEY				32
#define	MAX_VALUE			1024

// the editor uses these predefined yaw angles to orient entities up or down
#define	ANGLE_UP			-1
#define	ANGLE_DOWN			-2

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128

#define MAX_WORLD_COORD		( 128*1024 )
#define MIN_WORLD_COORD		( -128*1024 )
#define WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

#endif
