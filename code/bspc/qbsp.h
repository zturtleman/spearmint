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


#include <stdlib.h>
#include "../qcommon/q_shared.h"
#include "l_cmd.h"
#include "l_math.h"
#include "l_poly.h"
#include "l_threads.h"
#include "../botlib/l_script.h"
#include "l_bsp_ent.h"
#include "qfiles.h"
#include "l_mem.h"
#include "l_utils.h"
#include "l_log.h"
#include "l_qfiles.h"

#define BSPC_NAME			"Spearmint BSPC"
#define BSPC_VERSION		"2.1h"

#define ZTMAUTOARGS

#define DEBUG
#define NODELIST

#ifdef _MSC_VER
  // vsnprintf is ISO/IEC 9899:1999
  // abstracting this to make it portable
  int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#else
  #define Q_vsnprintf vsnprintf
#endif

#define MAX_BRUSH_SIDES		128		//maximum number of sides per brush
#define CLIP_EPSILON		0.1
#define MAX_MAP_BOUNDS		65535
#define BOGUS_RANGE			(MAX_MAP_BOUNDS+128)	//somewhere outside the map
#define TEXINFO_NODE		-1		//side is allready on a node
#define PLANENUM_LEAF		-1		//used for leaf nodes
#define MAXEDGES			20		//maximum number of face edges
#define MAX_NODE_BRUSHES	8		//maximum brushes in a node
//side flags
#define SFL_TESTED			1
#define SFL_VISIBLE			2
#define SFL_BEVEL			4
#define SFL_TEXTURED		8
#define SFL_CURVE			16

//map plane
typedef struct plane_s
{
	vec3_t normal;
	vec_t dist;
	int type;
	int signbits;
	struct plane_s	*hash_chain;
} plane_t;
//brush texture
typedef struct
{
	vec_t	shift[2];
	vec_t	rotate;
	vec_t	scale[2];
	char	name[32];
	int		flags;
	int		value;
} brush_texture_t;
//brush side
typedef struct side_s
{
	int				planenum;	// map plane this side is in
	int				texinfo;		// texture reference
	winding_t		*winding;	// winding of this side
	struct side_s	*original;	// bspbrush_t sides will reference the mapbrush_t sides
	int				contents;	// from miptex
	int				surf;			// from miptex
	unsigned short flags;		// side flags
} side_t;		//sizeof(side_t) = 36
//map brush
typedef struct mapbrush_s
{
	int		entitynum;
	int		brushnum;

	int		contents;
	int		expansionbbox;			//bbox used for expansion of the brush
	int		leafnum;
	int		modelnum;

	vec3_t	mins, maxs;

	int		numsides;
	side_t	*original_sides;
} mapbrush_t;
//bsp face
typedef struct face_s
{
	struct face_s		*next;		// on node

	// the chain of faces off of a node can be merged or split,
	// but each face_t along the way will remain in the chain
	// until the entire tree is freed
	struct face_s		*merged;	// if set, this face isn't valid anymore
	struct face_s		*split[2];	// if set, this face isn't valid anymore

	struct portal_s	*portal;
	int					texinfo;
	int					planenum;
	int					contents;	// faces in different contents can't merge
	int					outputnumber;
	winding_t			*w;
	int					numpoints;
	qboolean				badstartvert;	// tjunctions cannot be fixed without a midpoint vertex
	int					vertexnums[MAXEDGES];
} face_t;
//bsp brush
typedef struct bspbrush_s
{
	struct		bspbrush_s	*next;
	vec3_t		mins, maxs;
	int			side, testside;		// side of node during construction
	mapbrush_t	*original;
	int			numsides;
	side_t		sides[6];			// variably sized
} bspbrush_t;	//sizeof(bspbrush_t) = 44 + numsides * sizeof(side_t)
//bsp node
typedef struct node_s
{
	//both leafs and nodes
	int				planenum;	// -1 = leaf node
	struct node_s	*parent;
	vec3_t			mins, maxs;	// valid after portalization
	bspbrush_t		*volume;		// one for each leaf/node

	// nodes only
	qboolean			detail_seperator;	// a detail brush caused the split
	side_t			*side;		// the side that created the node
	struct node_s	*children[2];
	face_t			*faces;

	// leafs only
	bspbrush_t		*brushlist;	// fragments of all brushes in this leaf
	int				contents;	// OR of all brush contents
	int				occupied;	// 1 or greater can reach entity
	entity_t			*occupant;	// for leak file testing
	int				cluster;		// for portalfile writing
	int				area;			// for areaportals
	struct portal_s *portals;	// also on nodes during construction
#ifdef NODELIST
	struct node_s *next;			//next node in the nodelist
#endif
	int expansionbboxes;			//OR of all bboxes used for expansion of the brushes
	int modelnum;
} node_t;		//sizeof(node_t) = 80 bytes
//bsp portal
typedef struct portal_s
{
	plane_t plane;
	node_t *onnode;					// NULL = outside box
	node_t *nodes[2];				// [0] = front side of plane
	struct portal_s *next[2];
	winding_t *winding;

	qboolean	sidefound;			// false if ->side hasn't been checked
	side_t *side;					// NULL = non-visible
	face_t *face[2];				// output face in bsp file
	struct tmp_face_s *tmpface;		//pointer to the tmpface created for this portal
	int planenum;					//number of the map plane used by the portal
} portal_t;
//bsp tree
typedef struct
{
	node_t		*headnode;
	node_t		outside_node;
	vec3_t		mins, maxs;
} tree_t;

