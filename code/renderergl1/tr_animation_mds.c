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


#include "tr_local.h"

/*

All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.

*/

// undef to use floating-point lerping with explicit trig funcs
//#define YD_INGLES

//#define HIGH_PRECISION_BONES	// enable this for 32bit precision bones
//#define DBG_PROFILE_BONES

//-----------------------------------------------------------------------------
// Static Vars, ugly but easiest (and fastest) means of seperating RB_SurfaceAnim
// and R_CalcBones

static float frontlerp, backlerp;
static float torsoFrontlerp, torsoBacklerp;
static mdsBoneFrame_t bones[MDS_MAX_BONES], rawBones[MDS_MAX_BONES], oldBones[MDS_MAX_BONES];
static char validBones[MDS_MAX_BONES];
static char newBones[ MDS_MAX_BONES ];
static mdsBoneFrame_t  *bonePtr, *parentBone;
static mdsBoneFrameCompressed_t    *cBonePtr, *cTBonePtr, *cOldBonePtr, *cOldTBonePtr, *cBoneList, *cOldBoneList, *cBoneListTorso, *cOldBoneListTorso;
static mdsBoneInfo_t   *boneInfo, *thisBoneInfo, *parentBoneInfo;
static short           *sh, *sh2;
static float           *pf;
#ifdef YD_INGLES
static int ingles[ 3 ], tingles[ 3 ];
#else
static float a1, a2;
#endif
static vec3_t angles, tangles, torsoParentOffset, torsoAxis[3];
static vec3_t vec, v2, dir;
static float diff;
#ifndef DEDICATED
static int render_count;
static float lodScale;
#endif
static qboolean isTorso, fullTorso;
static vec4_t m1[4], m2[4];
// static  vec4_t m3[4], m4[4]; // TTimo unused
// static  vec4_t tmp1[4], tmp2[4]; // TTimo unused
static vec3_t t;
static refEntity_t lastBoneEntity;

static int totalrv, totalrt, totalv, totalt;    //----(SA)

//-----------------------------------------------------------------------------

#define MDS_FRAME( _header, _frame ) \
	( mdsFrame_t * )( (byte *)_header + _header->ofsFrames + _frame * (size_t)( &((mdsFrame_t *)0)->bones[ _header->numBones ] ) )

static mdsHeader_t *R_GetFrameModelDataByHandle( const refEntity_t *ent, qhandle_t hModel ) {
	model_t *mod;

	if ( !hModel ) {
		hModel = ent->hModel;
	}

	mod = R_GetModelByHandle( hModel );

	if ( mod->type != MOD_MDS ) {
		return NULL;
	}

	return mod->modelData;
}

#ifndef DEDICATED

static float RB_ProjectRadius( float r, vec3_t location ) {
	float pr;
	float dist;
	float c;
	vec3_t p;
	float projected[4];

	c = DotProduct( backEnd.viewParms.or.axis[0], backEnd.viewParms.or.origin );
	dist = DotProduct( backEnd.viewParms.or.axis[0], location ) - c;

	if ( dist <= 0 ) {
		return 0;
	}

	p[0] = 0;
	p[1] = fabs( r );
	p[2] = -dist;

	projected[0] = p[0] * backEnd.viewParms.projectionMatrix[0] +
				   p[1] * backEnd.viewParms.projectionMatrix[4] +
				   p[2] * backEnd.viewParms.projectionMatrix[8] +
				   backEnd.viewParms.projectionMatrix[12];

	projected[1] = p[0] * backEnd.viewParms.projectionMatrix[1] +
				   p[1] * backEnd.viewParms.projectionMatrix[5] +
				   p[2] * backEnd.viewParms.projectionMatrix[9] +
				   backEnd.viewParms.projectionMatrix[13];

	projected[2] = p[0] * backEnd.viewParms.projectionMatrix[2] +
				   p[1] * backEnd.viewParms.projectionMatrix[6] +
				   p[2] * backEnd.viewParms.projectionMatrix[10] +
				   backEnd.viewParms.projectionMatrix[14];

	projected[3] = p[0] * backEnd.viewParms.projectionMatrix[3] +
				   p[1] * backEnd.viewParms.projectionMatrix[7] +
				   p[2] * backEnd.viewParms.projectionMatrix[11] +
				   backEnd.viewParms.projectionMatrix[15];


	pr = projected[1] / projected[3];

	if ( pr > 1.0f ) {
		pr = 1.0f;
	}

	return pr;
}

/*
=============
R_CullModel
=============
*/
static int R_CullModel( trRefEntity_t *ent ) {
	vec3_t bounds[2];
	mdsHeader_t *oldHeader, *newHeader;
	mdsFrame_t  *oldFrame, *newFrame;
	int i;

	newHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.frameModel );
	oldHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.oldframeModel );

	if ( !newHeader || !oldHeader ) {
		return CULL_OUT;
	}

	// compute frame pointers
	newFrame = MDS_FRAME( newHeader, ent->e.frame );
	oldFrame = MDS_FRAME( oldHeader, ent->e.oldframe );

	// cull bounding sphere ONLY if this is not an upscaled entity
	if ( !ent->e.nonNormalizedAxes ) {
		if ( ent->e.frame == ent->e.oldframe && ent->e.frameModel == ent->e.oldframeModel ) {
			switch ( R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius ) )
			{
			case CULL_OUT:
				tr.pc.c_sphere_cull_md3_out++;
				return CULL_OUT;

			case CULL_IN:
				tr.pc.c_sphere_cull_md3_in++;
				return CULL_IN;

			case CULL_CLIP:
				tr.pc.c_sphere_cull_md3_clip++;
				break;
			}
		} else
		{
			int sphereCull, sphereCullB;

			sphereCull  = R_CullLocalPointAndRadius( newFrame->localOrigin, newFrame->radius );
			if ( newFrame == oldFrame ) {
				sphereCullB = sphereCull;
			} else {
				sphereCullB = R_CullLocalPointAndRadius( oldFrame->localOrigin, oldFrame->radius );
			}

			if ( sphereCull == sphereCullB ) {
				if ( sphereCull == CULL_OUT ) {
					tr.pc.c_sphere_cull_md3_out++;
					return CULL_OUT;
				} else if ( sphereCull == CULL_IN )   {
					tr.pc.c_sphere_cull_md3_in++;
					return CULL_IN;
				} else
				{
					tr.pc.c_sphere_cull_md3_clip++;
				}
			}
		}
	}

	// calculate a bounding box in the current coordinate system
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = oldFrame->bounds[0][i] < newFrame->bounds[0][i] ? oldFrame->bounds[0][i] : newFrame->bounds[0][i];
		bounds[1][i] = oldFrame->bounds[1][i] > newFrame->bounds[1][i] ? oldFrame->bounds[1][i] : newFrame->bounds[1][i];
	}

	switch ( R_CullLocalBox( bounds ) )
	{
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}

/*
=================
RB_CalcMDSLod

=================
*/
static float RB_CalcMDSLod( refEntity_t *refent, float modelBias, float modelScale ) {
	mdsHeader_t *frameHeader;
	mdsFrame_t  *frame;
	vec3_t origin;
	float radius;
	float flod, lodScale;
	float projectedRadius;

	frameHeader = R_GetFrameModelDataByHandle( refent, refent->frameModel );

	if ( !frameHeader ) {
		return 1.0f;
	}

	frame = MDS_FRAME( frameHeader, refent->frame );

	// TODO: lerp the radius and origin
	VectorAdd( refent->origin, frame->localOrigin, origin );
	radius = frame->radius;

	// compute projected bounding sphere and use that as a criteria for selecting LOD

	projectedRadius = RB_ProjectRadius( radius, origin );
	if ( projectedRadius != 0 ) {

//		ri.Printf (PRINT_ALL, "projected radius: %f\n", projectedRadius);

		lodScale = r_lodscale->value;   // fudge factor since MDS uses a much smoother method of LOD
		flod = projectedRadius * lodScale * modelScale;
	} else
	{
		// object intersects near view plane, e.g. view weapon
		flod = 1.0f;
	}

#if 0 // ZTM: FIXME?: Flags not supported yet
	if ( refent->reFlags & REFLAG_FORCE_LOD ) {
		flod *= 0.5;
	}
//----(SA)	like reflag_force_lod, but separate for the moment
	if ( refent->reFlags & REFLAG_DEAD_LOD ) {
		flod *= 0.8;
	}
#endif

	flod -= 0.25 * ( r_lodbias->value ) + modelBias;

	if ( flod < 0.0 ) {
		flod = 0.0;
	} else if ( flod > 1.0f ) {
		flod = 1.0f;
	}

	return flod;
}

