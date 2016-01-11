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

#include "l_cmd.h"
#include "l_mem.h"
#include "l_log.h"
#include "l_poly.h"
#include "../botlib/l_script.h"
#include "l_qfiles.h"
#include "l_bsp_q3.h"
#include "l_bsp_ent.h"

void Q3_ParseEntities (void);

void GetLeafNums (void);

//=============================================================================

#define WCONVEX_EPSILON		0.5

bspFile_t		*q3bsp = NULL;

char			q3_dbrushsidetextured[Q3_MAX_MAP_BRUSHSIDES];

extern qboolean forcesidesvisible;

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Q3_FreeMaxBSP(void)
{
	BSP_Free( q3bsp );
	q3bsp = NULL;
} //end of the function Q3_FreeMaxBSP


//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Q3_PlaneFromPoints(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t normal, float *dist)
{
	vec3_t t1, t2;

	VectorSubtract(p0, p1, t1);
	VectorSubtract(p2, p1, t2);
	CrossProduct(t1, t2, normal);
	VectorNormalize(normal);

	*dist = DotProduct(p0, normal);
} //end of the function PlaneFromPoints
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q3_SurfacePlane(dsurface_t *surface, vec3_t normal, float *dist)
{
	int i;
	float *p0, *p1, *p2;
	vec3_t t1, t2;

	VectorClear( t1 );
	VectorClear( t2 );

	p0 = q3bsp->drawVerts[surface->firstVert].xyz;
	for (i = 1; i < surface->numVerts-1; i++)
	{
		p1 = q3bsp->drawVerts[surface->firstVert + ((i) % surface->numVerts)].xyz;
		p2 = q3bsp->drawVerts[surface->firstVert + ((i+1) % surface->numVerts)].xyz;
		VectorSubtract(p0, p1, t1);
		VectorSubtract(p2, p1, t2);
		CrossProduct(t1, t2, normal);
		VectorNormalize(normal);
		if (VectorLength(normal)) break;
	} //end for*/
/*
	float dot;
	for (i = 0; i < surface->numVerts; i++)
	{
		p0 = q3bsp->drawVerts[surface->firstVert + ((i) % surface->numVerts)].xyz;
		p1 = q3bsp->drawVerts[surface->firstVert + ((i+1) % surface->numVerts)].xyz;
		p2 = q3bsp->drawVerts[surface->firstVert + ((i+2) % surface->numVerts)].xyz;
		VectorSubtract(p0, p1, t1);
		VectorSubtract(p2, p1, t2);
		VectorNormalize(t1);
		VectorNormalize(t2);
		dot = DotProduct(t1, t2);
		if (dot > -0.9 && dot < 0.9 &&
			VectorLength(t1) > 0.1 && VectorLength(t2) > 0.1) break;
	} //end for
	CrossProduct(t1, t2, normal);
	VectorNormalize(normal);
*/
	if (VectorLength(normal) < 0.9)
	{
		printf("surface %u bogus normal vector %f %f %f\n", (unsigned int)(surface - q3bsp->surfaces), normal[0], normal[1], normal[2]);
		printf("t1 = %f %f %f, t2 = %f %f %f\n", t1[0], t1[1], t1[2], t2[0], t2[1], t2[2]);
		for (i = 0; i < surface->numVerts; i++)
		{
			p1 = q3bsp->drawVerts[surface->firstVert + ((i) % surface->numVerts)].xyz;
			Log_Print("p%d = %f %f %f\n", i, p1[0], p1[1], p1[2]);
		} //end for
	} //end if
	*dist = DotProduct(p0, normal);
} //end of the function Q3_SurfacePlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
dplane_t *q3_surfaceplanes;

