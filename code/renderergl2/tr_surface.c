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
// tr_surf.c
#include "tr_local.h"
#if idppc_altivec && !defined(__APPLE__)
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/


//============================================================================


/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow( int verts, int indexes ) {
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES
		&& tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );
}

void RB_CheckVao(vao_t *vao)
{
	if (vao != glState.currentVao)
	{
		RB_EndSurface();
		RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);

		R_BindVao(vao);
	}

	if (vao != tess.vao)
		tess.useInternalVao = qfalse;
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 ) {
	vec3_t		normal;
	int16_t     iNormal[4];
	uint16_t    iColor[4];
	int			ndx;

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx+1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx+1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx+1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx+2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx+2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx+2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx+3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx+3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx+3][2] = origin[2] + left[2] - up[2];


	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.or.axis[0], normal );

	R_VaoPackNormal(iNormal, normal);

	VectorCopy4(iNormal, tess.normal[ndx]);
	VectorCopy4(iNormal, tess.normal[ndx + 1]);
	VectorCopy4(iNormal, tess.normal[ndx + 2]);
	VectorCopy4(iNormal, tess.normal[ndx + 3]);

	// standard square texture coordinates
	VectorSet2(tess.texCoords[ndx], s1, t1);
	VectorSet2(tess.lightCoords[ndx], s1, t1);

	VectorSet2(tess.texCoords[ndx+1], s2, t1);
	VectorSet2(tess.lightCoords[ndx+1], s2, t1);

	VectorSet2(tess.texCoords[ndx+2], s2, t2);
	VectorSet2(tess.lightCoords[ndx+2], s2, t2);

	VectorSet2(tess.texCoords[ndx+3], s1, t2);
	VectorSet2(tess.lightCoords[ndx+3], s1, t2);

	// constant color all the way around
	// should this be identity and let the shader specify from entity?

	R_VaoPackColor(iColor, color);

	VectorCopy4(iColor, tess.color[ndx]);
	VectorCopy4(iColor, tess.color[ndx + 1]);
	VectorCopy4(iColor, tess.color[ndx + 2]);
	VectorCopy4(iColor, tess.color[ndx + 3]);

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}


