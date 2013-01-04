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
================
R_CullSurface

Tries to back face cull surfaces before they are lighted or
added to the sorting list.

This will also allow mirrors on both sides of a model without recursion.
================
*/
static qboolean R_CullSurface( surfaceType_t *surface, shader_t *shader/*, int *frontFace*/ ) {
	srfGeneric_t    *gen;
	int cull;
	float d;


	// force to non-front facing
	//*frontFace = 0;

	// allow culling to be disabled
	if ( r_nocull->integer ) {
		return qfalse;
	}

	// ydnar: made surface culling generic, inline with q3map2 surface classification
	switch ( *surface )
	{
	case SF_FACE:
	case SF_TRIANGLES:
		break;
	case SF_GRID:
		if ( r_nocurves->integer ) {
			return qtrue;
		}
		break;
	case SF_FOLIAGE:
		if ( !r_drawfoliage->value ) {
			return qtrue;
		}
		break;

	case SF_FLARE:
	default:
		return qfalse;
	}

	// get generic surface
	gen = (srfGeneric_t*) surface;

	// plane cull
	if ( gen->plane.type != PLANE_NON_PLANAR && r_facePlaneCull->integer ) {
		d = DotProduct( tr.or.viewOrigin, gen->plane.normal ) - gen->plane.dist;
		//if ( d > 0.0f ) {
		//	*frontFace = 1;
		//}

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if ( shader->cullType == CT_FRONT_SIDED ) {
			if ( d < -8.0f ) {
				//tr.pc.c_plane_cull_out++;
				return qtrue;
			}
		} else if ( shader->cullType == CT_BACK_SIDED )    {
			if ( d > 8.0f ) {
				//tr.pc.c_plane_cull_out++;
				return qtrue;
			}
		}

		//tr.pc.c_plane_cull_in++;
	}

	{
		// try sphere cull
		if ( tr.currentEntityNum != ENTITYNUM_WORLD ) {
			cull = R_CullLocalPointAndRadius( gen->origin, gen->radius );
		} else {
			cull = R_CullPointAndRadius( gen->origin, gen->radius );
		}
		if ( cull == CULL_OUT ) {
			//tr.pc.c_sphere_cull_out++;
			return qtrue;
		}

		//tr.pc.c_sphere_cull_in++;
	}

	// must be visible
	return qfalse;
}

static int R_DlightSurface( msurface_t *surface, int dlightBits ) {
	int i;
	vec3_t origin;
	float radius;
	srfGeneric_t    *gen;


	// get generic surface
	gen = (srfGeneric_t*) surface->data;

	// ydnar: made surface dlighting generic, inline with q3map2 surface classification
	switch ( (surfaceType_t) *surface->data )
	{
	case SF_FACE:
	case SF_TRIANGLES:
	case SF_GRID:
	case SF_FOLIAGE:
		break;

	default:
		gen->dlightBits[ tr.smpFrame ] = 0;
		return 0;
	}

	// debug code
	//%	gen->dlightBits[ tr.smpFrame ] = dlightBits;
	//%	return dlightBits;

	// try to cull out dlights
	for ( i = 0; i < tr.refdef.num_dlights; i++ )
	{
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}

#if 0
		// junior dlights don't affect world surfaces
		if ( tr.refdef.dlights[ i ].flags & REF_JUNIOR_DLIGHT ) {
			dlightBits &= ~( 1 << i );
			continue;
		}

		// lightning dlights affect all surfaces
		if ( tr.refdef.dlights[ i ].flags & REF_DIRECTED_DLIGHT ) {
			continue;
		}
#endif

		// test surface bounding sphere against dlight bounding sphere
		VectorCopy( tr.refdef.dlights[ i ].transformed, origin );
		radius = tr.refdef.dlights[ i ].radius;

		if ( ( gen->origin[ 0 ] + gen->radius ) < ( origin[ 0 ] - radius ) ||
			 ( gen->origin[ 0 ] - gen->radius ) > ( origin[ 0 ] + radius ) ||
			 ( gen->origin[ 1 ] + gen->radius ) < ( origin[ 1 ] - radius ) ||
			 ( gen->origin[ 1 ] - gen->radius ) > ( origin[ 1 ] + radius ) ||
			 ( gen->origin[ 2 ] + gen->radius ) < ( origin[ 2 ] - radius ) ||
			 ( gen->origin[ 2 ] - gen->radius ) > ( origin[ 2 ] + radius ) ) {
			dlightBits &= ~( 1 << i );
		}
	}

	// Com_Printf( "Surf: 0x%08X dlightBits: 0x%08X\n", srf, dlightBits );

	// set counters
	if ( dlightBits == 0 ) {
		tr.pc.c_dlightSurfacesCulled++;
	} else {
		tr.pc.c_dlightSurfaces++;
	}

	// set surface dlight bits and return
	gen->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}