/*
=================
R_ComputeFogNum

=================
*/
static int R_ComputeFogNum( trRefEntity_t *ent ) {
	mdsHeader_t		*header;
	mdsFrame_t		*frame;
	vec3_t			localOrigin;

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	header = R_GetFrameModelDataByHandle( &ent->e, ent->e.frameModel );

	if ( !header )
		return 0;

	// FIXME: non-normalized axis issues
	frame = MDS_FRAME( header, ent->e.frame );
	VectorAdd( ent->e.origin, frame->localOrigin, localOrigin );

	return R_PointFogNum( &tr.refdef, localOrigin, frame->radius );
}

/*
==============
R_MDSAddAnimSurfaces
==============
*/
void R_MDSAddAnimSurfaces( trRefEntity_t *ent ) {
	mdsHeader_t     *header;
	mdsSurface_t    *surface;
	mdsHeader_t     *frameHeader, *oldFrameHeader;
	mdsHeader_t     *torsoFrameHeader, *oldTorsoFrameHeader;
	shader_t        *shader;
	int i, fogNum, cull;
	qboolean personalModel;

	// don't add mirror only objects if not in a mirror/portal
	personalModel = (ent->e.renderfx & RF_ONLY_MIRROR) && !tr.viewParms.isPortal;

	// if missing torso information use base
	if ( AxisEmpty( ent->e.torsoAxis ) ) {
		// setup identify matrix
		AxisCopy( axisDefault, ent->e.torsoAxis );

		ent->e.torsoFrameModel = ent->e.frameModel;
		ent->e.oldTorsoFrameModel = ent->e.oldframeModel;
		ent->e.torsoFrame = ent->e.frame;
		ent->e.oldTorsoFrame = ent->e.oldframe;
		ent->e.torsoBacklerp = ent->e.backlerp;
	}

	header = (mdsHeader_t *)tr.currentModel->modelData;

	frameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.frameModel );
	oldFrameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.oldframeModel );
	torsoFrameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.torsoFrameModel );
	oldTorsoFrameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.oldTorsoFrameModel );

	if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
		ent->e.frame %= frameHeader->numFrames;
		ent->e.oldframe %= oldFrameHeader->numFrames;
		ent->e.torsoFrame %= torsoFrameHeader->numFrames;
		ent->e.oldTorsoFrame %= oldTorsoFrameHeader->numFrames;
	}

	//
	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	//
	if ( (ent->e.frame >= frameHeader->numFrames)
		|| (ent->e.frame < 0)
		|| (ent->e.oldframe >= oldFrameHeader->numFrames)
		|| (ent->e.oldframe < 0) ) {
			ri.Printf( PRINT_DEVELOPER, "R_MDSAddAnimSurfaces: no such frame %d to %d for '%s'\n",
				ent->e.oldframe, ent->e.frame,
				tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	if ( (ent->e.torsoFrame >= torsoFrameHeader->numFrames)
		|| (ent->e.torsoFrame < 0)
		|| (ent->e.oldTorsoFrame >= oldTorsoFrameHeader->numFrames)
		|| (ent->e.oldTorsoFrame < 0) ) {
			ri.Printf( PRINT_DEVELOPER, "R_MDSAddAnimSurfaces: no such torso frame %d to %d for '%s'\n",
				ent->e.oldTorsoFrame, ent->e.torsoFrame,
				tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	//
	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum.
	//
	cull = R_CullModel( ent );
	if ( cull == CULL_OUT ) {
		return;
	}

	//
	// set up lighting now that we know we aren't culled
	//
	if ( !personalModel || r_shadows->integer > 1 ) {
		R_SetupEntityLighting( &tr.refdef, ent );
	}

	//
	// see if we are in a fog volume
	//
	fogNum = R_ComputeFogNum( ent );

	surface = ( mdsSurface_t * )( (byte *)header + header->ofsSurfaces );
	for ( i = 0 ; i < header->numSurfaces ; i++ ) {

		if ( ent->e.customShader || ent->e.customSkin ) {
			shader = R_CustomSurfaceShader( surface->name, ent->e.customShader, ent->e.customSkin );
			if (shader == tr.nodrawShader) {
				surface = ( mdsSurface_t * )( (byte *)surface + surface->ofsEnd );
				continue;
			}
		}
		else if(surface->shaderIndex > 0)
			shader = R_GetShaderByHandle( surface->shaderIndex );
		else
			shader = tr.defaultShader;

		// we will add shadows even if the main object isn't visible in the view

		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel
		        && r_shadows->integer == 2
			&& fogNum == 0
			&& !(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) )
			&& shader->sort == SS_OPAQUE )
		{
			R_AddDrawSurf( (void *)surface, tr.shadowShader, 0, qfalse );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3
			&& fogNum == 0
			&& (ent->e.renderfx & RF_SHADOW_PLANE )
			&& shader->sort == SS_OPAQUE )
		{
			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, qfalse );
		}

		if (!personalModel)
			R_AddEntDrawSurf( ent, (void *)surface, shader, fogNum, qfalse, 0 );

		surface = ( mdsSurface_t * )( (byte *)surface + surface->ofsEnd );
	}
}

#endif // !DEDICATED

static ID_INLINE void LocalMatrixTransformVector( vec3_t in, vec3_t mat[ 3 ], vec3_t out ) {
	out[ 0 ] = in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ];
	out[ 1 ] = in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ];
	out[ 2 ] = in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ];
}

#if 0
static ID_INLINE void LocalMatrixTransformVectorTranslate( vec3_t in, vec3_t mat[ 3 ], vec3_t tr, vec3_t out ) {
	out[ 0 ] = in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] + tr[ 0 ];
	out[ 1 ] = in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] + tr[ 1 ];
	out[ 2 ] = in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] + tr[ 2 ];
}
#endif

static ID_INLINE void LocalScaledMatrixTransformVector( vec3_t in, float s, vec3_t mat[ 3 ], vec3_t out ) {
	out[ 0 ] = ( 1.0f - s ) * in[ 0 ] + s * ( in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] );
	out[ 1 ] = ( 1.0f - s ) * in[ 1 ] + s * ( in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] );
	out[ 2 ] = ( 1.0f - s ) * in[ 2 ] + s * ( in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] );
}

#if 0
static ID_INLINE void LocalScaledMatrixTransformVectorTranslate( vec3_t in, float s, vec3_t mat[ 3 ], vec3_t tr, vec3_t out ) {
	out[ 0 ] = ( 1.0f - s ) * in[ 0 ] + s * ( in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] + tr[ 0 ] );
	out[ 1 ] = ( 1.0f - s ) * in[ 1 ] + s * ( in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] + tr[ 1 ] );
	out[ 2 ] = ( 1.0f - s ) * in[ 2 ] + s * ( in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] + tr[ 2 ] );
}

static ID_INLINE void LocalScaledMatrixTransformVectorFullTranslate( vec3_t in, float s, vec3_t mat[ 3 ], vec3_t tr, vec3_t out ) {
	out[ 0 ] = ( 1.0f - s ) * in[ 0 ] + s * ( in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] ) + tr[ 0 ];
	out[ 1 ] = ( 1.0f - s ) * in[ 1 ] + s * ( in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] ) + tr[ 1 ];
	out[ 2 ] = ( 1.0f - s ) * in[ 2 ] + s * ( in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] ) + tr[ 2 ];
}

static ID_INLINE void LocalAddScaledMatrixTransformVectorFullTranslate( vec3_t in, float s, vec3_t mat[ 3 ], vec3_t tr, vec3_t out ) {
	out[ 0 ] += s * ( in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] ) + tr[ 0 ];
	out[ 1 ] += s * ( in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] ) + tr[ 1 ];
	out[ 2 ] += s * ( in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] ) + tr[ 2 ];
}
#endif

static ID_INLINE void LocalAddScaledMatrixTransformVectorTranslate( vec3_t in, float s, vec3_t mat[ 3 ], vec3_t tr, vec3_t out ) {
	out[ 0 ] += s * ( in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] + tr[ 0 ] );
	out[ 1 ] += s * ( in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] + tr[ 1 ] );
	out[ 2 ] += s * ( in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] + tr[ 2 ] );
}