/*
==============
RB_InstantQuad

based on Tess_InstantQuad from xreal
==============
*/
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4])
{
	GLimp_LogComment("--- RB_InstantQuad2 ---\n");

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_UpdateTessVao(ATTR_POSITION | ATTR_TEXCOORD);

	R_DrawElements(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}


void RB_InstantQuad(vec4_t quadVerts[4])
{
	vec2_t texCoords[4];

	VectorSet2(texCoords[0], 0.0f, 0.0f);
	VectorSet2(texCoords[1], 1.0f, 0.0f);
	VectorSet2(texCoords[2], 1.0f, 1.0f);
	VectorSet2(texCoords[3], 0.0f, 1.0f);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMat4(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}


/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( void ) {
	vec3_t		axis[3];
	vec3_t		left, up;
	float		radius;
	float			colors[4];
	trRefEntity_t	*ent = backEnd.currentEntity;

	switch ( tess.shader->spriteGen ) {
		case SG_PARALLEL:
			// face screen
			AxisCopy( backEnd.viewParms.or.axis, axis );
			break;

		case SG_PARALLEL_UPRIGHT:
			// rotate around up
			VectorSet( axis[2], 0, 0, 1 );

			VectorCopy( backEnd.viewParms.or.axis[0], axis[0] );
			CrossProduct( axis[2], axis[0], axis[1] );
			VectorNormalize( axis[1] );
			break;

		case SG_PARALLEL_ORIENTED:
			// rotating around up axis
			VectorCopy( backEnd.currentEntity->e.axis[2], axis[2] );

			if ( !VectorLength( axis[2] ) ) {
				ri.Printf( PRINT_WARNING, "WARNING: Sprite using shader '%s' missing axis orientation for spriteGen\n", tess.shader->name );
				VectorSet( axis[2], 0, 0, 1 );
			}

			VectorCopy( backEnd.viewParms.or.axis[0], axis[0] );
			CrossProduct( axis[2], axis[0], axis[1] );
			VectorNormalize( axis[1] );
			break;

		case SG_ORIENTED:
			// face direction of normal
			AxisCopy( backEnd.currentEntity->e.axis, axis );
			if ( !VectorLength( axis[1] ) || !VectorLength( axis[2] ) ) {
				ri.Printf( PRINT_WARNING, "WARNING: Sprite using shader '%s' missing axis orientation for spriteGen\n", tess.shader->name );
				VectorSet( axis[1], 0, 1, 0 );
				VectorSet( axis[2], 0, 0, 1 );
			}
			break;

		default:
			ri.Error( ERR_DROP, "Unhandled spriteGen %d", tess.shader->spriteGen );
			break;
	}

	// calculate the xyz locations for the four corners
	radius = ent->e.radius * tess.shader->spriteScale;
	if ( ent->e.rotation == 0 ) {
		VectorScale( axis[1], radius, left );
		VectorScale( axis[2], radius, up );
	} else {
		float	s, c;
		float	ang;
		
		ang = M_PI * ent->e.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( axis[1], c * radius, left );
		VectorMA( left, -s * radius, axis[2], left );

		VectorScale( axis[2], c * radius, up );
		VectorMA( up, s * radius, axis[1], up );
	}
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	VectorScale4(ent->e.shaderRGBA, 1.0f / 255.0f, colors);

	RB_AddQuadStamp( ent->e.origin, left, up, colors );
}


/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0] = p->verts[i].st[0];
		tess.texCoords[numv][1] = p->verts[i].st[1];
		tess.color[numv][0] = (int)p->verts[i].modulate[0] * 257;
		tess.color[numv][1] = (int)p->verts[i].modulate[1] * 257;
		tess.color[numv][2] = (int)p->verts[i].modulate[2] * 257;
		tess.color[numv][3] = (int)p->verts[i].modulate[3] * 257;

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ ) {
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

static void RB_SurfaceVertsAndIndexes( int numVerts, srfVert_t *verts, int numIndexes, glIndex_t *indexes, int dlightBits, int pshadowBits)
{
	int             i;
	glIndex_t      *inIndex;
	srfVert_t      *dv;
	float          *xyz, *texCoords, *lightCoords;
	int16_t        *lightdir;
	int16_t        *normal;
	int16_t        *tangent;
	glIndex_t      *outIndex;
	uint16_t       *color;

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( numVerts, numIndexes );

	inIndex = indexes;
	outIndex = &tess.indexes[ tess.numIndexes ];
	for ( i = 0 ; i < numIndexes ; i++ ) {
		*outIndex++ = tess.numVertexes + *inIndex++;
	}
	tess.numIndexes += numIndexes;

	if ( tess.shader->vertexAttribs & ATTR_POSITION )
	{
		dv = verts;
		xyz = tess.xyz[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, xyz+=4 )
			VectorCopy(dv->xyz, xyz);
	}

	if ( tess.shader->vertexAttribs & ATTR_NORMAL )
	{
		dv = verts;
		normal = tess.normal[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, normal+=4 )
			VectorCopy4(dv->normal, normal);
	}

	if ( tess.shader->vertexAttribs & ATTR_TANGENT )
	{
		dv = verts;
		tangent = tess.tangent[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, tangent+=4 )
			VectorCopy4(dv->tangent, tangent);
	}

	if ( tess.shader->vertexAttribs & ATTR_TEXCOORD )
	{
		dv = verts;
		texCoords = tess.texCoords[tess.numVertexes];
		for ( i = 0 ; i < numVerts ; i++, dv++, texCoords+=2 )
			VectorCopy2(dv->st, texCoords);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTCOORD )
	{
		dv = verts;
		lightCoords = tess.lightCoords[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightCoords+=2 )
			VectorCopy2(dv->lightmap, lightCoords);
	}

	if ( tess.shader->vertexAttribs & ATTR_COLOR )
	{
		dv = verts;
		color = tess.color[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, color+=4 )
			VectorCopy4(dv->color, color);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
	{
		dv = verts;
		lightdir = tess.lightdir[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightdir+=4 )
			VectorCopy4(dv->lightdir, lightdir);
	}

#if 0  // nothing even uses vertex dlightbits
	for ( i = 0 ; i < numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i ] = dlightBits;
	}
#endif

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	tess.numVertexes += numVerts;
}

static qboolean RB_SurfaceVaoCached(int numVerts, srfVert_t *verts, int numIndexes, glIndex_t *indexes, int dlightBits, int pshadowBits)
{
	qboolean recycleVertexBuffer = qfalse;
	qboolean recycleIndexBuffer = qfalse;
	qboolean endSurface = qfalse;

	if (!(!ShaderRequiresCPUDeforms(tess.shader) && !tess.shader->isSky && !tess.shader->isPortal))
		return qfalse;

	if (!numIndexes || !numVerts)
		return qfalse;

	VaoCache_BindVao();

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	VaoCache_CheckAdd(&endSurface, &recycleVertexBuffer, &recycleIndexBuffer, numVerts, numIndexes);

	if (endSurface)
	{
		RB_EndSurface();
		RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);
	}

	if (recycleVertexBuffer)
		VaoCache_RecycleVertexBuffer();

	if (recycleIndexBuffer)
		VaoCache_RecycleIndexBuffer();

	if (!tess.numVertexes)
		VaoCache_InitQueue();

	VaoCache_AddSurface(verts, numVerts, indexes, numIndexes);

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVerts;
	tess.useInternalVao = qfalse;
	tess.useCacheVao = qtrue;

	return qtrue;
}