//=============================================================================
// bspc.c
//=============================================================================

extern	qboolean noprune;
extern	qboolean nodetail;
extern	qboolean fulldetail;
extern	qboolean nomerge;
extern	qboolean nosubdiv;
extern	qboolean nowater;
extern	qboolean noweld;
extern	qboolean noshare;
extern	qboolean notjunc;
extern	qboolean onlyents;
extern	qboolean nocsg;
extern	qboolean create_aas;
extern	qboolean freetree;
extern	qboolean lessbrushes;
extern	qboolean nobrushmerge;
extern	qboolean cancelconversion;
extern	qboolean noliquids;
extern	qboolean capsule_collision;

extern	float subdivide_size;
extern	vec_t microvolume;

extern	char outbase[32];
extern	char source[1024];

//=============================================================================
// map.c
//=============================================================================

#define MAX_MAPFILE_PLANES			256000
#define MAX_MAPFILE_BRUSHES			65535
#define MAX_MAPFILE_BRUSHSIDES		(MAX_MAPFILE_BRUSHES*8)

extern	int			entity_num;

extern	plane_t		mapplanes[MAX_MAPFILE_PLANES];
extern	int			nummapplanes;
extern	int			mapplaneusers[MAX_MAPFILE_PLANES];

extern	int			nummapbrushes;
extern	mapbrush_t	mapbrushes[MAX_MAPFILE_BRUSHES];

extern	vec3_t		map_mins, map_maxs;

extern	int			nummapbrushsides;
extern	side_t		brushsides[MAX_MAPFILE_BRUSHSIDES];
extern	brush_texture_t	side_brushtextures[MAX_MAPFILE_BRUSHSIDES];

#define NODESTACKSIZE		1024

extern	int nodestack[NODESTACKSIZE];
extern	int *nodestackptr;
extern	int nodestacksize;
extern	int brushmodelnumbers[MAX_MAPFILE_BRUSHES];
extern	int dbrushleafnums[MAX_MAPFILE_BRUSHES];
extern	int dplanes2mapplanes[MAX_MAPFILE_PLANES];

extern	int c_boxbevels;
extern	int c_edgebevels;
extern	int c_areaportals;
extern	int c_clipbrushes;
extern	int c_squattbrushes;

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are allways opposites

