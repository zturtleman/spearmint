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

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1201 // 1200 + 1 buffer for RB_EndSurface overflow check
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)
#define	SHADER_MAX_TRIANGLES	(SHADER_MAX_INDEXES/3)


// the maximum size of game reletive pathnames
#define	MAX_QPATH		64


/*
========================================================================

PCX files are used for 8 bit images

========================================================================
* 

typedef struct {
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;
*/


/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
* 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


*/

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		4
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

  .BSP file format

==============================================================================
*/


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	Q3_MAX_MAP_MODELS		0x400
#define	Q3_MAX_MAP_BRUSHES		0x8000 // ZTM: NOTE: Using value from Quake3/RTCW-MP, it's only 0x4000 in WolfET.
#define	Q3_MAX_MAP_ENTITIES	0x1000 // ZTM: NOTE: Using value from WolfET, it's only 0x800 in Quake3/RTCW-MP.
#define	Q3_MAX_MAP_ENTSTRING	0x40000
#define	Q3_MAX_MAP_SHADERS		0x400

#define	Q3_MAX_MAP_AREAS		0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
#define	Q3_MAX_MAP_FOGS		0x100
#define	Q3_MAX_MAP_PLANES		0x40000 // ZTM: NOTE: Using value from WolfET, it's only 0x20000 in Quake3/RTCW-MP.
#define	Q3_MAX_MAP_NODES		0x20000
#define	Q3_MAX_MAP_BRUSHSIDES	0x100000 // ZTM: NOTE: Using value from WolfET, it's only 0x20000 in Quake3/RTCW-MP.
#define	Q3_MAX_MAP_LEAFS		0x20000
#define	Q3_MAX_MAP_LEAFFACES	0x20000
#define	Q3_MAX_MAP_LEAFBRUSHES 0x40000
#define	Q3_MAX_MAP_PORTALS		0x20000
#define	Q3_MAX_MAP_LIGHTING	0x800000
#define	Q3_MAX_MAP_LIGHTGRID	0x800000
#define	Q3_MAX_MAP_VISIBILITY	0x200000

#define	Q3_MAX_MAP_DRAW_SURFS	0x20000
#define	Q3_MAX_MAP_DRAW_VERTS	0x80000
#define	Q3_MAX_MAP_DRAW_INDEXES	0x80000


// key / value pair sizes in the entities lump
#define	Q3_MAX_KEY				32
#define	Q3_MAX_VALUE			1024

// the editor uses these predefined yaw angles to orient entities up or down
#define	ANGLE_UP			-1
#define	ANGLE_DOWN			-2

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128


#endif