/*
======================
R_AddWorldSurface
======================
*/
static void R_AddWorldSurface( msurface_t *surf, shader_t *shader, int fogNum, int dlightBits ) {
	if ( surf->viewCount == tr.viewCount ) {
		return;		// already in this view
	}

	surf->viewCount = tr.viewCount;
	surf->fogIndex = fogNum;

	// try to cull before dlighting or adding
	if ( R_CullSurface( surf->data, shader ) ) {
		return;
	}

	// check for dlighting
	if ( dlightBits ) {
		dlightBits = R_DlightSurface( surf, dlightBits );
		dlightBits = ( dlightBits != 0 );
	}

	R_AddDrawSurf( surf->data, shader, surf->fogIndex, dlightBits );
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
=================
R_BmodelFogNum

See if a sprite is inside a fog volume
Return positive with /any part/ of the brush falling within a fog volume
=================
*/
int R_BmodelFogNum( trRefEntity_t *re, bmodel_t *bmodel ) {
	int i, j;
	fog_t *fog;

	for ( i = 1; i < tr.world->numfogs; i++ )
	{
		fog = &tr.world->fogs[ i ];
		for ( j = 0; j < 3; j++ )
		{
			if ( re->e.origin[ j ] + bmodel->bounds[ 0 ][ j ] >= fog->bounds[ 1 ][ j ] ) {
				break;
			}
			if ( re->e.origin[ j ] + bmodel->bounds[ 1 ][ j ] <= fog->bounds[ 0 ][ j ] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return R_DefaultFogNum();
}

/*
=================
R_AddBrushModelSurfaces
=================
*/
void R_AddBrushModelSurfaces ( trRefEntity_t *ent ) {
	bmodel_t	*bmodel;
	int			clip;
	model_t		*pModel;
	int			i;
	int			fognum;
	msurface_t	*surf;

	pModel = R_GetModelByHandle( ent->e.hModel );

	bmodel = pModel->bmodel;

	clip = R_CullLocalBox( bmodel->bounds );
	if ( clip == CULL_OUT ) {
		return;
	}

	// set current brush model to world
	tr.currentBModel = bmodel;

	// set model state for decals and dynamic fog
	VectorCopy( ent->e.origin, bmodel->orientation[ tr.smpFrame ].origin );
	VectorCopy( ent->e.axis[ 0 ], bmodel->orientation[ tr.smpFrame ].axis[ 0 ] );
	VectorCopy( ent->e.axis[ 1 ], bmodel->orientation[ tr.smpFrame ].axis[ 1 ] );
	VectorCopy( ent->e.axis[ 2 ], bmodel->orientation[ tr.smpFrame ].axis[ 2 ] );
	bmodel->visible[ tr.smpFrame ] = qtrue;
	bmodel->entityNum[ tr.smpFrame ] = tr.currentEntityNum;

	R_SetupEntityLighting( &tr.refdef, ent );
	R_DlightBmodel( bmodel );

	// determine if in fog
	fognum = R_BmodelFogNum( ent, bmodel );

	// add model surfaces
	for ( i = 0; i < bmodel->numSurfaces; i++ ) {
		surf = ( msurface_t * )( bmodel->firstSurface + i );

		// custom shader support for brushmodels
		if ( ent->e.customShader ) {
			R_AddWorldSurface( surf, R_GetShaderByHandle( ent->e.customShader ), fognum, tr.currentEntity->needDlights );
		} else {
			R_AddWorldSurface( surf, surf->shader, fognum, tr.currentEntity->needDlights );
		}
	}

	// clear current brush model
	tr.currentBModel = NULL;
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
=================
R_LeafFogNum

See if leaf is inside a fog volume
Return positive with /any part/ of the leaf falling within a fog volume
=================
*/
int R_LeafFogNum( mnode_t *node ) {
	int i, j;
	fog_t *fog;

	for ( i = 1; i < tr.world->numfogs; i++ )
	{
		fog = &tr.world->fogs[ i ];
		for ( j = 0; j < 3; j++ )
		{
			if ( node->mins[ j ] >= fog->bounds[ 1 ][ j ] ) {
				break;
			}
			if ( node->maxs[ j ] <= fog->bounds[ 0 ][ j ] ) {
				break;
			}
		}
		if ( j == 3 ) {
			return i;
		}
	}

	return R_DefaultFogNum();
}

/*
================
R_AddLeafSurfaces

Adds a leaf's drawsurfaces
================
*/
static void R_AddLeafSurfaces( mnode_t *node, int dlightBits ) {
	int c, fogNum;
	msurface_t  *surf, **mark;

	// add to count
	tr.pc.c_leafs++;

	// add to z buffer bounds
	if ( node->mins[0] < tr.viewParms.visBounds[0][0] ) {
		tr.viewParms.visBounds[0][0] = node->mins[0];
	}
	if ( node->mins[1] < tr.viewParms.visBounds[0][1] ) {
		tr.viewParms.visBounds[0][1] = node->mins[1];
	}
	if ( node->mins[2] < tr.viewParms.visBounds[0][2] ) {
		tr.viewParms.visBounds[0][2] = node->mins[2];
	}

	if ( node->maxs[0] > tr.viewParms.visBounds[1][0] ) {
		tr.viewParms.visBounds[1][0] = node->maxs[0];
	}
	if ( node->maxs[1] > tr.viewParms.visBounds[1][1] ) {
		tr.viewParms.visBounds[1][1] = node->maxs[1];
	}
	if ( node->maxs[2] > tr.viewParms.visBounds[1][2] ) {
		tr.viewParms.visBounds[1][2] = node->maxs[2];
	}

	fogNum = R_LeafFogNum( node );

	// add the individual surfaces
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while ( c-- ) {
		// the surface may have already been added if it
		// spans multiple leafs
		surf = *mark;
		R_AddWorldSurface( surf, surf->shader, fogNum, dlightBits );
		mark++;
	}
}


/*
================
R_RecursiveWorldNode
================
*/
static void R_RecursiveWorldNode( mnode_t *node, int planeBits, int dlightBits ) {

	do {
		int			newDlights[2];

		// if the node wasn't marked as potentially visible, exit
		if (node->visframe != tr.visCount) {
			return;
		}

		// if the bounding volume is outside the frustum, nothing
		// inside can be visible OPTIMIZE: don't do this all the way to leafs?

		if ( !r_nocull->integer ) {
			int		r;

			if ( planeBits & 1 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~1;			// all descendants will also be in front
				}
			}

			if ( planeBits & 2 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~2;			// all descendants will also be in front
				}
			}

			if ( planeBits & 4 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~4;			// all descendants will also be in front
				}
			}

			if ( planeBits & 8 ) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2) {
					return;						// culled
				}
				if ( r == 1 ) {
					planeBits &= ~8;			// all descendants will also be in front
				}
			}

			// farplane culling
			if ( planeBits & 16 ) {
				r = BoxOnPlaneSide( node->mins, node->maxs, &tr.viewParms.frustum[4] );
				if ( r == 2 ) {
					return;                     // culled
				}
				if ( r == 1 ) {
					planeBits &= ~16;            // all descendants will also be in front
				}
			}

		}

		if ( node->contents != -1 ) {
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative

		// determine which dlights are needed
		newDlights[0] = 0;
		newDlights[1] = 0;
		if ( dlightBits ) {
			int	i;

			for ( i = 0 ; i < tr.refdef.num_dlights ; i++ ) {
				dlight_t	*dl;
				float		dist;

				if ( dlightBits & ( 1 << i ) ) {
					dl = &tr.refdef.dlights[i];
					dist = DotProduct( dl->origin, node->plane->normal ) - node->plane->dist;
					
					if ( dist > -dl->radius ) {
						newDlights[0] |= ( 1 << i );
					}
					if ( dist < dl->radius ) {
						newDlights[1] |= ( 1 << i );
					}
				}
			}
		}

		// recurse down the children, front side first
		R_RecursiveWorldNode (node->children[0], planeBits, newDlights[0] );

		// tail recurse
		node = node->children[1];
		dlightBits = newDlights[1];
	} while ( 1 );

	// short circuit
	if ( node->nummarksurfaces == 0 ) {
		return;
	}

	R_AddLeafSurfaces( node, dlightBits );
}


/*
===============
R_PointInLeaf
===============
*/
static mnode_t *R_PointInLeaf( const vec3_t p ) {
	mnode_t		*node;
	float		d;
	cplane_t	*plane;
	
	if ( !tr.world ) {
		ri.Error (ERR_DROP, "R_PointInLeaf: bad model");
	}

	node = tr.world->nodes;
	while( 1 ) {
		if (node->contents != -1) {
			break;
		}
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0) {
			node = node->children[0];
		} else {
			node = node->children[1];
		}
	}
	
	return node;
}

