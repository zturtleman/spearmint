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
//
// cg_surface.c -- procedurally generated surfaces

#include "cg_local.h"

/*
==============
CG_AddCustomSurface
==============
*/
qboolean CG_AddCustomSurface( const refEntity_t *re ) {
	switch ( re->reType ) {
#if 0 // ZTM: sprites don't mirror correctly because they're one sided, also requires using cg.refdef in UI code for player sprites
		case RT_SPRITE:
			CG_SurfaceSprite( re );
			return qtrue;
#endif
		case RT_RAIL_RINGS:
			CG_SurfaceRailRings( re );
			return qtrue;
		case RT_RAIL_CORE:
			CG_SurfaceRailCore( re );
			return qtrue;
		case RT_LIGHTNING:
			CG_SurfaceLightningBolt( re );
			return qtrue;
		case RT_BEAM:
			CG_SurfaceBeam( re );
			return qtrue;
		default:
			return qfalse;
	}
}

/*
==============
CG_SurfaceSprite
==============
*/
void CG_SurfaceSprite( const refEntity_t *e ) {
	polyVert_t verts[4];
	vec3_t left, up;
	refEntity_t re;
	int j;

	re = *e;
	re.reType = RT_POLY;

	// FIXME: cg.refdef.viewaxis doesn't work in UI drawing!
	if ( re.rotation == 0 ) {
		VectorScale( cg.refdef.viewaxis[1], re.radius, left );
		VectorScale( cg.refdef.viewaxis[2], re.radius, up );
	} else {
		float	s, c;
		float	ang;

		ang = M_PI * re.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( cg.refdef.viewaxis[1], c * re.radius, left );
		VectorMA( left, -s * re.radius, cg.refdef.viewaxis[2], left );

		VectorScale( cg.refdef.viewaxis[2], c * re.radius, up );
		VectorMA( up, s * re.radius, cg.refdef.viewaxis[1], up );
	}

	//
	verts[0].xyz[0] = re.origin[0] + left[0] + up[0];
	verts[0].xyz[1] = re.origin[1] + left[1] + up[1];
	verts[0].xyz[2] = re.origin[2] + left[2] + up[2];

	verts[1].xyz[0] = re.origin[0] - left[0] + up[0];
	verts[1].xyz[1] = re.origin[1] - left[1] + up[1];
	verts[1].xyz[2] = re.origin[2] - left[2] + up[2];

	verts[2].xyz[0] = re.origin[0] - left[0] - up[0];
	verts[2].xyz[1] = re.origin[1] - left[1] - up[1];
	verts[2].xyz[2] = re.origin[2] - left[2] - up[2];

	verts[3].xyz[0] = re.origin[0] + left[0] - up[0];
	verts[3].xyz[1] = re.origin[1] + left[1] - up[1];
	verts[3].xyz[2] = re.origin[2] + left[2] - up[2];

	// standard square texture coordinates
	for ( j = 0; j < 4; j++ ) {
		verts[j].st[0] = ( j && j != 3 );
		verts[j].st[1] = ( j < 2 );

		* ( unsigned int * )&verts[j].modulate = * ( unsigned int * )re.shaderRGBA;
	}

	trap_R_AddPolyRefEntityToScene( &re, 4, verts, 1 );
}

/*
==============
CG_DoRailDiscs
==============
*/
static void CG_DoRailDiscs( qhandle_t hShader, byte color[4], int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up )
{
	int i, j;
	vec3_t	pos[4];
	vec3_t	v;
	int		spanWidth = cg_railWidth.integer;
	float c, s;
	float		scale;
	polyVert_t	verts[4];

	if ( numSegs > 1 )
		numSegs--;
	if ( !numSegs )
		return;

	scale = 0.25;

	for ( i = 0; i < 4; i++ )
	{
		c = cos( DEG2RAD( 45 + i * 90 ) );
		s = sin( DEG2RAD( 45 + i * 90 ) );
		v[0] = ( right[0] * c + up[0] * s ) * scale * spanWidth;
		v[1] = ( right[1] * c + up[1] * s ) * scale * spanWidth;
		v[2] = ( right[2] * c + up[2] * s ) * scale * spanWidth;
		VectorAdd( start, v, pos[i] );

		if ( numSegs > 1 )
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[i], dir, pos[i] );
		}
	}

	for ( i = 0; i < numSegs; i++ )
	{
		for ( j = 0; j < 4; j++ )
		{
			VectorCopy( pos[j], verts[j].xyz );
			verts[j].st[0] = ( j && j != 3 );
			verts[j].st[1] = ( j < 2 );
			verts[j].modulate[0] = color[0];
			verts[j].modulate[1] = color[1];
			verts[j].modulate[2] = color[2];
			verts[j].modulate[3] = 0xff;

			VectorAdd( pos[j], dir, pos[j] );
		}

		trap_R_AddPolysToScene( hShader, 4, verts, 1 );
	}
}

/*
==============
CG_SurfaceRailRings
==============
*/
void CG_SurfaceRailRings( const refEntity_t *originEnt ) {
	refEntity_t re;
	int			numSegs;
	int			len;
	vec3_t		vec;
	vec3_t		right, up;
	vec3_t		start, end;

	re = *originEnt;

	VectorCopy( re.oldorigin, start );
	VectorCopy( re.origin, end );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );
	MakeNormalVectors( vec, right, up );
	numSegs = ( len ) / cg_railSegmentLength.value;
	if ( numSegs <= 0 ) {
		numSegs = 1;
	}

	VectorScale( vec, cg_railSegmentLength.value, vec );

	CG_DoRailDiscs( re.customShader, re.shaderRGBA, numSegs, start, vec, right, up );
}