/*
=============
RB_SurfaceTriangles
=============
*/
static void RB_SurfaceTriangles( srfBspSurface_t *srf ) {
	if (RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
		srf->indexes, srf->dlightBits, srf->pshadowBits))
	{
		return;
	}

	RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
			srf->indexes, srf->dlightBits, srf->pshadowBits);
}


/*
=============
RB_SurfaceFoliage
=============
*/
static void RB_SurfaceFoliage( srfFoliage_t *srf ) {
	int o, i;
	vec4_t distanceCull, distanceVector;
	float alpha, z, dist, fovScale;
	float *xyz;
	uint16_t srcColor[4];
	uint16_t *color;
	vec3_t local;
	foliageInstance_t   *instance;
	int numPlanes;

	// set fov scale
	fovScale = backEnd.viewParms.fovX * ( 1.0 / 90.0 );

	// calculate distance vector
	VectorSubtract( backEnd.or.origin, backEnd.viewParms.or.origin, local );
	distanceVector[ 0 ] = -backEnd.or.modelMatrix[ 2 ];
	distanceVector[ 1 ] = -backEnd.or.modelMatrix[ 6 ];
	distanceVector[ 2 ] = -backEnd.or.modelMatrix[ 10 ];
	distanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.or.axis[ 0 ] );

	// attempt distance cull
	Vector4Copy( tess.shader->distanceCull, distanceCull );
	if ( distanceCull[ 1 ] > 0 ) {
		//VectorSubtract( srf->localOrigin, viewOrigin, delta );
		//alpha = (distanceCull[ 1 ] - VectorLength( delta ) + srf->radius) * distanceCull[ 3 ];
		z = fovScale * ( DotProduct( srf->origin, distanceVector ) + distanceVector[ 3 ] - srf->radius );
		alpha = ( distanceCull[ 1 ] - z ) * distanceCull[ 3 ];
		if ( alpha < distanceCull[ 2 ] ) {
			return;
		}
	}

	numPlanes = (backEnd.viewParms.flags & VPF_FARPLANEFRUSTUM) ? 5 : 4;

	// iterate through origin list
	instance = srf->instances;
	for ( o = 0; o < srf->numInstances; o++, instance++ )
	{
		// fade alpha based on distance between inner and outer radii
		if ( distanceCull[ 1 ] > 0.0f ) {
			// calculate z distance
			z = fovScale * ( DotProduct( instance->origin, distanceVector ) + distanceVector[ 3 ] );
			if ( z < -64.0f ) {  // epsilon so close-by foliage doesn't pop in and out
				continue;
			}

			// check against frustum planes
			for ( i = 0; i < numPlanes; i++ )
			{
				dist = DotProduct( instance->origin, backEnd.viewParms.frustum[ i ].normal ) - backEnd.viewParms.frustum[ i ].dist;
				if ( dist < -64.0 ) {
					break;
				}
			}
			if ( i != numPlanes ) {
				continue;
			}

			// radix
			if ( o & 1 ) {
				z *= 1.25;
				if ( o & 2 ) {
					z *= 1.25;
				}
			}

			// calculate alpha
			alpha = ( distanceCull[ 1 ] - z ) * distanceCull[ 3 ];
			if ( alpha < distanceCull[ 2 ] ) {
				continue;
			}

			// set color
			alpha = Com_Clamp( 0, 255, alpha * 255 );
		} else {
			alpha = 255;
		}

		Vector4Copy( instance->color, srcColor );
		srcColor[3] = alpha * 257;

		// ri.Printf( PRINT_ALL, "Color: %f %f %f %f\n", instance->color[ 0 ], instance->color[ 1 ], instance->color[ 2 ], alpha );

		RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
								  srf->indexes, srf->dlightBits, srf->pshadowBits);

		// offset xyz
		xyz = tess.xyz[ tess.numVertexes -  srf->numVerts ];
		for ( i = 0 ; i < srf->numVerts ; i++, xyz+=4 ) {
			VectorAdd( xyz, instance->origin, xyz );
		}

		// copy color
		color = tess.color[ tess.numVertexes -  srf->numVerts ];
		for ( i = 0 ; i < srf->numVerts ; i++, color+=4 ) {
			Vector4Copy( srcColor, color );
		}
	}

	// RB_DrawBounds( srf->bounds[ 0 ], srf->bounds[ 1 ] );
}