void Q3_CreatePlanarSurfacePlanes(void)
{
	int i;
	dsurface_t *surface;

	Log_Print("creating planar surface planes...\n");
	q3_surfaceplanes = (dplane_t *) GetClearedMemory(q3bsp->numSurfaces * sizeof(dplane_t));

	for (i = 0; i < q3bsp->numSurfaces; i++)
	{
		surface = &q3bsp->surfaces[i];
		if (surface->surfaceType != MST_PLANAR) continue;
		Q3_SurfacePlane(surface, q3_surfaceplanes[i].normal, &q3_surfaceplanes[i].dist);
		//Log_Print("normal = %f %f %f, dist = %f\n", q3_surfaceplanes[i].normal[0],
		//											q3_surfaceplanes[i].normal[1],
		//											q3_surfaceplanes[i].normal[2], q3_surfaceplanes[i].dist);
	} //end for
} //end of the function Q3_CreatePlanarSurfacePlanes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q3_FreePlanarSurfacePlanes(void)
{
	if ( q3_surfaceplanes ) FreeMemory( q3_surfaceplanes );
	q3_surfaceplanes = NULL;
} //end of the function Q3_FreePlanarSurfacePlanes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
/*
void Q3_SurfacePlane(dsurface_t *surface, vec3_t normal, float *dist)
{
	//take the plane information from the lightmap vector
	// VectorCopy(surface->lightmapVecs[2], normal);
	//calculate plane dist with first surface vertex
	// *dist = DotProduct(q3bsp->drawVerts[surface->firstVert].xyz, normal);
	Q3_PlaneFromPoints(q3bsp->drawVerts[surface->firstVert].xyz,
						q3bsp->drawVerts[surface->firstVert+1].xyz,
						q3bsp->drawVerts[surface->firstVert+2].xyz, normal, dist);
} //end of the function Q3_SurfacePlane*/
//===========================================================================
// returns the amount the face and the winding overlap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Q3_FaceOnWinding(dsurface_t *surface, winding_t *winding)
{
	int i;
	float dist, area;
	dplane_t plane;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	winding_t *w;

	//copy the winding before chopping
	w = CopyWinding(winding);
	//retrieve the surface plane
	Q3_SurfacePlane(surface, plane.normal, &plane.dist);
	//chop the winding with the surface edge planes
	for (i = 0; i < surface->numVerts && w; i++)
	{
		v1 = q3bsp->drawVerts[surface->firstVert + ((i) % surface->numVerts)].xyz;
		v2 = q3bsp->drawVerts[surface->firstVert + ((i+1) % surface->numVerts)].xyz;
		//create a plane through the edge from v1 to v2, orthogonal to the
		//surface plane and with the normal vector pointing inward
		VectorSubtract(v2, v1, edgevec);
		CrossProduct(edgevec, plane.normal, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		ChopWindingInPlace(&w, normal, dist, -0.1); //CLIP_EPSILON
	} //end for
	if (w)
	{
		area = WindingArea(w);
		FreeWinding(w);
		return area;
	} //end if
	return 0;
} //end of the function Q3_FaceOnWinding
//===========================================================================
// creates a winding for the given brush side on the given brush
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
winding_t *Q3_BrushSideWinding(dbrush_t *brush, dbrushside_t *baseside)
{
	int i;
	dplane_t *baseplane, *plane;
	winding_t *w;
	dbrushside_t *side;

	//create a winding for the brush side with the given planenumber
	baseplane = &q3bsp->planes[baseside->planeNum];
	w = BaseWindingForPlane(baseplane->normal, baseplane->dist);
	for (i = 0; i < brush->numSides && w; i++)
	{
		side = &q3bsp->brushSides[brush->firstSide + i];
		//don't chop with the base plane
		if (side->planeNum == baseside->planeNum) continue;
		//also don't use planes that are almost equal
		plane = &q3bsp->planes[side->planeNum];
		if (DotProduct(baseplane->normal, plane->normal) > 0.999
				&& fabs(baseplane->dist - plane->dist) < 0.01) continue;
		//
		plane = &q3bsp->planes[side->planeNum^1];
		ChopWindingInPlace(&w, plane->normal, plane->dist, -0.1); //CLIP_EPSILON);
	} //end for
	return w;
} //end of the function Q3_BrushSideWinding
//===========================================================================
// fix screwed brush texture references
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WindingIsTiny(winding_t *w);

void Q3_FindVisibleBrushSides(void)
{
	int i, j, k, we, numtextured, numsides;
	float dot;
	dplane_t *plane;
	dbrushside_t *brushside;
	dbrush_t *brush;
	dsurface_t *surface;
	winding_t *w;

	memset(q3_dbrushsidetextured, false, Q3_MAX_MAP_BRUSHSIDES);
	//
	numsides = 0;
	//go over all the brushes
	if ( forcesidesvisible ) {
		Log_Print("searching nodraw brush sides...\n");
		for ( i = 0; i < q3bsp->numBrushSides; i++ ) {
			if ( q3bsp->shaders[q3bsp->brushSides[i].shaderNum].surfaceFlags & SURF_NODRAW ) {
				q3_dbrushsidetextured[i] = false;
			} //end if
			else
			{
				q3_dbrushsidetextured[i] = true;
			} // end else
		} //end for
	} else {
		//create planes for the planar surfaces
		Q3_CreatePlanarSurfacePlanes();
		Log_Print("searching visible brush sides...\n");
		Log_Print("%6d brush sides", numsides);
		for (i = 0; i < q3bsp->numBrushes; i++)
		{
			brush = &q3bsp->brushes[i];
			//go over all the sides of the brush
			for (j = 0; j < brush->numSides; j++)
			{
				qprintf("\r%6d", numsides++);
				brushside = &q3bsp->brushSides[brush->firstSide + j];
				//
				if ( q3bsp->shaders[brushside->shaderNum].surfaceFlags & SURF_NODRAW ) {
					continue;
				} //end if
				//
				w = Q3_BrushSideWinding(brush, brushside);
				if (!w)
				{
					q3_dbrushsidetextured[brush->firstSide + j] = true;
					continue;
				} //end if
				else
				{
					//RemoveEqualPoints(w, 0.2);
					if (WindingIsTiny(w))
					{
						FreeWinding(w);
						q3_dbrushsidetextured[brush->firstSide + j] = true;
						continue;
					} //end if
					else
					{
						we = WindingError(w);
						if (we == WE_NOTENOUGHPOINTS
							|| we == WE_SMALLAREA
							|| we == WE_POINTBOGUSRANGE
	//						|| we == WE_NONCONVEX
							)
						{
							FreeWinding(w);
							q3_dbrushsidetextured[brush->firstSide + j] = true;
							continue;
						} //end if
					} //end else
				} //end else
				if (WindingArea(w) < 20)
				{
					q3_dbrushsidetextured[brush->firstSide + j] = true;
					continue;
				} //end if
				//find a face for texturing this brush
				for (k = 0; k < q3bsp->numSurfaces; k++)
				{
					surface = &q3bsp->surfaces[k];
					if (surface->surfaceType != MST_PLANAR) continue;
					//
					//Q3_SurfacePlane(surface, plane.normal, &plane.dist);
					plane = &q3_surfaceplanes[k];
					//the surface plane and the brush side plane should be pretty much the same
					if (fabs(fabs(plane->dist) - fabs(q3bsp->planes[brushside->planeNum].dist)) > 5) continue;
					dot = DotProduct(plane->normal, q3bsp->planes[brushside->planeNum].normal);
					if (dot > -0.9 && dot < 0.9) continue;
					//if the face is partly or totally on the brush side
					if (Q3_FaceOnWinding(surface, w))
					{
						q3_dbrushsidetextured[brush->firstSide + j] = true;
						//Log_Write("Q3_FaceOnWinding");
						break;
					} //end if
				} //end for
				FreeWinding(w);
			} //end for
		} //end for
		qprintf("\r%6d brush sides\n", numsides);
		Q3_FreePlanarSurfacePlanes();
	} //end if
	numtextured = 0;
	for (i = 0; i < q3bsp->numBrushSides; i++)
	{
		if (q3_dbrushsidetextured[i]) numtextured++;
	} //end for
	Log_Print("%d brush sides textured out of %d\n", numtextured, q3bsp->numBrushSides);
} //end of the function Q3_FindVisibleBrushSides

/*
=============
CountTriangles
=============
*/
void CountTriangles( void ) {
	int i, numTris, numPatchTris;
	dsurface_t *surface;

	numTris = numPatchTris = 0;
	for ( i = 0; i < q3bsp->numSurfaces; i++ ) {
		surface = &q3bsp->surfaces[i];

		numTris += surface->numIndexes / 3;

		if ( surface->patchWidth ) {
			numPatchTris += surface->patchWidth * surface->patchHeight * 2;
		}
	}

	Log_Print( "%6d triangles\n", numTris );
	Log_Print( "%6d patch tris\n", numPatchTris );
}

/*
=============
Q3_LoadBSPFile
=============
*/
void	Q3_LoadBSPFile(struct quakefile_s *qf)
{
	q3bsp = BSP_Load( (const char *)qf );

	CountTriangles();

	Q3_FindVisibleBrushSides();
}

//============================================================================

/*
================
Q3_ParseEntities

Parses the entity string into entities
================
*/
void Q3_ParseEntities (void)
{
	script_t *script;

	num_entities = 0;
	script = LoadScriptMemory(q3bsp->entityString, q3bsp->entityStringLength, "*Quake3 bsp file");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES |
									SCFL_NOSTRINGESCAPECHARS);

	while(ParseEntity(script))
	{
	} //end while

	FreeScript(script);
} //end of the function Q3_ParseEntities

