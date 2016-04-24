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

/*****************************************************************************
 * name:		be_aas_entity.c
 *
 * desc:		AAS entities
 *
 * $Archive: /MissionPack/code/botlib/be_aas_entity.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_UpdateEntity(int entnum, bot_entitystate_t *state)
{
	qboolean relink;
	aas_entity_t *ent;

	if (!aasworld.loaded)
	{
		botimport.Print(PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n");
		return BLERR_NOAASFILE;
	} //end if

	ent = &aasworld.entities[entnum];

	if (!state) {
		//unlink the entity
		AAS_UnlinkFromAreas(ent->areas);
		//unlink the entity from the BSP leaves
		AAS_UnlinkFromBSPLeaves(ent->leaves);
		//
		ent->areas = NULL;
		//
		ent->leaves = NULL;
		return BLERR_NOERROR;
	}

	//updated so set valid flag
	ent->valid = qtrue;
	//link everything the first frame
	if (aasworld.numframes == 1) relink = qtrue;
	else relink = state->relink;
	//if the entity should be relinked
	if (relink)
	{
		//don't link the world model
		if (entnum != ENTITYNUM_WORLD)
		{
			//unlink the entity
			AAS_UnlinkFromAreas(ent->areas);
			//relink the entity to the AAS areas (use the larges bbox)
			ent->areas = AAS_LinkEntityClientBBox(state->absmins, state->absmaxs, entnum, PRESENCE_NORMAL);
			//unlink the entity from the BSP leaves
			AAS_UnlinkFromBSPLeaves(ent->leaves);
			//link the entity to the world BSP tree
			ent->leaves = AAS_BSPLinkEntity(state->absmins, state->absmaxs, entnum, 0);
		} //end if
	} //end if
	return BLERR_NOERROR;
} //end of the function AAS_UpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ResetEntityLinks(void)
{
	int i;
	for (i = 0; i < aasworld.maxentities; i++)
	{
		aasworld.entities[i].areas = NULL;
		aasworld.entities[i].leaves = NULL;
	} //end for
} //end of the function AAS_ResetEntityLinks
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InvalidateEntities(void)
{
	int i;
	for (i = 0; i < aasworld.maxentities; i++)
	{
		aasworld.entities[i].valid = qfalse;
	} //end for
} //end of the function AAS_InvalidateEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_UnlinkInvalidEntities(void)
{
	int i;
	aas_entity_t *ent;

	for (i = 0; i < aasworld.maxentities; i++)
	{
		ent = &aasworld.entities[i];
		if (!ent->valid)
		{
			AAS_UnlinkFromAreas( ent->areas );
			ent->areas = NULL;
			AAS_UnlinkFromBSPLeaves( ent->leaves );
			ent->leaves = NULL;
		} //end for
	} //end for
} //end of the function AAS_UnlinkInvalidEntities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NextEntity(int entnum)
{
	if (!aasworld.loaded) return 0;

	if (entnum < 0) entnum = -1;
	while(++entnum < aasworld.maxentities)
	{
		if (aasworld.entities[entnum].valid) return entnum;
	} //end while
	return 0;
} //end of the function AAS_NextEntity
