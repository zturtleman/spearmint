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


#define MDS_FRAME( _header, _frame ) \
	( skaFrame_t * )( (byte *)_header + _header->ofsFrames + _frame * (size_t)( &((skaFrame_t *)0)->bones[ _header->numBones ] ) )

static skaHeader_t *R_GetFrameModelDataByHandle( const refEntity_t *ent, qhandle_t hModel ) {
	model_t *mod;

	if ( !hModel ) {
		hModel = ent->hModel;
	}

	mod = R_GetModelByHandle( hModel );

	if ( mod->type != MOD_SKA ) {
		return NULL;
	}

	return mod->modelData;
}

#ifndef DEDICATED
/*
=============
R_CullModel
=============
*/
static int R_CullModel( trRefEntity_t *ent ) {
	vec3_t bounds[2];
	skaHeader_t *oldHeader, *newHeader;
	skaFrame_t  *oldFrame, *newFrame;
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
R_ComputeFogNum

=================
*/
static int R_ComputeFogNum( trRefEntity_t *ent ) {
	skaHeader_t		*header;
	skaFrame_t		*frame;
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
R_SKBAddAnimSurfaces
==============
*/
void R_SKBAddAnimSurfaces( trRefEntity_t *ent ) {
	skbHeader_t     *header;
	skbSurface_t    *surface;
	skaHeader_t     *frameHeader, *oldFrameHeader;
	skaHeader_t     *torsoFrameHeader, *oldTorsoFrameHeader;
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

	header = (skbHeader_t *)tr.currentModel->modelData;

	frameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.frameModel );
	oldFrameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.oldframeModel );
	torsoFrameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.torsoFrameModel );
	oldTorsoFrameHeader = R_GetFrameModelDataByHandle( &ent->e, ent->e.oldTorsoFrameModel );

	if ( !frameHeader || !oldFrameHeader || !torsoFrameHeader || !oldTorsoFrameHeader ) {
		ri.Printf( PRINT_WARNING, "WARNING: Cannot render SKB '%s' without frameModel\n", tr.currentModel->name );
		return;
	}

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
			ri.Printf( PRINT_DEVELOPER, "R_SKBAddAnimSurfaces: no such frame %d to %d for '%s'\n",
				ent->e.oldframe, ent->e.frame,
				tr.currentModel->name );
			ent->e.frame = 0;
			ent->e.oldframe = 0;
	}

	if ( (ent->e.torsoFrame >= torsoFrameHeader->numFrames)
		|| (ent->e.torsoFrame < 0)
		|| (ent->e.oldTorsoFrame >= oldTorsoFrameHeader->numFrames)
		|| (ent->e.oldTorsoFrame < 0) ) {
			ri.Printf( PRINT_DEVELOPER, "R_SKBAddAnimSurfaces: no such torso frame %d to %d for '%s'\n",
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

	surface = ( skbSurface_t * )( (byte *)header + header->ofsSurfaces );
	for ( i = 0 ; i < header->numSurfaces ; i++ ) {

		if ( ent->e.customShader || ent->e.customSkin ) {
			shader = R_CustomSurfaceShader( surface->name, ent->e.customShader, ent->e.customSkin );
			if (shader == tr.nodrawShader) {
				surface = ( skbSurface_t * )( (byte *)surface + surface->ofsEnd );
				continue;
			}
		}
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

		surface = ( skbSurface_t * )( (byte *)surface + surface->ofsEnd );
	}
}
#endif

// FIXME: Copied math functions from tr_model_iqm.c
// "multiply" 3x4 matrices, these are assumed to be the top 3 rows
// of a 4x4 matrix with the last row = (0 0 0 1)
static void Matrix34Multiply( const float *a, const float *b, float *out ) {
	out[ 0] = a[0] * b[0] + a[1] * b[4] + a[ 2] * b[ 8];
	out[ 1] = a[0] * b[1] + a[1] * b[5] + a[ 2] * b[ 9];
	out[ 2] = a[0] * b[2] + a[1] * b[6] + a[ 2] * b[10];
	out[ 3] = a[0] * b[3] + a[1] * b[7] + a[ 2] * b[11] + a[ 3];
	out[ 4] = a[4] * b[0] + a[5] * b[4] + a[ 6] * b[ 8];
	out[ 5] = a[4] * b[1] + a[5] * b[5] + a[ 6] * b[ 9];
	out[ 6] = a[4] * b[2] + a[5] * b[6] + a[ 6] * b[10];
	out[ 7] = a[4] * b[3] + a[5] * b[7] + a[ 6] * b[11] + a[ 7];
	out[ 8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[ 8];
	out[ 9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[ 9];
	out[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10];
	out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11];
}

static void Matrix34Invert( const float *inMat, float *outMat ) {
	vec3_t trans;
	float invSqrLen, *v;

	outMat[ 0] = inMat[ 0]; outMat[ 1] = inMat[ 4]; outMat[ 2] = inMat[ 8];
	outMat[ 4] = inMat[ 1]; outMat[ 5] = inMat[ 5]; outMat[ 6] = inMat[ 9];
	outMat[ 8] = inMat[ 2]; outMat[ 9] = inMat[ 6]; outMat[10] = inMat[10];

	v = outMat + 0; invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	v = outMat + 4; invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	v = outMat + 8; invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);

	trans[0] = inMat[ 3];
	trans[1] = inMat[ 7];
	trans[2] = inMat[11];

	outMat[ 3] = -DotProduct(outMat + 0, trans);
	outMat[ 7] = -DotProduct(outMat + 4, trans);
	outMat[11] = -DotProduct(outMat + 8, trans);
}

/*
==============
R_CalcBones
==============
*/
static void R_CalcBones( const refEntity_t *refent, skaBone_t *bones ) {
	int i, j;
	skbHeader_t *header;
	skaHeader_t *frameHeader, *oldFrameHeader;
	skaHeader_t *torsoFrameHeader, *oldTorsoFrameHeader;
	skaFrame_t *frame, *oldFrame;
	skaFrame_t *torsoFrame, *oldTorsoFrame;

	float backlerp, frontlerp;
	float torsoBacklerp, torsoFrontlerp;
	skaBone_t *bonePtr;
	skbBoneInfo_t *boneInfo;

	header = (skbHeader_t *)R_GetModelByHandle( refent->hModel )->modelData;
	frameHeader = R_GetFrameModelDataByHandle( refent, refent->frameModel );
	oldFrameHeader = R_GetFrameModelDataByHandle( refent, refent->oldframeModel );
	torsoFrameHeader = R_GetFrameModelDataByHandle( refent, refent->torsoFrameModel );
	oldTorsoFrameHeader = R_GetFrameModelDataByHandle( refent, refent->oldTorsoFrameModel );

	if ( !frameHeader || !oldFrameHeader || !torsoFrameHeader || !oldTorsoFrameHeader ) {
		return;
	}

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

	// TODO: Apply torsoAxis

#if 1
	//
	// lerp all the needed bones
	//
	boneInfo = (skbBoneInfo_t*)( (byte*)header + header->ofsBones );
	if ( !backlerp )
	{
		// no lerping needed
		bonePtr = bones;
		
		for ( i = 0 ; i < header->numBones; i++, bonePtr++ )
		{
			if ( boneInfo[i].flags & SKB_BONEFLAG_LEG ) {
				memcpy( bonePtr->matrix, frame->bones[i].matrix, sizeof( frame->bones[i].matrix ) );
			} else {
				memcpy( bonePtr->matrix, torsoFrame->bones[i].matrix, sizeof( torsoFrame->bones[i].matrix ) );
			}
		}
	}
	else
	{
		bonePtr = bones;
		
		for ( i = 0 ; i < header->numBones; i++, bonePtr++ )
		{
			for ( j = 0; j < 12; j++ ) {
				if ( boneInfo[i].flags & SKB_BONEFLAG_LEG ) {
					((float *)bonePtr->matrix)[j] = frontlerp * ((float *)frame->bones[i].matrix)[j] + backlerp * ((float *)oldFrame->bones[i].matrix)[j];
				} else {
					((float *)bonePtr->matrix)[j] = torsoFrontlerp * ((float *)torsoFrame->bones[i].matrix)[j] + torsoBacklerp * ((float *)oldTorsoFrame->bones[i].matrix)[j];
				}
			}
		}
	}

	for ( i = 0 ; i < header->numBones ; i++ )
	{
		if ( boneInfo[i].parent != -1 )
		{
			float tmpMatrix[3][4];
			Matrix34Multiply( (float*)bones[boneInfo[i].parent].matrix, (float*)bones[i].matrix, (float*)tmpMatrix );
			memcpy( bones[i].matrix, tmpMatrix, sizeof( tmpMatrix ) );
		}
	}

#else

	//
	// lerp all the needed bones (torsoParent is always the first bone in the list)
	//
	cBoneList = frame->bones;
	cBoneListTorso = torsoFrame->bones;

	boneInfo = ( mdxBoneInfo_t * )( (byte *)frameHeader + frameHeader->ofsBones );
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

#if 0
			if ( !( thisBoneInfo->flags & MDX_BONEFLAG_TAG ) ) {
#endif

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

#if 0
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
#endif
		}
	}

	// backup the final bones
	memcpy( oldBones, bones, sizeof( bones[0] ) * frameHeader->numBones );

#endif
}