//finds a float plane for the given normal and distance
int FindFloatPlane(vec3_t normal, vec_t dist);
//returns the plane type for the given normal
int BSPC_PlaneTypeForNormal(vec3_t normal);
//add bevels to the map brush
void AddBrushBevels(mapbrush_t *b);
//makes brush side windings for the brush
qboolean MakeBrushWindings(mapbrush_t *ob);
//marks brush bevels of the brush as bevel
void MarkBrushBevels(mapbrush_t *brush);
//returns true if the map brush already exists
int BrushExists(mapbrush_t *brush);
//loads a map from a bsp file
int LoadMapFromBSP(struct quakefile_s *qf);
//resets map loading
void ResetMapLoading(void);
//print some map info
void PrintMapInfo(void);
//writes a map file (type depending on loaded map type)
void WriteMapFile(char *filename);

//=============================================================================
// map_q3.c
//=============================================================================
void Q3_ResetMapLoading(void);
//loads a map from a Quake3 bsp file
void Q3_LoadMapFromBSP(struct quakefile_s *qf);

//=============================================================================
// csg
//=============================================================================

bspbrush_t *MakeBspBrushList(int startbrush, int endbrush, vec3_t clipmins, vec3_t clipmaxs);
bspbrush_t *ChopBrushes(bspbrush_t *head);
bspbrush_t *InitialBrushList(bspbrush_t *list);
bspbrush_t *OptimizedBrushList(bspbrush_t *list);
void WriteBrushMap(char *name, bspbrush_t *list);
void CheckBSPBrush(bspbrush_t *brush);
void BSPBrushWindings(bspbrush_t *brush);
bspbrush_t *TryMergeBrushes(bspbrush_t *brush1, bspbrush_t *brush2);
tree_t *ProcessWorldBrushes(int brush_start, int brush_end);

//=============================================================================
// brushbsp
//=============================================================================

#define	PSIDE_FRONT			1
#define	PSIDE_BACK			2
#define	PSIDE_BOTH			(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING		4

void WriteBrushList(char *name, bspbrush_t *brush, qboolean onlyvis);
bspbrush_t *CopyBrush(bspbrush_t *brush);
void SplitBrush(bspbrush_t *brush, int planenum, bspbrush_t **front, bspbrush_t **back);
node_t *AllocNode(void);
bspbrush_t *AllocBrush(int numsides);
int CountBrushList(bspbrush_t *brushes);
void FreeBrush(bspbrush_t *brushes);
vec_t BrushVolume(bspbrush_t *brush);
void BoundBrush(bspbrush_t *brush);
void FreeBrushList(bspbrush_t *brushes);
tree_t *BrushBSP(bspbrush_t *brushlist, vec3_t mins, vec3_t maxs);
bspbrush_t *BrushFromBounds(vec3_t mins, vec3_t maxs);
int BrushMostlyOnSide(bspbrush_t *brush, plane_t *plane);
qboolean WindingIsHuge(winding_t *w);
qboolean WindingIsTiny(winding_t *w);
void ResetBrushBSP(void);

//=============================================================================
// portals.c
//=============================================================================

int VisibleContents (int contents);
void MakeHeadnodePortals (tree_t *tree);
void MakeNodePortal (node_t *node);
void SplitNodePortals (node_t *node);
qboolean Portal_VisFlood (portal_t *p);
qboolean FloodEntities (tree_t *tree);
void FillOutside (node_t *headnode);
void FloodAreas (tree_t *tree);
void MarkVisibleSides (tree_t *tree, int start, int end);
void FreePortal (portal_t *p);
void EmitAreaPortals (node_t *headnode);
void MakeTreePortals (tree_t *tree);

//=============================================================================
// leakfile.c
//=============================================================================

void LeakFile (tree_t *tree);

//=============================================================================
// tree.c
//=============================================================================

tree_t *Tree_Alloc(void);
void Tree_Free(tree_t *tree);
void Tree_Free_r(node_t *node);
void Tree_Print_r(node_t *node, int depth);
void Tree_FreePortals_r(node_t *node);
void Tree_PruneNodes_r(node_t *node);
void Tree_PruneNodes(node_t *node);