#if 0
static ID_INLINE void LocalAddScaledMatrixTransformVector( vec3_t in, float s, vec3_t mat[ 3 ], vec3_t out ) {
	out[ 0 ] += s * ( in[ 0 ] * mat[ 0 ][ 0 ] + in[ 1 ] * mat[ 0 ][ 1 ] + in[ 2 ] * mat[ 0 ][ 2 ] );
	out[ 1 ] += s * ( in[ 0 ] * mat[ 1 ][ 0 ] + in[ 1 ] * mat[ 1 ][ 1 ] + in[ 2 ] * mat[ 1 ][ 2 ] );
	out[ 2 ] += s * ( in[ 0 ] * mat[ 2 ][ 0 ] + in[ 1 ] * mat[ 2 ][ 1 ] + in[ 2 ] * mat[ 2 ][ 2 ] );
}
#endif

static float LAVangle;
static float sp, sy, cp, cy;
#ifdef YD_INGLES
static float sr, cr;
#endif

static ID_INLINE void LocalAngleVector( vec3_t angles, vec3_t forward ) {
	LAVangle = angles[YAW] * ( M_PI * 2 / 360 );
	sy = sin( LAVangle );
	cy = cos( LAVangle );
	LAVangle = angles[PITCH] * ( M_PI * 2 / 360 );
	sp = sin( LAVangle );
	cp = cos( LAVangle );

	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
}

static ID_INLINE void LocalVectorMA( vec3_t org, float dist, vec3_t vec, vec3_t out ) {
	out[0] = org[0] + dist * vec[0];
	out[1] = org[1] + dist * vec[1];
	out[2] = org[2] + dist * vec[2];
}

#define ANGLES_SHORT_TO_FLOAT( pf, sh )     { *( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = SHORT2ANGLE( *( sh++ ) ); }

static ID_INLINE void SLerp_Normal( vec3_t from, vec3_t to, float tt, vec3_t out ) {
	float ft = 1.0 - tt;

	out[0] = from[0] * ft + to[0] * tt;
	out[1] = from[1] * ft + to[1] * tt;
	out[2] = from[2] * ft + to[2] * tt;

	VectorNormalize( out );
}

#ifdef YD_INGLES

#define FUNCTABLE_SHIFT     ( 16 - FUNCTABLE_SIZE2 )
#define SIN_TABLE( i )      tr.sinTable[ ( i ) >> FUNCTABLE_SHIFT ];
#define COS_TABLE( i )      tr.sinTable[ ( ( ( i ) >> FUNCTABLE_SHIFT ) + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];

static ID_INLINE void LocalIngleVector( int ingles[ 3 ], vec3_t forward ) {
	sy = SIN_TABLE( ingles[ YAW ] & 65535 );
	cy = COS_TABLE( ingles[ YAW ] & 65535 );
	sp = SIN_TABLE( ingles[ PITCH ] & 65535 );
	cp = COS_TABLE( ingles[ PITCH ] & 65535 );

	//%	sy = sin( SHORT2ANGLE( ingles[ YAW ] ) * (M_PI*2 / 360) );
	//%	cy = cos( SHORT2ANGLE( ingles[ YAW ] ) * (M_PI*2 / 360) );
	//%	sp = sin( SHORT2ANGLE( ingles[ PITCH ] ) * (M_PI*2 / 360) );
	//%	cp = cos( SHORT2ANGLE( ingles[ PITCH ] ) *  (M_PI*2 / 360) );

	forward[ 0 ] = cp * cy;
	forward[ 1 ] = cp * sy;
	forward[ 2 ] = -sp;
}

static void InglesToAxis( int ingles[ 3 ], vec3_t axis[ 3 ] ) {
	// get sine/cosines for angles
	sy = SIN_TABLE( ingles[ YAW ] & 65535 );
	cy = COS_TABLE( ingles[ YAW ] & 65535 );
	sp = SIN_TABLE( ingles[ PITCH ] & 65535 );
	cp = COS_TABLE( ingles[ PITCH ] & 65535 );
	sr = SIN_TABLE( ingles[ ROLL ] & 65535 );
	cr = COS_TABLE( ingles[ ROLL ] & 65535 );

	// calculate axis vecs
	axis[ 0 ][ 0 ] = cp * cy;
	axis[ 0 ][ 1 ] = cp * sy;
	axis[ 0 ][ 2 ] = -sp;

	axis[ 1 ][ 0 ] = sr * sp * cy + cr * -sy;
	axis[ 1 ][ 1 ] = sr * sp * sy + cr * cy;
	axis[ 1 ][ 2 ] = sr * cp;

	axis[ 2 ][ 0 ] = cr * sp * cy + - sr * -sy;
	axis[ 2 ][ 1 ] = cr * sp * sy + - sr * cy;
	axis[ 2 ][ 2 ] = cr * cp;
}

#endif // YD_INGLES


/*
===============================================================================

4x4 Matrices

===============================================================================
*/

static ID_INLINE void Matrix4Multiply( const vec4_t a[4], const vec4_t b[4], vec4_t dst[4] ) {
	dst[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
	dst[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
	dst[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
	dst[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

	dst[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
	dst[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
	dst[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
	dst[1][3] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

	dst[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
	dst[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
	dst[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
	dst[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];

	dst[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];
	dst[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];
	dst[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];
	dst[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];
}

// TTimo: const usage would require an explicit cast, non ANSI C
// see unix/const-arg.c
static ID_INLINE void Matrix4MultiplyInto3x3AndTranslation( /*const*/ vec4_t a[4], /*const*/ vec4_t b[4], vec3_t dst[3], vec3_t t ) {
	dst[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];
	dst[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];
	dst[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];
	t[0]      = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];

	dst[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];
	dst[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];
	dst[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];
	t[1]      = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];

	dst[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];
	dst[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];
	dst[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];
	t[2]      = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];
}

static ID_INLINE void Matrix4Transpose( const vec4_t matrix[4], vec4_t transpose[4] ) {
	int i, j;
	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

static ID_INLINE void Matrix4FromAxis( const vec3_t axis[3], vec4_t dst[4] ) {
	int i, j;
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			dst[i][j] = axis[i][j];
		}
		dst[3][i] = 0;
		dst[i][3] = 0;
	}
	dst[3][3] = 1;
}

static ID_INLINE void Matrix4FromScaledAxis( const vec3_t axis[3], const float scale, vec4_t dst[4] ) {
	int i, j;

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			dst[i][j] = scale * axis[i][j];
			if ( i == j ) {
				dst[i][j] += 1.0f - scale;
			}
		}
		dst[3][i] = 0;
		dst[i][3] = 0;
	}
	dst[3][3] = 1;
}

static ID_INLINE void Matrix4FromTranslation( const vec3_t t, vec4_t dst[4] ) {
	int i, j;

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			if ( i == j ) {
				dst[i][j] = 1;
			} else {
				dst[i][j] = 0;
			}
		}
		dst[i][3] = t[i];
		dst[3][i] = 0;
	}
	dst[3][3] = 1;
}

// can put an axis rotation followed by a translation directly into one matrix
// TTimo: const usage would require an explicit cast, non ANSI C
// see unix/const-arg.c
static ID_INLINE void Matrix4FromAxisPlusTranslation( /*const*/ vec3_t axis[3], const vec3_t t, vec4_t dst[4] ) {
	int i, j;
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			dst[i][j] = axis[i][j];
		}
		dst[3][i] = 0;
		dst[i][3] = t[i];
	}
	dst[3][3] = 1;
}

// can put a scaled axis rotation followed by a translation directly into one matrix
// TTimo: const usage would require an explicit cast, non ANSI C
// see unix/const-arg.c
static ID_INLINE void Matrix4FromScaledAxisPlusTranslation( /*const*/ vec3_t axis[3], const float scale, const vec3_t t, vec4_t dst[4] ) {
	int i, j;

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			dst[i][j] = scale * axis[i][j];
			if ( i == j ) {
				dst[i][j] += 1.0f - scale;
			}
		}
		dst[3][i] = 0;
		dst[i][3] = t[i];
	}
	dst[3][3] = 1;
}

static ID_INLINE void Matrix4FromScale( const float scale, vec4_t dst[4] ) {
	int i, j;

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			if ( i == j ) {
				dst[i][j] = scale;
			} else {
				dst[i][j] = 0;
			}
		}
	}
	dst[3][3] = 1;
}