#ifndef DEDICATED
/*
==============
RB_SKBSurfaceAnim
==============
*/
void RB_SKBSurfaceAnim( skbSurface_t *surface ) {
	int i;
	int j, k;
	refEntity_t *refent;
	int *boneList;
	skbHeader_t *header;
	int *triangles;
	glIndex_t *pIndexes;
	int indexes;
	int baseIndex, baseVertex, oldIndexes;
	skbVertex_t *v;
	float *tempVert, *tempNormal;
	int *collapse_map, *pCollapseMap;
	int collapse[ SKB_MAX_VERTS ], *pCollapse;
	int p0, p1, p2;
	float lodScale;
	int render_count;
	skaHeader_t *frameHeader, *oldFrameHeader;
	skaFrame_t *frame, *oldFrame;
	skaBone_t bones[SKA_MAX_BONES], *bonePtr, *bone;
	skbBoneInfo_t *boneInfo;
	float			frontlerp, backlerp;

	refent = &backEnd.currentEntity->e;

	header = (skbHeader_t *)R_GetModelByHandle( backEnd.currentEntity->e.hModel )->modelData;

	frameHeader = R_GetFrameModelDataByHandle( &backEnd.currentEntity->e, backEnd.currentEntity->e.frameModel );
	oldFrameHeader = R_GetFrameModelDataByHandle( &backEnd.currentEntity->e, backEnd.currentEntity->e.oldframeModel );

	if ( !frameHeader || !oldFrameHeader ) {
		return;
	}

	// compute frame pointers
	frame = MDS_FRAME( frameHeader, backEnd.currentEntity->e.frame );
	oldFrame = MDS_FRAME( oldFrameHeader, backEnd.currentEntity->e.oldframe );


	// don't lerp if lerping off, or this is the only frame, or the last frame...
	//
	if (backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame) 
	{
		backlerp	= 0;	// if backlerp is 0, lerping is off and frontlerp is never used
		frontlerp	= 1;
	} 
	else  
	{
		backlerp	= backEnd.currentEntity->e.backlerp;
		frontlerp	= 1.0f - backlerp;
	}


	//
	// TODO: calculate LOD
	//
	lodScale = 1.0f;

	// This is just my LOD test. --zturtleman
	if ( r_lodbias->integer == 1 ) {
		lodScale = ( surface->minLod + 0.5f * (surface->numVerts-surface->minLod) ) / (float)surface->numVerts;
	} else if ( r_lodbias->integer == 2 ) {
		lodScale = ( surface->minLod + 0.25f * (surface->numVerts-surface->minLod) ) / (float)surface->numVerts;
	} else if ( r_lodbias->integer == 3 ) {
		lodScale = surface->minLod / (float)surface->numVerts;
	}

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

	if ( render_count > surface->numVerts ) {
		render_count = surface->numVerts;
	}

	//
	// setup triangle list
	//
	RB_CHECKOVERFLOW( render_count, surface->numTriangles * 3 );

	collapse_map   = ( int * )( ( byte * )surface + surface->ofsCollapseMap );
	triangles = ( int * )( (byte *)surface + surface->ofsTriangles );
	indexes = surface->numTriangles * 3;
	baseIndex = tess.numIndexes;
	baseVertex = tess.numVertexes;
	oldIndexes = baseIndex;

	tess.numVertexes += render_count;

	pIndexes = (glIndex_t *)&tess.indexes[baseIndex];

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

	R_CalcBones( &backEnd.currentEntity->e, bones );

	if ( r_bonesDebug->integer ) {
		vec3_t pos1, pos2;

		GL_Bind( tr.whiteImage );
		GL_State( GLS_DEFAULT );
		qglDepthRange( 0, 0 );
		qglBegin( GL_LINES );
		for ( i = 0; i < header->numBones; i++ ) {
			pos1[0] = bones[i].matrix[0][3];
			pos1[1] = bones[i].matrix[1][3];
			pos1[2] = bones[i].matrix[2][3];

			// draw rotation axis
			qglColor3f( 1.0f, 0.0f, 0.0f );
			qglVertex3fv( pos1 );
			// FIXME: this 5.0f may not match Alice
			pos2[0] = pos1[0] + bones[i].matrix[0][0] * 5.0f;
			pos2[1] = pos1[1] + bones[i].matrix[1][0] * 5.0f;
			pos2[2] = pos1[2] + bones[i].matrix[2][0] * 5.0f;
			qglVertex3fv( pos2 );

			qglColor3f( 0.0f, 1.0f, 0.0f );
			qglVertex3fv( pos1 );
			pos2[0] = pos1[0] + bones[i].matrix[0][1] * 5.0f;
			pos2[1] = pos1[1] + bones[i].matrix[1][1] * 5.0f;
			pos2[2] = pos1[2] + bones[i].matrix[2][1] * 5.0f;
			qglVertex3fv( pos2 );

			qglColor3f( 0.0f, 0.0f, 1.0f );
			qglVertex3fv( pos1 );
			pos2[0] = pos1[0] + bones[i].matrix[0][2] * 5.0f;
			pos2[1] = pos1[1] + bones[i].matrix[1][2] * 5.0f;
			pos2[2] = pos1[2] + bones[i].matrix[2][2] * 5.0f;
			qglVertex3fv( pos2 );

			// parent line
			if ( boneInfo[i].parent == -1 ) {
				continue;
			}
			qglColor3f( 0.0f, 1.0f, 1.0f );
			qglVertex3fv( pos1 );
			pos2[0] = bones[boneInfo[i].parent].matrix[0][3];
			pos2[1] = bones[boneInfo[i].parent].matrix[1][3];
			pos2[2] = bones[boneInfo[i].parent].matrix[2][3];
			qglVertex3fv( pos2 );
		}
		qglEnd();
		qglDepthRange( 0, 1 );
	}

	//
	// deform the vertexes by the lerped bones
	//
	v = ( skbVertex_t * )( (byte *)surface + surface->ofsVerts );
	for ( j = 0; j < render_count; j++ ) {
		vec3_t	tempVert, tempNormal;
		skbWeight_t *w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) {
			bone = bones + w->boneIndex;

			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );
			
			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (skbVertex_t *)&v->weights[v->numWeights];
	}

}