//================================================================================


static void LerpMeshVertexes(mdvSurface_t *surf, float backlerp)
{
	float *outXyz;
	int16_t *outNormal, *outTangent;
	mdvVertex_t *newVerts;
	int		vertNum;

	newVerts = surf->verts + backEnd.currentEntity->e.frame * surf->numVerts;

	outXyz =     tess.xyz[tess.numVertexes];
	outNormal =  tess.normal[tess.numVertexes];
	outTangent = tess.tangent[tess.numVertexes];

	if (backlerp == 0)
	{
		//
		// just copy the vertexes
		//

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			VectorCopy(newVerts->xyz,    outXyz);
			VectorCopy4(newVerts->normal, outNormal);
			VectorCopy4(newVerts->tangent, outTangent);

			newVerts++;
			outXyz += 4;
			outNormal += 4;
			outTangent += 4;
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//

		mdvVertex_t *oldVerts;

		oldVerts = surf->verts + backEnd.currentEntity->e.oldframe * surf->numVerts;

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			VectorLerp(newVerts->xyz,    oldVerts->xyz,    backlerp, outXyz);

			outNormal[0] = (int16_t)(newVerts->normal[0] * (1.0f - backlerp) + oldVerts->normal[0] * backlerp);
			outNormal[1] = (int16_t)(newVerts->normal[1] * (1.0f - backlerp) + oldVerts->normal[1] * backlerp);
			outNormal[2] = (int16_t)(newVerts->normal[2] * (1.0f - backlerp) + oldVerts->normal[2] * backlerp);
			outNormal[3] = 0;

			outTangent[0] = (int16_t)(newVerts->tangent[0] * (1.0f - backlerp) + oldVerts->tangent[0] * backlerp);
			outTangent[1] = (int16_t)(newVerts->tangent[1] * (1.0f - backlerp) + oldVerts->tangent[1] * backlerp);
			outTangent[2] = (int16_t)(newVerts->tangent[2] * (1.0f - backlerp) + oldVerts->tangent[2] * backlerp);
			outTangent[3] = newVerts->tangent[3];

			newVerts++;
			oldVerts++;
			outXyz += 4;
			outNormal += 4;
			outTangent += 4;
		}
	}

}