static ID_INLINE void Matrix4TransformVector( const vec4_t m[4], const vec3_t src, vec3_t dst ) {
	dst[0] = m[0][0] * src[0] + m[0][1] * src[1] + m[0][2] * src[2] + m[0][3];
	dst[1] = m[1][0] * src[0] + m[1][1] * src[1] + m[1][2] * src[2] + m[1][3];
	dst[2] = m[2][0] * src[0] + m[2][1] * src[1] + m[2][2] * src[2] + m[2][3];
}

/*
===============================================================================

3x3 Matrices

===============================================================================
*/

static ID_INLINE void Matrix3Transpose( const vec3_t matrix[3], vec3_t transpose[3] ) {
	int i, j;
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			transpose[i][j] = matrix[j][i];
		}
	}
}


/*
==============
R_CalcBone
==============
*/
static void R_CalcBone( mdsFrame_t *frame, int torsoParent, int boneNum ) {
	int j;

	thisBoneInfo = &boneInfo[boneNum];
	if ( thisBoneInfo->torsoWeight ) {
		cTBonePtr = &cBoneListTorso[boneNum];
		isTorso = qtrue;
		if ( thisBoneInfo->torsoWeight == 1.0f ) {
			fullTorso = qtrue;
		}
	} else {
		isTorso = qfalse;
		fullTorso = qfalse;
	}
	cBonePtr = &cBoneList[boneNum];

	bonePtr = &bones[ boneNum ];

	// we can assume the parent has already been uncompressed for this frame + lerp
	if ( thisBoneInfo->parent >= 0 ) {
		parentBone = &bones[ thisBoneInfo->parent ];
		parentBoneInfo = &boneInfo[ thisBoneInfo->parent ];
	} else {
		parentBone = NULL;
		parentBoneInfo = NULL;
	}

#ifdef HIGH_PRECISION_BONES
	// rotation
	if ( fullTorso ) {
		VectorCopy( cTBonePtr->angles, angles );
	} else {
		VectorCopy( cBonePtr->angles, angles );
		if ( isTorso ) {
			VectorCopy( cTBonePtr->angles, tangles );
			// blend the angles together
			for ( j = 0; j < 3; j++ ) {
				diff = tangles[j] - angles[j];
				if ( fabs( diff ) > 180 ) {
					diff = AngleNormalize180( diff );
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}
		}
	}
#else
	// rotation
	if ( fullTorso ) {
		sh = (short *)cTBonePtr->angles;
		pf = angles;
		ANGLES_SHORT_TO_FLOAT( pf, sh );
	} else {
		sh = (short *)cBonePtr->angles;
		pf = angles;
		ANGLES_SHORT_TO_FLOAT( pf, sh );
		if ( isTorso ) {
			sh = (short *)cTBonePtr->angles;
			pf = tangles;
			ANGLES_SHORT_TO_FLOAT( pf, sh );
			// blend the angles together
			for ( j = 0; j < 3; j++ ) {
				diff = tangles[j] - angles[j];
				if ( fabs( diff ) > 180 ) {
					diff = AngleNormalize180( diff );
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}
		}
	}
#endif
	AnglesToAxis( angles, bonePtr->matrix );

	// translation
	if ( parentBone ) {

#ifdef HIGH_PRECISION_BONES
		if ( fullTorso ) {
			angles[0] = cTBonePtr->ofsAngles[0];
			angles[1] = cTBonePtr->ofsAngles[1];
			angles[2] = 0;
			LocalAngleVector( angles, vec );
			LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation );
		} else {

			angles[0] = cBonePtr->ofsAngles[0];
			angles[1] = cBonePtr->ofsAngles[1];
			angles[2] = 0;
			LocalAngleVector( angles, vec );

			if ( isTorso ) {
				tangles[0] = cTBonePtr->ofsAngles[0];
				tangles[1] = cTBonePtr->ofsAngles[1];
				tangles[2] = 0;
				LocalAngleVector( tangles, v2 );

				// blend the angles together
				SLerp_Normal( vec, v2, thisBoneInfo->torsoWeight, vec );
				LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation );

			} else {    // legs bone
				LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation );
			}
		}
#else
		if ( fullTorso ) {
			#ifndef YD_INGLES
			sh = (short *)cTBonePtr->ofsAngles; pf = angles;
			*( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = 0;
			LocalAngleVector( angles, vec );
			#else
			sh = (short*) cTBonePtr->ofsAngles;
			ingles[ 0 ] = sh[ 0 ];
			ingles[ 1 ] = sh[ 1 ];
			ingles[ 2 ] = 0;
			LocalIngleVector( ingles, vec );
			#endif
			LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation );
		} else {

			#ifndef YD_INGLES
			sh = (short *)cBonePtr->ofsAngles; pf = angles;
			*( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = 0;
			LocalAngleVector( angles, vec );
			#else
			sh = (short*) cBonePtr->ofsAngles;
			ingles[ 0 ] = sh[ 0 ];
			ingles[ 1 ] = sh[ 1 ];
			ingles[ 2 ] = 0;
			LocalIngleVector( ingles, vec );
			#endif

			if ( isTorso ) {
				#ifndef YD_INGLES
				sh = (short *)cTBonePtr->ofsAngles;
				pf = tangles;
				*( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = SHORT2ANGLE( *( sh++ ) ); *( pf++ ) = 0;
				LocalAngleVector( tangles, v2 );
				#else
				sh = (short*) cTBonePtr->ofsAngles;
				tingles[ 0 ] = sh[ 0 ];
				tingles[ 1 ] = sh[ 1 ];
				tingles[ 2 ] = 0;
				LocalIngleVector( tingles, v2 );
				#endif

				// blend the angles together
				SLerp_Normal( vec, v2, thisBoneInfo->torsoWeight, vec );
				LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation );

			} else {    // legs bone
				LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, vec, bonePtr->translation );
			}
		}
#endif
	} else {    // just use the frame position
		bonePtr->translation[0] = frame->parentOffset[0];
		bonePtr->translation[1] = frame->parentOffset[1];
		bonePtr->translation[2] = frame->parentOffset[2];
	}
	//
	if ( boneNum == torsoParent ) { // this is the torsoParent
		VectorCopy( bonePtr->translation, torsoParentOffset );
	}
	//
	validBones[boneNum] = 1;
	//
	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;

}