#endif // !DEDICATED

/*
===============
R_GetSKBBoneTag
===============
*/
int R_GetSKBBoneTag( orientation_t *outTag, const model_t *mod,
					 const char *tagName, int startTagIndex,
					 qhandle_t frameModel, int startFrame,
					 qhandle_t endFrameModel, int endFrame,
					 float frac, const vec3_t *torsoAxis,
					 qhandle_t torsoFrameModel, int torsoStartFrame,
					 qhandle_t torsoEndFrameModel, int torsoEndFrame,
					 float torsoFrac )
{
	int i;
	skbHeader_t *header;
	skbBoneInfo_t *pTag;
	skaBone_t bones[SKA_MAX_BONES], *bone;
	int *boneList;
	refEntity_t refent;

	header = (skbHeader_t *)mod->modelData;

	if ( startTagIndex >= header->numBones ) {
		return -1;
	}

	if ( !frameModel || !endFrameModel || !torsoFrameModel || !torsoEndFrameModel ) {
		ri.Printf( PRINT_WARNING, "WARNING: Cannot get SKB tag '%s' from '%s' without frameModel\n", tagName, mod->name );
		return -1;
	}

	// find the correct tag

	pTag = ( skbBoneInfo_t * )( (byte *)header + header->ofsBones );

	for ( i = 0; i < header->numBones; i++, pTag++ ) {
		if ( i >= startTagIndex && !strcmp( pTag->name, tagName ) ) {
			break;
		}
	}

	if ( i >= header->numBones ) {
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

	// calc the bones

	R_CalcBones( &refent, bones );

	// now extract the orientation for the bone that represents our tag

#if 1
	// FIXME: Setup tag data.
	bone = &bones[i];
	VectorClear( outTag->origin );
	AxisClear( outTag->axis );
#else
	bone = &bones[pTag->boneIndex];
	VectorClear( outTag->origin );
	LocalAddScaledMatrixTransformVectorTranslate( pTag->offset, 1.f, bone->matrix, bone->translation, outTag->origin );
	for ( i = 0; i < 3; i++ ) {
		LocalMatrixTransformVector( pTag->axis[i], bone->matrix, outTag->axis[i] );
	}
#endif

	return i;
}