/*
==============
R_ClusterPVS
==============
*/
static const byte *R_ClusterPVS (int cluster) {
	if (!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters ) {
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
=================
R_inPVS
=================
*/
qboolean R_inPVS( const vec3_t p1, const vec3_t p2 ) {
	mnode_t *leaf;
	byte	*vis;

	leaf = R_PointInLeaf( p1 );
	vis = ri.CM_ClusterPVS( leaf->cluster ); // why not R_ClusterPVS ??
	leaf = R_PointInLeaf( p2 );

	if ( !(vis[leaf->cluster>>3] & (1<<(leaf->cluster&7))) ) {
		return qfalse;
	}
	return qtrue;
}

/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
static void R_MarkLeaves (void) {
	const byte	*vis;
	mnode_t	*leaf, *parent;
	int		i;
	int		cluster;

	// lockpvs lets designers walk around to determine the
	// extent of the current pvs
	if ( r_lockpvs->integer ) {
		return;
	}

	// current viewcluster
	leaf = R_PointInLeaf( tr.viewParms.pvsOrigin );
	cluster = leaf->cluster;

	// if the cluster is the same and the area visibility matrix
	// hasn't changed, we don't need to mark everything again

	// if r_showcluster was just turned on, remark everything 
	if ( tr.viewCluster == cluster && !tr.refdef.areamaskModified 
		&& !r_showcluster->modified ) {
		return;
	}

	if ( r_showcluster->modified || r_showcluster->integer ) {
		r_showcluster->modified = qfalse;
		if ( r_showcluster->integer ) {
			ri.Printf( PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area );
		}
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if ( r_novis->integer || tr.viewCluster == -1 ) {
		for (i=0 ; i<tr.world->numnodes ; i++) {
			if (tr.world->nodes[i].contents != CONTENTS_SOLID) {
				tr.world->nodes[i].visframe = tr.visCount;
			}
		}
		return;
	}

	vis = R_ClusterPVS (tr.viewCluster);
	
	for (i=0,leaf=tr.world->nodes ; i<tr.world->numnodes ; i++, leaf++) {
		cluster = leaf->cluster;
		if ( cluster < 0 || cluster >= tr.world->numClusters ) {
			continue;
		}

		// check general pvs
		if ( !(vis[cluster>>3] & (1<<(cluster&7))) ) {
			continue;
		}

		// check for door connection
		if ( (tr.refdef.areamask[leaf->area>>3] & (1<<(leaf->area&7)) ) ) {
			continue;		// not visible
		}

		parent = leaf;
		do {
			if (parent->visframe == tr.visCount)
				break;
			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while (parent);
	}
}


/*
=============
R_AddWorldSurfaces
=============
*/
void R_AddWorldSurfaces (void) {
	if ( !r_drawworld->integer ) {
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL ) {
		return;
	}

	tr.currentEntityNum = REFENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_REFENTITYNUM_SHIFT;

	// set current brush model to world
	tr.currentBModel = &tr.world->bmodels[ 0 ];

	// determine which leaves are in the PVS / areamask
	R_MarkLeaves ();

	// clear out the visible min/max
	ClearBounds( tr.viewParms.visBounds[0], tr.viewParms.visBounds[1] );

	// perform frustum culling and add all the potentially visible surfaces
	if ( tr.refdef.num_dlights > 32 ) {
		tr.refdef.num_dlights = 32 ;
	}
	R_RecursiveWorldNode( tr.world->nodes, 255, ( 1 << tr.refdef.num_dlights ) - 1 );

	// clear brush model
	tr.currentBModel = NULL;
}