/*
==============
R_CalcBoneLerp
==============
*/
static void R_CalcBoneLerp( mdsFrame_t *frame, mdsFrame_t *oldFrame, int torsoParent, int boneNum ) {
	int j;

	if ( boneNum < 0 || boneNum >= MDS_MAX_BONES ) {
		return;
	}


	thisBoneInfo = &boneInfo[boneNum];

	if ( !thisBoneInfo ) {
		return;
	}

	if ( thisBoneInfo->parent >= 0 ) {
		parentBone = &bones[ thisBoneInfo->parent ];
		parentBoneInfo = &boneInfo[ thisBoneInfo->parent ];
	} else {
		parentBone = NULL;
		parentBoneInfo = NULL;
	}

	if ( thisBoneInfo->torsoWeight ) {
		cTBonePtr = &cBoneListTorso[boneNum];
		cOldTBonePtr = &cOldBoneListTorso[boneNum];
		isTorso = qtrue;
		if ( thisBoneInfo->torsoWeight == 1.0f ) {
			fullTorso = qtrue;
		}
	} else {
		isTorso = qfalse;
		fullTorso = qfalse;
	}
	cBonePtr = &cBoneList[boneNum];
	cOldBonePtr = &cOldBoneList[boneNum];

	bonePtr = &bones[boneNum];

	newBones[ boneNum ] = 1;

	// rotation (take into account 170 to -170 lerps, which need to take the shortest route)
	#ifndef YD_INGLES

	if ( fullTorso ) {

		sh = (short *)cTBonePtr->angles;
		sh2 = (short *)cOldTBonePtr->angles;
		pf = angles;

		a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
		*( pf++ ) = a1 - torsoBacklerp * diff;
		a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
		*( pf++ ) = a1 - torsoBacklerp * diff;
		a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
		*( pf++ ) = a1 - torsoBacklerp * diff;

	} else {

		sh = (short *)cBonePtr->angles;
		sh2 = (short *)cOldBonePtr->angles;
		pf = angles;

		a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
		*( pf++ ) = a1 - backlerp * diff;
		a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
		*( pf++ ) = a1 - backlerp * diff;
		a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
		*( pf++ ) = a1 - backlerp * diff;

		if ( isTorso ) {

			sh = (short *)cTBonePtr->angles;
			sh2 = (short *)cOldTBonePtr->angles;
			pf = tangles;

			a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
			*( pf++ ) = a1 - torsoBacklerp * diff;
			a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
			*( pf++ ) = a1 - torsoBacklerp * diff;
			a1 = SHORT2ANGLE( *( sh++ ) ); a2 = SHORT2ANGLE( *( sh2++ ) ); diff = AngleNormalize180( a1 - a2 );
			*( pf++ ) = a1 - torsoBacklerp * diff;

			// blend the angles together
			for ( j = 0; j < 3; j++ ) {
				diff = tangles[j] - angles[j];
				if ( fabs( diff ) > 180 ) {
					diff = AngleNormalize180( diff );
				}
				angles[j] = angles[j] + thisBoneInfo->torsoWeight * diff;
			}

		}

	}
	AnglesToAxis( angles, bonePtr->matrix );

	#else

	// ydnar: ingles-based bone code
	if ( fullTorso ) {
		sh = (short*) cTBonePtr->angles;
		sh2 = (short*) cOldTBonePtr->angles;
		for ( j = 0; j < 3; j++ )
		{
			ingles[ j ] = ( sh[ j ] - sh2[ j ] ) & 65535;
			if ( ingles[ j ] > 32767 ) {
				ingles[ j ] -= 65536;
			}
			ingles[ j ] = sh[ j ] - torsoBacklerp * ingles[ j ];
		}
	} else
	{
		sh = (short*) cBonePtr->angles;
		sh2 = (short*) cOldBonePtr->angles;
		for ( j = 0; j < 3; j++ )
		{
			ingles[ j ] = ( sh[ j ] - sh2[ j ] ) & 65535;
			if ( ingles[ j ] > 32767 ) {
				ingles[ j ] -= 65536;
			}
			ingles[ j ] = sh[ j ] - backlerp * ingles[ j ];
		}

		if ( isTorso ) {
			sh = (short*) cTBonePtr->angles;
			sh2 = (short*) cOldTBonePtr->angles;
			for ( j = 0; j < 3; j++ )
			{
				tingles[ j ] = ( sh[ j ] - sh2[ j ] ) & 65535;
				if ( tingles[ j ] > 32767 ) {
					tingles[ j ] -= 65536;
				}
				tingles[ j ] = sh[ j ] - torsoBacklerp * tingles[ j ];

				// blend torso and angles
				tingles[ j ] = ( tingles[ j ] - ingles[ j ] ) & 65535;
				if ( tingles[ j ] > 32767 ) {
					tingles[ j ] -= 65536;
				}
				ingles[ j ] += thisBoneInfo->torsoWeight * tingles[ j ];
			}
		}

	}

	InglesToAxis( ingles, bonePtr->matrix );

	#endif

	if ( parentBone ) {

		if ( fullTorso ) {
			sh = (short *)cTBonePtr->ofsAngles;
			sh2 = (short *)cOldTBonePtr->ofsAngles;
		} else {
			sh = (short *)cBonePtr->ofsAngles;
			sh2 = (short *)cOldBonePtr->ofsAngles;
		}

		#ifndef YD_INGLES
		pf = angles;
		*( pf++ ) = SHORT2ANGLE( *( sh++ ) );
		*( pf++ ) = SHORT2ANGLE( *( sh++ ) );
		*( pf++ ) = 0;
		LocalAngleVector( angles, v2 );     // new

		pf = angles;
		*( pf++ ) = SHORT2ANGLE( *( sh2++ ) );
		*( pf++ ) = SHORT2ANGLE( *( sh2++ ) );
		*( pf++ ) = 0;
		LocalAngleVector( angles, vec );    // old
		#else
		ingles[ 0 ] = sh[ 0 ];
		ingles[ 1 ] = sh[ 1 ];
		ingles[ 2 ] = 0;
		LocalIngleVector( ingles, v2 );     // new

		ingles[ 0 ] = sh2[ 0 ];
		ingles[ 1 ] = sh2[ 1 ];
		ingles[ 2 ] = 0;
		LocalIngleVector( ingles, vec );    // old
		#endif

		// blend the angles together
		if ( fullTorso ) {
			SLerp_Normal( vec, v2, torsoFrontlerp, dir );
		} else {
			SLerp_Normal( vec, v2, frontlerp, dir );
		}

		// translation
		if ( !fullTorso && isTorso ) {    // partial legs/torso, need to lerp according to torsoWeight

			// calc the torso frame
			sh = (short *)cTBonePtr->ofsAngles;
			sh2 = (short *)cOldTBonePtr->ofsAngles;

			#ifndef YD_INGLES
			pf = angles;
			*( pf++ ) = SHORT2ANGLE( *( sh++ ) );
			*( pf++ ) = SHORT2ANGLE( *( sh++ ) );
			*( pf++ ) = 0;
			LocalAngleVector( angles, v2 );     // new

			pf = angles;
			*( pf++ ) = SHORT2ANGLE( *( sh2++ ) );
			*( pf++ ) = SHORT2ANGLE( *( sh2++ ) );
			*( pf++ ) = 0;
			LocalAngleVector( angles, vec );    // old
			#else
			ingles[ 0 ] = sh[ 0 ];
			ingles[ 1 ] = sh[ 1 ];
			ingles[ 2 ] = 0;
			LocalIngleVector( ingles, v2 );     // new

			ingles[ 0 ] = sh[ 0 ];
			ingles[ 1 ] = sh[ 1 ];
			ingles[ 2 ] = 0;
			LocalIngleVector( ingles, vec );    // old
			#endif

			// blend the angles together
			SLerp_Normal( vec, v2, torsoFrontlerp, v2 );

			// blend the torso/legs together
			SLerp_Normal( dir, v2, thisBoneInfo->torsoWeight, dir );

		}

		LocalVectorMA( parentBone->translation, thisBoneInfo->parentDist, dir, bonePtr->translation );

	} else {    // just interpolate the frame positions

		bonePtr->translation[0] = frontlerp * frame->parentOffset[0] + backlerp * oldFrame->parentOffset[0];
		bonePtr->translation[1] = frontlerp * frame->parentOffset[1] + backlerp * oldFrame->parentOffset[1];
		bonePtr->translation[2] = frontlerp * frame->parentOffset[2] + backlerp * oldFrame->parentOffset[2];

	}
	//
	if ( boneNum == torsoParent ) { // this is the torsoParent
		VectorCopy( bonePtr->translation, torsoParentOffset );
	}
	validBones[boneNum] = 1;
	//
	rawBones[boneNum] = *bonePtr;
	newBones[boneNum] = 1;

}