/*
==============
CG_DoRailCore
==============
*/
static void CG_DoRailCore( polyVert_t *verts, const byte color[4], const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth )
{
	float		spanWidth2;
	float		t = len / 256.0f;
	int			numVertexes;

	spanWidth2 = -spanWidth;
	numVertexes = 0;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, verts[numVertexes].xyz );
	verts[numVertexes].st[0] = 0;
	verts[numVertexes].st[1] = 0;
	verts[numVertexes].modulate[0] = color[0] * 0.25;
	verts[numVertexes].modulate[1] = color[1] * 0.25;
	verts[numVertexes].modulate[2] = color[2] * 0.25;
	verts[numVertexes].modulate[3] = 0xff;
	numVertexes++;

	VectorMA( start, spanWidth2, up, verts[numVertexes].xyz );
	verts[numVertexes].st[0] = 0;
	verts[numVertexes].st[1] = 1;
	verts[numVertexes].modulate[0] = color[0];
	verts[numVertexes].modulate[1] = color[1];
	verts[numVertexes].modulate[2] = color[2];
	verts[numVertexes].modulate[3] = 0xff;
	numVertexes++;

	VectorMA( end, spanWidth2, up, verts[numVertexes].xyz );
	verts[numVertexes].st[0] = t;
	verts[numVertexes].st[1] = 1;
	verts[numVertexes].modulate[0] = color[0];
	verts[numVertexes].modulate[1] = color[1];
	verts[numVertexes].modulate[2] = color[2];
	verts[numVertexes].modulate[3] = 0xff;
	numVertexes++;

	VectorMA( end, spanWidth, up, verts[numVertexes].xyz );
	verts[numVertexes].st[0] = t;
	verts[numVertexes].st[1] = 0;
	verts[numVertexes].modulate[0] = color[0];
	verts[numVertexes].modulate[1] = color[1];
	verts[numVertexes].modulate[2] = color[2];
	verts[numVertexes].modulate[3] = 0xff;
	numVertexes++;
}

/*
==============
CG_SurfaceRailCore
==============
*/
void CG_SurfaceRailCore( const refEntity_t *originEnt ) {
	int			len;
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;
	polyVert_t	verts[4];
	refEntity_t re;

	re = *originEnt;
	re.reType = RT_POLY;

	VectorCopy( re.oldorigin, start );
	VectorCopy( re.origin, end );

	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, cg.refdef.vieworg, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, cg.refdef.vieworg, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	CG_DoRailCore( verts, re.shaderRGBA, start, end, right, len, cg_railCoreWidth.integer );

	trap_R_AddPolyRefEntityToScene( &re, 4, verts, 1 );
}

/*
==============
CG_SurfaceLightningBolt
==============
*/
void CG_SurfaceLightningBolt( const refEntity_t *originEnt ) {
	int			len;
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;
	int			i;
#define NUM_BOLT_POLYS 4
	polyVert_t	verts[NUM_BOLT_POLYS*4];
	refEntity_t re;

	re = *originEnt;
	re.reType = RT_POLY;

	VectorCopy( re.oldorigin, end );
	VectorCopy( re.origin, start );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, cg.refdef.vieworg, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, cg.refdef.vieworg, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	for ( i = 0 ; i < NUM_BOLT_POLYS ; i++ ) {
		vec3_t	temp;

		CG_DoRailCore( &verts[i*4], re.shaderRGBA, start, end, right, len, 8 );
		RotatePointAroundVector( temp, vec, right, 180.0f / NUM_BOLT_POLYS );
		VectorCopy( temp, right );
	}

	trap_R_AddPolyRefEntityToScene( &re, 4, verts, NUM_BOLT_POLYS );
}

/*
==============
CG_SurfaceBeam
==============
*/
void CG_SurfaceBeam( const refEntity_t *originEnt )
{
#define NUM_BEAM_SEGS 6
	refEntity_t re;
	int	i, j;
	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;
	polyVert_t	verts[NUM_BEAM_SEGS*4];
	int			numVerts;

	re = *originEnt;

	oldorigin[0] = re.oldorigin[0];
	oldorigin[1] = re.oldorigin[1];
	oldorigin[2] = re.oldorigin[2];

	origin[0] = re.origin[0];
	origin[1] = re.origin[1];
	origin[2] = re.origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS ; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] ); // Note: Q3A incorrectly had this disabled, causing all beams to start at map origin
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	re.customShader = cgs.media.whiteShader;

	numVerts = 0;
	for ( i = 0; i < NUM_BEAM_SEGS; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			if ( j && j != 3 )
				VectorCopy( end_points[ (i+(j>1)) % NUM_BEAM_SEGS], verts[numVerts].xyz );
			else
				VectorCopy( start_points[ (i+(j>1)) % NUM_BEAM_SEGS], verts[numVerts].xyz );
			verts[numVerts].st[0] = ( j < 2 );
			verts[numVerts].st[1] = ( j && j != 3 );
			verts[numVerts].modulate[0] = 0xff;
			verts[numVerts].modulate[1] = 0;
			verts[numVerts].modulate[2] = 0;
			verts[numVerts].modulate[3] = 0xff;
			numVerts++;
		}
	}

	// for acting like Q3A create a custom shader with
	// cull none and blendfunc GL_ONE GL_ONE
	trap_R_AddPolysToScene( re.customShader, 4, verts, NUM_BEAM_SEGS );
}