/*
=============
RB_SurfaceMesh
=============
*/
static void RB_SurfaceMesh(mdvSurface_t *surface) {
	int				j;
	float			backlerp;
	mdvSt_t			*texCoords;
	int				Bob, Doug;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CheckVao(tess.vao);

	RB_CHECKOVERFLOW( surface->numVerts, surface->numIndexes );

	LerpMeshVertexes (surface, backlerp);

	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for (j = 0 ; j < surface->numIndexes ; j++) {
		tess.indexes[Bob + j] = Doug + surface->indexes[j];
	}
	tess.numIndexes += surface->numIndexes;

	texCoords = surface->st;

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0] = texCoords[j].st[0];
		tess.texCoords[Doug + j][1] = texCoords[j].st[1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;

}


/*
==============
RB_SurfaceFace
==============
*/
static void RB_SurfaceFace( srfBspSurface_t *srf ) {
	if (RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
		srf->indexes, srf->dlightBits, srf->pshadowBits))
	{
		return;
	}

	RB_SurfaceVertsAndIndexes(srf->numVerts, srf->verts, srf->numIndexes,
			srf->indexes, srf->dlightBits, srf->pshadowBits);
}


static float	LodErrorForVolume( vec3_t local, float radius ) {
	vec3_t		world;
	float		d;

	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	world[0] = local[0] * backEnd.or.axis[0][0] + local[1] * backEnd.or.axis[1][0] + 
		local[2] * backEnd.or.axis[2][0] + backEnd.or.origin[0];
	world[1] = local[0] * backEnd.or.axis[0][1] + local[1] * backEnd.or.axis[1][1] + 
		local[2] * backEnd.or.axis[2][1] + backEnd.or.origin[1];
	world[2] = local[0] * backEnd.or.axis[0][2] + local[1] * backEnd.or.axis[1][2] + 
		local[2] * backEnd.or.axis[2][2] + backEnd.or.origin[2];

	VectorSubtract( world, backEnd.viewParms.or.origin, world );
	d = DotProduct( world, backEnd.viewParms.or.axis[0] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
static void RB_SurfaceGrid( srfBspSurface_t *srf ) {
	int		i, j;
	float	*xyz;
	float	*texCoords, *lightCoords;
	int16_t *normal;
	int16_t *tangent;
	uint16_t *color;
	int16_t *lightdir;
	srfVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int     pshadowBits;
	//int		*vDlightBits;

	if (RB_SurfaceVaoCached(srf->numVerts, srf->verts, srf->numIndexes,
		srf->indexes, srf->dlightBits, srf->pshadowBits))
	{
		return;
	}

	RB_CheckVao(tess.vao);

	dlightBits = srf->dlightBits;
	tess.dlightBits |= dlightBits;

	pshadowBits = srf->pshadowBits;
	tess.pshadowBits |= pshadowBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( srf->lodOrigin, srf->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < srf->width-1 ; i++ ) {
		if ( srf->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = srf->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < srf->height-1 ; i++ ) {
		if ( srf->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = srf->height-1;
	lodHeight++;


	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex );
			} else {
				break;
			}
		} while ( 1 );
		
		rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz = tess.xyz[numVertexes];
		normal = tess.normal[numVertexes];
		tangent = tess.tangent[numVertexes];
		texCoords = tess.texCoords[numVertexes];
		lightCoords = tess.lightCoords[numVertexes];
		color = tess.color[numVertexes];
		lightdir = tess.lightdir[numVertexes];
		//vDlightBits = &tess.vertexDlightBits[numVertexes];

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = srf->verts + heightTable[ used + i ] * srf->width
					+ widthTable[ j ];

				if ( tess.shader->vertexAttribs & ATTR_POSITION )
				{
					VectorCopy(dv->xyz, xyz);
					xyz += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_NORMAL )
				{
					VectorCopy4(dv->normal, normal);
					normal += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_TANGENT )
				{
					VectorCopy4(dv->tangent, tangent);
					tangent += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_TEXCOORD )
				{
					VectorCopy2(dv->st, texCoords);
					texCoords += 2;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTCOORD )
				{
					VectorCopy2(dv->lightmap, lightCoords);
					lightCoords += 2;
				}

				if ( tess.shader->vertexAttribs & ATTR_COLOR )
				{
					VectorCopy4(dv->color, color);
					color += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
				{
					VectorCopy4(dv->lightdir, lightdir);
					lightdir += 4;
				}

				//*vDlightBits++ = dlightBits;
			}
		}


		// add the indexes
		{
			int		numIndexes;
			int		w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;
			for (i = 0 ; i < h ; i++) {
				for (j = 0 ; j < w ; j++) {
					int		v1, v2, v3, v4;
			
					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i*lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[numIndexes] = v2;
					tess.indexes[numIndexes+1] = v3;
					tess.indexes[numIndexes+2] = v1;
					
					tess.indexes[numIndexes+3] = v1;
					tess.indexes[numIndexes+4] = v3;
					tess.indexes[numIndexes+5] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis( void ) {
	// FIXME: implement this
#if 0
	GL_BindToTMU( tr.whiteImage, TB_COLORMAP );
	GL_State( GLS_DEFAULT );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
#endif
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void RB_SurfaceEntity( surfaceType_t *surfType ) {
	switch( backEnd.currentEntity->e.reType ) {
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_POLY_GLOBAL:
	case RT_POLY_LOCAL:
		{
			trRefEntity_t	*ent;
			srfPoly_t		polySurf;
			int				polyNum;

			ent = backEnd.currentEntity;

			polySurf.surfaceType = SF_POLY; //not used by RB_SurfacePolychain
			polySurf.hShader = ent->e.customShader; //ditto

			polySurf.numVerts = ent->numVerts;

			for ( polyNum = 0; polyNum < ent->numPolys; polyNum++ ) {
				polySurf.verts = &ent->verts[ ent->numVerts * polyNum ];
				RB_SurfacePolychain( &polySurf );
			}
		}
		break;
	default:
		RB_SurfaceAxis();
		break;
	}
}

static void RB_SurfaceBad( surfaceType_t *surfType ) {
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

static void RB_SurfaceFlare(srfFlare_t *surf)
{
	shader_t *shader;

	shader = tess.shader;

	// q3map2 doesn't add default flare shader I guess
	if ( shader == tr.defaultShader ) {
		shader = tr.flareShader;
	}

	if (r_flares->integer)
		RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, 1.0f, surf->normal, -1, qtrue, shader);
}

static void RB_SurfacePolyBuffer( srfPolyBuffer_t *surf ) {
	int		i;
	int		numv;

	RB_CHECKOVERFLOW( surf->pPolyBuffer->numVerts, surf->pPolyBuffer->numIndicies );

	numv = tess.numVertexes;
	for (i = 0; i < surf->pPolyBuffer->numVerts; i++) {
		VectorCopy( surf->pPolyBuffer->xyz[i], tess.xyz[numv] );
		tess.texCoords[numv][0] = surf->pPolyBuffer->st[i][0];
		tess.texCoords[numv][1] = surf->pPolyBuffer->st[i][1];
		tess.color[numv][0] = surf->pPolyBuffer->color[i][0] * 257;
		tess.color[numv][1] = surf->pPolyBuffer->color[i][1] * 257;
		tess.color[numv][2] = surf->pPolyBuffer->color[i][2] * 257;
		tess.color[numv][3] = surf->pPolyBuffer->color[i][3] * 257;

		numv++;
	}

	for (i = 0; i < surf->pPolyBuffer->numIndicies; i++) {
		tess.indexes[tess.numIndexes++] = tess.numVertexes + surf->pPolyBuffer->indicies[i];
	}

	tess.numVertexes = numv;
}

void RB_SurfaceVaoMdvMesh(srfVaoMdvMesh_t * surface)
{
	//mdvModel_t     *mdvModel;
	//mdvSurface_t   *mdvSurface;
	refEntity_t    *refEnt;

	GLimp_LogComment("--- RB_SurfaceVaoMdvMesh ---\n");

	if (ShaderRequiresCPUDeforms(tess.shader))
	{
		RB_SurfaceMesh(surface->mdvSurface);
		return;
	}

	if(!surface->vao)
		return;

	//RB_CheckVao(surface->vao);
	RB_EndSurface();
	RB_BeginSurface(tess.shader, tess.fogNum, tess.cubemapIndex);

	R_BindVao(surface->vao);

	tess.useInternalVao = qfalse;

	tess.numIndexes = surface->numIndexes;
	tess.numVertexes = surface->numVerts;

	//mdvModel = surface->mdvModel;
	//mdvSurface = surface->mdvSurface;

	refEnt = &backEnd.currentEntity->e;

	glState.vertexAttribsInterpolation = (refEnt->oldframe == refEnt->frame) ? 0.0f : refEnt->backlerp;

	if (surface->mdvModel->numFrames > 1)
	{
		int frameOffset, attribIndex;
		vaoAttrib_t *vAtb;

		glState.vertexAnimation = qtrue;

		if (glRefConfig.vertexArrayObject)
		{
			qglBindBuffer(GL_ARRAY_BUFFER, surface->vao->vertexesVBO);
		}

		frameOffset    = refEnt->frame * surface->vao->frameSize;

		attribIndex = ATTR_INDEX_POSITION;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_NORMAL;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_TANGENT;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		frameOffset = refEnt->oldframe * surface->vao->frameSize;

		attribIndex = ATTR_INDEX_POSITION2;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_NORMAL2;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));

		attribIndex = ATTR_INDEX_TANGENT2;
		vAtb = &surface->vao->attribs[attribIndex];
		qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset + frameOffset));


		if (!glRefConfig.vertexArrayObject)
		{
			attribIndex = ATTR_INDEX_TEXCOORD;
			vAtb = &surface->vao->attribs[attribIndex];
			qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));
		}
	}

	RB_EndSurface();

	// So we don't lerp surfaces that shouldn't be lerped
	glState.vertexAnimation = qfalse;
}

static void RB_SurfaceSkip( void *surf ) {
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) = {
	(void(*)(void*))RB_SurfaceBad,			// SF_BAD, 
	(void(*)(void*))RB_SurfaceSkip,			// SF_SKIP, 
	(void(*)(void*))RB_SurfaceFace,			// SF_FACE,
	(void(*)(void*))RB_SurfaceGrid,			// SF_GRID,
	(void(*)(void*))RB_SurfaceTriangles,		// SF_TRIANGLES,
	(void(*)(void*))RB_SurfaceFoliage,			// SF_FOLIAGE,
	(void(*)(void*))RB_SurfacePolychain,		// SF_POLY,
	(void(*)(void*))RB_SurfacePolyBuffer,		// SF_POLYBUFFER,
	(void(*)(void*))RB_SurfaceMesh,			// SF_MDV,
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
	(void(*)(void*))RB_MDSSurfaceAnim,		// SF_MDS,
	(void(*)(void*))RB_MDMSurfaceAnim,		// SF_MDM,
	(void(*)(void*))RB_IQMSurfaceAnim,		// SF_IQM,
	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
	(void(*)(void*))RB_SurfaceVaoMdvMesh,   // SF_VAO_MDVMESH
};