/*
==============
R_BonesStillValid

	FIXME: optimization opportunity here, profile which values change most often and check for those first to get early outs

	Other way we could do this is doing a random memory probe, which in worst case scenario ends up being the memcmp? - BAD as only a few values are used

	Another solution: bones cache on an entity basis?
==============
*/
static qboolean R_BonesStillValid( const refEntity_t *refent ) {
	if ( lastBoneEntity.hModel != refent->hModel ) {
		return qfalse;
	} else if ( lastBoneEntity.frame != refent->frame ) {
		return qfalse;
	} else if ( lastBoneEntity.oldframe != refent->oldframe ) {
		return qfalse;
	} else if ( lastBoneEntity.frameModel != refent->frameModel ) {
		return qfalse;
	} else if ( lastBoneEntity.oldframeModel != refent->oldframeModel ) {
		return qfalse;
	} else if ( lastBoneEntity.backlerp != refent->backlerp ) {
		return qfalse;
	} else if ( lastBoneEntity.torsoFrame != refent->torsoFrame ) {
		return qfalse;
	} else if ( lastBoneEntity.oldTorsoFrame != refent->oldTorsoFrame ) {
		return qfalse;
	} else if ( lastBoneEntity.torsoFrameModel != refent->torsoFrameModel ) {
		return qfalse;
	} else if ( lastBoneEntity.oldTorsoFrameModel != refent->oldTorsoFrameModel ) {
		return qfalse;
	} else if ( lastBoneEntity.torsoBacklerp != refent->torsoBacklerp ) {
		return qfalse;
/*
	} else if ( lastBoneEntity.reFlags != refent->reFlags ) {
		return qfalse;
*/
	} else if ( !VectorCompare( lastBoneEntity.torsoAxis[0], refent->torsoAxis[0] ) ||
				!VectorCompare( lastBoneEntity.torsoAxis[1], refent->torsoAxis[1] ) ||
				!VectorCompare( lastBoneEntity.torsoAxis[2], refent->torsoAxis[2] ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
==============
R_CalcBones

	The list of bones[] should only be built and modified from within here
==============
*/
static void R_CalcBones( const refEntity_t *refent, int *boneList, int numBones ) {

	int i;
	int     *boneRefs;
	float torsoWeight;
	mdsHeader_t *frameHeader, *oldFrameHeader;
	mdsHeader_t *torsoFrameHeader, *oldTorsoFrameHeader;
	mdsFrame_t *frame, *oldFrame;
	mdsFrame_t *torsoFrame, *oldTorsoFrame;

	frameHeader = R_GetFrameModelDataByHandle( refent, refent->frameModel );
	oldFrameHeader = R_GetFrameModelDataByHandle( refent, refent->oldframeModel );
	torsoFrameHeader = R_GetFrameModelDataByHandle( refent, refent->torsoFrameModel );
	oldTorsoFrameHeader = R_GetFrameModelDataByHandle( refent, refent->oldTorsoFrameModel );

	if ( !frameHeader || !oldFrameHeader || !torsoFrameHeader || !oldTorsoFrameHeader ) {
		return;
	}

	//
	// if the entity has changed since the last time the bones were built, reset them
	//
	if ( !R_BonesStillValid( refent ) ) {
		// different, cached bones are not valid
		memset( validBones, 0, frameHeader->numBones );
		lastBoneEntity = *refent;

#ifndef DEDICATED
		// (SA) also reset these counter statics
//----(SA)	print stats for the complete model (not per-surface)
		if ( r_bonesDebug->integer == 4 && totalrt ) {
			ri.Printf( PRINT_ALL, "Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n",
					   lodScale,
					   totalrv,
					   totalv,
					   totalrt,
					   totalt,
					   ( float )( 100.0 * totalrt ) / (float) totalt );
		}
//----(SA)	end
#endif
		totalrv = totalrt = totalv = totalt = 0;

	}

	memset( newBones, 0, frameHeader->numBones );

	if ( refent->oldframe == refent->frame ) {
		backlerp = 0;
		frontlerp = 1;
	} else  {
		backlerp = refent->backlerp;
		frontlerp = 1.0f - backlerp;
	}

	if ( refent->oldTorsoFrame == refent->torsoFrame ) {
		torsoBacklerp = 0;
		torsoFrontlerp = 1;
	} else {
		torsoBacklerp = refent->torsoBacklerp;
		torsoFrontlerp = 1.0f - torsoBacklerp;
	}

	frame = MDS_FRAME( frameHeader, refent->frame );
	torsoFrame = MDS_FRAME( torsoFrameHeader, refent->torsoFrame );
	oldFrame = MDS_FRAME( oldFrameHeader, refent->oldframe );
	oldTorsoFrame = MDS_FRAME( oldTorsoFrameHeader, refent->oldTorsoFrame );

	//
	// lerp all the needed bones (torsoParent is always the first bone in the list)
	//
	cBoneList = frame->bones;
	cBoneListTorso = torsoFrame->bones;

	boneInfo = ( mdsBoneInfo_t * )( (byte *)frameHeader + frameHeader->ofsBones );
	boneRefs = boneList;
	//
	Matrix3Transpose( refent->torsoAxis, torsoAxis );

#ifdef HIGH_PRECISION_BONES
	if ( qtrue ) {
#else
	if ( !backlerp && !torsoBacklerp ) {
#endif

		for ( i = 0; i < numBones; i++, boneRefs++ ) {

			if ( validBones[*boneRefs] ) {
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ( ( boneInfo[*boneRefs].parent >= 0 ) && ( !validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent] ) ) {
				R_CalcBone( frame, frameHeader->torsoParent, boneInfo[*boneRefs].parent );
			}

			R_CalcBone( frame, frameHeader->torsoParent, *boneRefs );

		}

	} else {    // interpolated

		cOldBoneList = oldFrame->bones;
		cOldBoneListTorso = oldTorsoFrame->bones;

		for ( i = 0; i < numBones; i++, boneRefs++ ) {

			if ( validBones[*boneRefs] ) {
				// this bone is still in the cache
				bones[*boneRefs] = rawBones[*boneRefs];
				continue;
			}

			// find our parent, and make sure it has been calculated
			if ( ( boneInfo[*boneRefs].parent >= 0 ) && ( !validBones[boneInfo[*boneRefs].parent] && !newBones[boneInfo[*boneRefs].parent] ) ) {
				R_CalcBoneLerp( frame, oldFrame, frameHeader->torsoParent, boneInfo[*boneRefs].parent );
			}

			R_CalcBoneLerp( frame, oldFrame, frameHeader->torsoParent, *boneRefs );

		}

	}

	// adjust for torso rotations
	torsoWeight = 0;
	boneRefs = boneList;
	for ( i = 0; i < numBones; i++, boneRefs++ ) {

		thisBoneInfo = &boneInfo[ *boneRefs ];
		bonePtr = &bones[ *boneRefs ];
		// add torso rotation
		if ( thisBoneInfo->torsoWeight > 0 ) {

			if ( !newBones[ *boneRefs ] ) {
				// just copy it back from the previous calc
				bones[ *boneRefs ] = oldBones[ *boneRefs ];
				continue;
			}

			if ( !( thisBoneInfo->flags & MDS_BONEFLAG_TAG ) ) {

				// 1st multiply with the bone->matrix
				// 2nd translation for rotation relative to bone around torso parent offset
				VectorSubtract( bonePtr->translation, torsoParentOffset, t );
				Matrix4FromAxisPlusTranslation( bonePtr->matrix, t, m1 );
				// 3rd scaled rotation
				// 4th translate back to torso parent offset
				// use previously created matrix if available for the same weight
				if ( torsoWeight != thisBoneInfo->torsoWeight ) {
					Matrix4FromScaledAxisPlusTranslation( torsoAxis, thisBoneInfo->torsoWeight, torsoParentOffset, m2 );
					torsoWeight = thisBoneInfo->torsoWeight;
				}
				// multiply matrices to create one matrix to do all calculations
				Matrix4MultiplyInto3x3AndTranslation( m2, m1, bonePtr->matrix, bonePtr->translation );

			} else {    // tag's require special handling
				vec3_t tmpAxis[3];

				// rotate each of the axis by the torsoAngles
				LocalScaledMatrixTransformVector( bonePtr->matrix[0], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[0] );
				LocalScaledMatrixTransformVector( bonePtr->matrix[1], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[1] );
				LocalScaledMatrixTransformVector( bonePtr->matrix[2], thisBoneInfo->torsoWeight, torsoAxis, tmpAxis[2] );
				memcpy( bonePtr->matrix, tmpAxis, sizeof( tmpAxis ) );

				// rotate the translation around the torsoParent
				VectorSubtract( bonePtr->translation, torsoParentOffset, t );
				LocalScaledMatrixTransformVector( t, thisBoneInfo->torsoWeight, torsoAxis, bonePtr->translation );
				VectorAdd( bonePtr->translation, torsoParentOffset, bonePtr->translation );

			}
		}
	}

	// backup the final bones
	memcpy( oldBones, bones, sizeof( bones[0] ) * frameHeader->numBones );
}

#ifdef DBG_PROFILE_BONES
#define DBG_SHOWTIME    Com_Printf( "%i: %i, ", di++, ( dt = ri.Milliseconds() ) - ldt ); ldt = dt;
#else
#define DBG_SHOWTIME    ;
#endif

#ifndef DEDICATED

/*
==============
RB_MDSSurfaceAnim
==============
*/
void RB_MDSSurfaceAnim( mdsSurface_t *surface ) {
	int i;
	int j, k;
	refEntity_t *refent;
	int *boneList;
	mdsHeader_t *header;
	int *triangles;
	glIndex_t *pIndexes;
	int indexes;
	int baseIndex, baseVertex, oldIndexes;
	mdsVertex_t *v;
	float *tempVert, *tempNormal;
	int *collapse_map, *pCollapseMap;
	int collapse[ MDS_MAX_VERTS ], *pCollapse;
	int p0, p1, p2;

#ifdef DBG_PROFILE_BONES
	int di = 0, dt, ldt;

	dt = ri.Milliseconds();
	ldt = dt;
#endif

	refent = &backEnd.currentEntity->e;
	boneList = ( int * )( (byte *)surface + surface->ofsBoneReferences );
	header = ( mdsHeader_t * )( (byte *)surface + surface->ofsHeader );

	if ( r_bonesDebug->integer == 9 ) {
		if ( surface == ( mdsSurface_t * )( (byte *)header + header->ofsSurfaces ) ) {
			int boneListAll[ MDS_MAX_BONES ];

			for ( i = 0; i < header->numBones; i++ ) {
				boneListAll[ i ] = i;
			}

			R_CalcBones( (const refEntity_t *)refent, boneListAll, header->numBones );;
		}
	} else {
		R_CalcBones( (const refEntity_t *)refent, boneList, surface->numBoneReferences );
	}

	DBG_SHOWTIME

	//
	// calculate LOD
	//
	lodScale = RB_CalcMDSLod( refent, header->lodBias, header->lodScale );

//DBG_SHOWTIME

//----(SA)	modification to allow dead skeletal bodies to go below minlod (experiment)
#if 0 // ZTM: FIXME?: Flag not supported yet
	if ( refent->reFlags & REFLAG_DEAD_LOD ) {
		if ( lodScale < 0.35 ) {   // allow dead to lod down to 35% (even if below surf->minLod) (%35 is arbitrary and probably not good generally.  worked for the blackguard/infantry as a test though)
			lodScale = 0.35;
		}
		render_count = (int)( (float) surface->numVerts * lodScale );

	} else {
#endif
		render_count = (int)( (float) surface->numVerts * lodScale );
		if ( render_count < surface->minLod ) {
			render_count = surface->minLod;
		}
#if 0
	}
#endif
//----(SA)	end

	// ydnar: to profile bone transform performance only
	if ( r_bonesDebug->integer == 10 ) {
		return;
	}

	if ( render_count > surface->numVerts ) {
		render_count = surface->numVerts;
	}

//DBG_SHOWTIME

	//
	// setup triangle list
	//
	RB_CHECKOVERFLOW( render_count, surface->numTriangles * 3 );

//DBG_SHOWTIME

	collapse_map   = ( int * )( ( byte * )surface + surface->ofsCollapseMap );
	triangles = ( int * )( (byte *)surface + surface->ofsTriangles );
	indexes = surface->numTriangles * 3;
	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	oldIndexes = baseIndex;

	tess.numVertexes += render_count;

	pIndexes = (glIndex_t *)&tess.indexes[baseIndex];

//DBG_SHOWTIME

	if ( render_count == surface->numVerts ) {
		for ( j = 0; j < indexes; j++ ) {
			pIndexes[j] = triangles[j] + baseVertex;
		}
		tess.numIndexes += indexes;
	} else
	{
		int *collapseEnd;

		pCollapse = collapse;
		for ( j = 0; j < render_count; pCollapse++, j++ )
		{
			*pCollapse = j;
		}

		pCollapseMap = &collapse_map[render_count];
		for ( collapseEnd = collapse + surface->numVerts ; pCollapse < collapseEnd; pCollapse++, pCollapseMap++ )
		{
			*pCollapse = collapse[ *pCollapseMap ];
		}

		for ( j = 0 ; j < indexes ; j += 3 )
		{
			p0 = collapse[ *( triangles++ ) ];
			p1 = collapse[ *( triangles++ ) ];
			p2 = collapse[ *( triangles++ ) ];

			// FIXME
			// note:  serious optimization opportunity here,
			//  by sorting the triangles the following "continue"
			//  could have been made into a "break" statement.
			if ( p0 == p1 || p1 == p2 || p2 == p0 ) {
				continue;
			}

			*( pIndexes++ ) = baseVertex + p0;
			*( pIndexes++ ) = baseVertex + p1;
			*( pIndexes++ ) = baseVertex + p2;
			tess.numIndexes += 3;
		}

		baseIndex = tess.numIndexes;
	}

//DBG_SHOWTIME

	//
	// deform the vertexes by the lerped bones
	//
	v = ( mdsVertex_t * )( (byte *)surface + surface->ofsVerts );
	tempVert = ( float * )( tess.xyz + baseVertex );
	tempNormal = ( float * )( tess.normal + baseVertex );
	for ( j = 0; j < render_count; j++, tempVert += 4, tempNormal += 4 ) {
		mdsWeight_t *w;

		VectorClear( tempVert );

		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) {
			mdsBoneFrame_t  *bone = &bones[w->boneIndex];
			LocalAddScaledMatrixTransformVectorTranslate( w->offset, w->boneWeight, bone->matrix, bone->translation, tempVert );
		}

		LocalMatrixTransformVector( v->normal, bones[v->weights[0].boneIndex].matrix, tempNormal );

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (mdsVertex_t *)&v->weights[v->numWeights];
	}

	DBG_SHOWTIME

	if ( r_bonesDebug->integer ) {
		GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
		if ( r_bonesDebug->integer < 3 || r_bonesDebug->integer == 5 || r_bonesDebug->integer == 8 || r_bonesDebug->integer == 9 ) {
			// DEBUG: show the bones as a stick figure with axis at each bone
			int *boneRefs = ( int * )( (byte *)surface + surface->ofsBoneReferences );
			for ( i = 0; i < surface->numBoneReferences; i++, boneRefs++ ) {
				bonePtr = &bones[*boneRefs];

				if ( r_bonesDebug->integer != 9 ) {
					GL_Bind( tr.whiteImage );
					qglLineWidth( 1 );
					qglBegin( GL_LINES );
					for ( j = 0; j < 3; j++ ) {
						VectorClear( vec );
						vec[j] = 1;
						qglColor3fv( vec );
						qglVertex3fv( bonePtr->translation );
						VectorMA( bonePtr->translation, ( r_bonesDebug->integer == 8 ? 1.5f : 5 ), bonePtr->matrix[j], vec );
						qglVertex3fv( vec );
					}
					qglEnd();
				}

				// connect to our parent if it's valid
				if ( validBones[boneInfo[*boneRefs].parent] ) {
					qglLineWidth( r_bonesDebug->integer == 8 ? 4 : 2 );
					qglBegin( GL_LINES );
					qglColor3f( .6,.6,.6 );
					qglVertex3fv( bonePtr->translation );
					qglVertex3fv( bones[boneInfo[*boneRefs].parent].translation );
					qglEnd();
				}

				qglLineWidth( 1 );
			}

			if ( r_bonesDebug->integer == 8 ) {
				// FIXME: Actually draw the whole skeleton
				//% if( surface == (mdsSurface_t *)((byte *)header + header->ofsSurfaces) ) {
				//mdsHeader_t *frameHeader = R_GetFrameModelDataByHandle( refent, refent->frameModel );
				boneRefs = ( int * )( (byte *)surface + surface->ofsBoneReferences );

				qglDepthRange( 0, 0 );      // never occluded
				qglBlendFunc( GL_SRC_ALPHA, GL_ONE );

				for ( i = 0; i < surface->numBoneReferences; i++, boneRefs++ ) {
					vec3_t diff;
					//mdsBoneInfo_t *mdsBoneInfo = ( mdsBoneInfo_t * )( (byte *)mdxHeader + mdxHeader->ofsBones + *boneRefs * sizeof( mdxBoneInfo_t ) );
					bonePtr = &bones[*boneRefs];

					VectorSet( vec, 0.f, 0.f, 32.f );
					VectorSubtract( bonePtr->translation, vec, diff );
					vec[0] = vec[0] + diff[0] * 6;
					vec[1] = vec[1] + diff[1] * 6;
					vec[2] = vec[2] + diff[2] * 3;

					qglEnable( GL_BLEND );
					qglBegin( GL_LINES );
					qglColor4f( 1.f, .4f, .05f, .35f );
					qglVertex3fv( bonePtr->translation );
					qglVertex3fv( vec );
					qglEnd();
					qglDisable( GL_BLEND );

					//R_DebugText( vec, 1.f, 1.f, 1.f, mdsBoneInfo->name, qfalse );       // qfalse, as there is no reason to set depthrange again
				}

				qglDepthRange( 0, 1 );
				//% }
			} else if ( r_bonesDebug->integer == 9 ) {
				if ( surface == ( mdsSurface_t * )( (byte *)header + header->ofsSurfaces ) ) {
					mdsTag_t *pTag = ( mdsTag_t * )( (byte *)header + header->ofsTags );

					qglDepthRange( 0, 0 );  // never occluded
					qglBlendFunc( GL_SRC_ALPHA, GL_ONE );

					for ( i = 0; i < header->numTags; i++ ) {
						mdsBoneFrame_t  *tagBone;
						orientation_t outTag;
						vec3_t diff;

						// now extract the orientation for the bone that represents our tag
						tagBone = &bones[pTag->boneIndex];
						memcpy( outTag.axis, tagBone->matrix, sizeof( outTag.axis ) );
						VectorCopy( tagBone->translation, outTag.origin );

						GL_Bind( tr.whiteImage );
						qglLineWidth( 2 );
						qglBegin( GL_LINES );
						for ( j = 0; j < 3; j++ ) {
							VectorClear( vec );
							vec[j] = 1;
							qglColor3fv( vec );
							qglVertex3fv( outTag.origin );
							VectorMA( outTag.origin, 5, outTag.axis[j], vec );
							qglVertex3fv( vec );
						}
						qglEnd();

						VectorSet( vec, 0.f, 0.f, 32.f );
						VectorSubtract( outTag.origin, vec, diff );
						vec[0] = vec[0] + diff[0] * 2;
						vec[1] = vec[1] + diff[1] * 2;
						vec[2] = vec[2] + diff[2] * 1.5;

						qglLineWidth( 1 );
						qglEnable( GL_BLEND );
						qglBegin( GL_LINES );
						qglColor4f( 1.f, .4f, .05f, .35f );
						qglVertex3fv( outTag.origin );
						qglVertex3fv( vec );
						qglEnd();
						qglDisable( GL_BLEND );

						//R_DebugText( vec, 1.f, 1.f, 1.f, pTag->name, qfalse );  // qfalse, as there is no reason to set depthrange again

						pTag++;
					}
					qglDepthRange( 0, 1 );
				}
			}
		}

		if ( r_bonesDebug->integer >= 3 && r_bonesDebug->integer <= 6 ) {
			int render_indexes = ( tess.numIndexes - oldIndexes );

			// show mesh edges
			tempVert = ( float * )( tess.xyz + baseVertex );
			tempNormal = ( float * )( tess.normal + baseVertex );

			GL_Bind( tr.whiteImage );
			qglLineWidth( 1 );
			qglBegin( GL_LINES );
			qglColor3f( .0,.0,.8 );

			pIndexes = (glIndex_t *)&tess.indexes[oldIndexes];
			for ( j = 0; j < render_indexes / 3; j++, pIndexes += 3 ) {
				qglVertex3fv( tempVert + 4 * pIndexes[0] );
				qglVertex3fv( tempVert + 4 * pIndexes[1] );

				qglVertex3fv( tempVert + 4 * pIndexes[1] );
				qglVertex3fv( tempVert + 4 * pIndexes[2] );

				qglVertex3fv( tempVert + 4 * pIndexes[2] );
				qglVertex3fv( tempVert + 4 * pIndexes[0] );
			}

			qglEnd();

//----(SA)	track debug stats
			if ( r_bonesDebug->integer == 4 ) {
				totalrv += render_count;
				totalrt += render_indexes / 3;
				totalv += surface->numVerts;
				totalt += surface->numTriangles;
			}
//----(SA)	end

			if ( r_bonesDebug->integer == 3 ) {
				ri.Printf( PRINT_ALL, "Lod %.2f  verts %4d/%4d  tris %4d/%4d  (%.2f%%)\n", lodScale, render_count, surface->numVerts, render_indexes / 3, surface->numTriangles,
						   ( float )( 100.0 * render_indexes / 3 ) / (float) surface->numTriangles );
			}
		}

		if ( r_bonesDebug->integer == 6 || r_bonesDebug->integer == 7 ) {
			v = ( mdsVertex_t * )( (byte *)surface + surface->ofsVerts );
			tempVert = ( float * )( tess.xyz + baseVertex );
			GL_Bind( tr.whiteImage );
			qglPointSize( 5 );
			qglBegin( GL_POINTS );
			for ( j = 0; j < render_count; j++, tempVert += 4 ) {
				if ( v->numWeights > 1 ) {
					if ( v->numWeights == 2 ) {
						qglColor3f( .4f, .4f, 0.f );
					} else if ( v->numWeights == 3 ) {
						qglColor3f( .8f, .4f, 0.f );
					} else {
						qglColor3f( 1.f, .4f, 0.f );
					}
					qglVertex3fv( tempVert );
				}
				v = (mdsVertex_t *)&v->weights[v->numWeights];
			}
			qglEnd();
		}
	}

	if ( r_bonesDebug->integer > 1 ) {
		// dont draw the actual surface
		tess.numIndexes = oldIndexes;
		tess.numVertexes = baseVertex;
		return;
	}

#ifdef DBG_PROFILE_BONES
	Com_Printf( "\n" );
#endif

}

#endif // !DEDICATED

/*
===============
R_RecursiveBoneListAdd
===============
*/
static void R_RecursiveBoneListAdd( int bi, int *boneList, int *numBones, mdsBoneInfo_t *boneInfoList ) {

	if ( boneInfoList[ bi ].parent >= 0 ) {

		R_RecursiveBoneListAdd( boneInfoList[ bi ].parent, boneList, numBones, boneInfoList );

	}

	boneList[ ( *numBones )++ ] = bi;

}

/*
===============
R_GetMDSBoneTag
===============
*/
int R_GetMDSBoneTag( orientation_t *outTag, const model_t *mod,
					 const char *tagName, int startTagIndex,
					 qhandle_t frameModel, int startFrame,
					 qhandle_t endFrameModel, int endFrame,
					 float frac, const vec3_t *torsoAxis,
					 qhandle_t torsoFrameModel, int torsoStartFrame,
					 qhandle_t torsoEndFrameModel, int torsoEndFrame,
					 float torsoFrac )
{
	int i;
	mdsHeader_t *header;
	mdsTag_t    *pTag;
	mdsBoneInfo_t *boneInfoList;
	int boneList[ MDS_MAX_BONES ];
	int numBones;
	refEntity_t refent;

	header = (mdsHeader_t *)mod->modelData;

	if ( startTagIndex >= header->numTags ) {
		return -1;
	}

	// find the correct tag

	pTag = ( mdsTag_t * )( (byte *)header + header->ofsTags );

	for ( i = 0; i < header->numTags; i++ ) {
		if ( i >= startTagIndex && !strcmp( pTag->name, tagName ) ) {
			break;
		}
		pTag++;
	}

	if ( i >= header->numTags ) {
		return -1;
	}

	// just checking if tag exists
	if ( !outTag ) {
		return i;
	}

	// now set up fake refent

	Com_Memset( &refent, 0, sizeof ( refent ) );
	refent.hModel = mod->index;
	refent.frameModel = endFrameModel;
	refent.oldframeModel = frameModel;
	refent.frame = endFrame;
	refent.oldframe = startFrame;
	refent.backlerp = 1.0f - frac;

	// use torso information if present
	// ZTM: FIXME: casted away const to silence warning
	if ( torsoAxis != NULL && !AxisEmpty( (vec3_t *)torsoAxis ) ) {
		AxisCopy( (vec3_t *)torsoAxis, refent.torsoAxis );

		refent.torsoFrameModel = torsoEndFrameModel;
		refent.oldTorsoFrameModel = torsoFrameModel;
		refent.torsoFrame = torsoEndFrame;
		refent.oldTorsoFrame = torsoStartFrame;
		refent.torsoBacklerp = 1.0f - torsoFrac;
	} else {
		// setup identify matrix
		AxisCopy( axisDefault, refent.torsoAxis );

		refent.torsoFrameModel = refent.frameModel;
		refent.oldTorsoFrameModel = refent.oldframeModel;
		refent.torsoFrame = refent.frame;
		refent.oldTorsoFrame = refent.oldframe;
		refent.torsoBacklerp = refent.backlerp;
	}

	// now build the list of bones we need to calc to get this tag's bone information

	boneInfoList = ( mdsBoneInfo_t * )( (byte *)header + header->ofsBones );
	numBones = 0;

	R_RecursiveBoneListAdd( pTag->boneIndex, boneList, &numBones, boneInfoList );

	// calc the bones

	R_CalcBones( &refent, boneList, numBones );

	// now extract the orientation for the bone that represents our tag

	memcpy( outTag->axis, bones[ pTag->boneIndex ].matrix, sizeof( outTag->axis ) );
	VectorCopy( bones[ pTag->boneIndex ].translation, outTag->origin );

	return i;
}

