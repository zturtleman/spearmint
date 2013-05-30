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
 * name:		ai_goal.c
 *
 * desc:		goal AI
 *
 * $Archive: /MissionPack/code/game/ai_goal.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
//
#include "ai_ea.h"
#include "ai_goal.h"
#include "ai_move.h"
#include "ai_weap.h"
#include "ai_weight.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_vcmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
//
#include "chars.h"				//characteristics
#include "inv.h"				//indexes into the inventory
#include "syn.h"				//synonyms
#include "match.h"				//string matching types and vars

//#define DEBUG_AI_GOAL
#ifdef RANDOMIZE
#define UNDECIDEDFUZZY
#endif //RANDOMIZE
#define DROPPEDWEIGHT
//minimum avoid goal time
#define AVOID_MINIMUM_TIME		10
//default avoid goal time
#define AVOID_DEFAULT_TIME		30
//avoid dropped goal time
#define AVOID_DROPPED_TIME		10
//
#define TRAVELTIME_SCALE		0.01
//item flags
#define IFL_NOTFREE				1		//not in free for all
#define IFL_NOTTEAM				2		//not in team play
#define IFL_NOTSINGLE			4		//not in single player
#define IFL_NOTBOT				8		//bot should never go for this
#define IFL_ROAM				16		//bot roam goal

//camp spots "info_camp"
typedef struct campspot_s
{
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	float range;
	float weight;
	float wait;
	float random;
	struct campspot_s *next;
} campspot_t;

typedef struct levelitem_s
{
	int number;							//number of the level item
	int iteminfo;						//index into the item info
	int flags;							//item flags
	float weight;						//fixed roam weight
	vec3_t origin;						//origin of the item
	int goalareanum;					//area the item is in
	vec3_t goalorigin;					//goal origin within the area
	int entitynum;						//entity number
	float timeout;						//item is removed after this time
	struct levelitem_s *prev, *next;
} levelitem_t;

typedef struct iteminfo_s
{
	char classname[32];					//classname of the item
	char name[MAX_STRINGFIELD];			//name of the item
	char model[MAX_STRINGFIELD];		//model of the item
	int modelindex;						//model index
	int type;							//item type
	int index;							//index in the inventory
	float respawntime;					//respawn time
	vec3_t mins;						//mins of the item
	vec3_t maxs;						//maxs of the item
	int number;							//number of the item info
} iteminfo_t;

#define ITEMINFO_OFS(x)	(size_t)&(((iteminfo_t *)0)->x)

fielddef_t iteminfo_fields[] =
{
{"name", ITEMINFO_OFS(name), FT_STRING},
{"model", ITEMINFO_OFS(model), FT_STRING},
{"modelindex", ITEMINFO_OFS(modelindex), FT_INT},
{"type", ITEMINFO_OFS(type), FT_INT},
{"index", ITEMINFO_OFS(index), FT_INT},
{"respawntime", ITEMINFO_OFS(respawntime), FT_FLOAT},
{"mins", ITEMINFO_OFS(mins), FT_FLOAT|FT_ARRAY, 3},
{"maxs", ITEMINFO_OFS(maxs), FT_FLOAT|FT_ARRAY, 3},
{NULL, 0, 0}
};

structdef_t iteminfo_struct =
{
	sizeof(iteminfo_t), iteminfo_fields
};

typedef struct itemconfig_s
{
	int numiteminfo;
	iteminfo_t iteminfo[MAX_ITEMS];
} itemconfig_t;

static bot_goalstate_t botgoalstates[MAX_CLIENTS + 1]; // FIXME: init?
//item configuration
static itemconfig_t bot_itemconfig;
itemconfig_t *itemconfig = NULL;
//level items
levelitem_t *levelitemheap = NULL;
levelitem_t *freelevelitems = NULL;
levelitem_t *levelitems = NULL;
int numlevelitems = 0;
//camp spots
campspot_t *campspots = NULL;

//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
bot_goalstate_t *BotGoalStateFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		BotAI_Print(PRT_FATAL, "goal state handle %d out of range\n", handle);
		return NULL;
	} //end if
	/*
	if (!botgoalstates[handle])
	{
		BotAI_Print(PRT_FATAL, "invalid goal state %d\n", handle);
		return NULL;
	} //end if
	*/
	return &botgoalstates[handle];
} //end of the function BotGoalStateFromHandle
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child)
{
	bot_goalstate_t *p1, *p2, *c;

	p1 = BotGoalStateFromHandle(parent1);
	p2 = BotGoalStateFromHandle(parent2);
	c = BotGoalStateFromHandle(child);

	InterbreedWeightConfigs(p1->itemweightconfig, p2->itemweightconfig,
									c->itemweightconfig);
} //end of the function BotInterbreedingGoalFuzzyLogic
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotSaveGoalFuzzyLogic(int goalstate, char *filename)
{
	//bot_goalstate_t *gs;

	//gs = BotGoalStateFromHandle(goalstate);

	//WriteWeightConfig(filename, gs->itemweightconfig);
} //end of the function BotSaveGoalFuzzyLogic
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMutateGoalFuzzyLogic(int goalstate, float range)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);

	EvolveWeightConfig(gs->itemweightconfig);
} //end of the function BotMutateGoalFuzzyLogic
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
itemconfig_t *LoadItemConfig(char *filename)
{
	pc_token_t token;
	char path[MAX_QPATH];
	int source;
	itemconfig_t *ic;
	iteminfo_t *ii;

	strncpy( path, filename, MAX_QPATH );
	source = trap_PC_LoadSource( path, BOTFILESBASEFOLDER );
	if( !source ) {
		BotAI_Print( PRT_ERROR, "counldn't load %s\n", path );
		return NULL;
	} //end if
	//initialize item config
	ic = &bot_itemconfig;
	Com_Memset( ic, 0, sizeof (itemconfig_t) );
	ic->numiteminfo = 0;
	//parse the item config file
	while(trap_PC_ReadToken(source, &token))
	{
		if (!strcmp(token.string, "iteminfo"))
		{
			if (ic->numiteminfo >= MAX_ITEMS)
			{
				PC_SourceError(source, "more than %d item info defined", MAX_ITEMS);
				trap_PC_FreeSource(source);
				return NULL;
			} //end if
			ii = &ic->iteminfo[ic->numiteminfo];
			Com_Memset(ii, 0, sizeof(iteminfo_t));
			if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
			{
				trap_PC_FreeSource(source);
				return NULL;
			} //end if
			strncpy(ii->classname, token.string, sizeof(ii->classname)-1);
			if (!PC_ReadStructure(source, &iteminfo_struct, (void *) ii))
			{
				trap_PC_FreeSource(source);
				return NULL;
			} //end if
			ii->number = ic->numiteminfo;
			ic->numiteminfo++;
		} //end if
		else
		{
			PC_SourceError(source, "unknown definition %s", token.string);
			trap_PC_FreeSource(source);
			return NULL;
		} //end else
	} //end while
	trap_PC_FreeSource(source);
	//
	if (!ic->numiteminfo) BotAI_Print(PRT_WARNING, "no item info loaded\n");
	BotAI_Print(PRT_DEVELOPER, "loaded %s\n", path);
	return ic;
} //end of the function LoadItemConfig
//===========================================================================
// index to find the weight function of an iteminfo
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ItemWeightIndex(const itemconfig_t *ic, weightconfig_t *iwc, int *index)
{
	int i;

	for (i = 0; i < ic->numiteminfo; i++)
	{
		index[i] = FindFuzzyWeight(iwc, ic->iteminfo[i].classname);
		if (index[i] < 0 && bot_developer.integer)
		{
			G_Printf("item info %d \"%s\" has no fuzzy weight\r\n", i, ic->iteminfo[i].classname);
		} //end if
	} //end for
	for ( ; i < MAX_ITEMS; i++ )
	{
		index[i] = -1;
	} //end for
	return;
} //end of the function ItemWeightIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void InitLevelItemHeap(void)
{
	int i;
	static levelitem_t bot_levelitemheap[MAX_ITEMS];

	levelitemheap = bot_levelitemheap;

	for (i = 0; i < MAX_ITEMS-1; i++)
	{
		levelitemheap[i].next = &levelitemheap[i + 1];
	} //end for
	levelitemheap[MAX_ITEMS-1].next = NULL;
	//
	freelevelitems = levelitemheap;
} //end of the function InitLevelItemHeap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
levelitem_t *AllocLevelItem(void)
{
	levelitem_t *li;

	li = freelevelitems;
	if (!li)
	{
		BotAI_Print(PRT_FATAL, "out of level items\n");
		return NULL;
	} //end if
	//
	freelevelitems = freelevelitems->next;
	Com_Memset(li, 0, sizeof(levelitem_t));
	return li;
} //end of the function AllocLevelItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FreeLevelItem(levelitem_t *li)
{
	li->next = freelevelitems;
	freelevelitems = li;
} //end of the function FreeLevelItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddLevelItemToList(levelitem_t *li)
{
	if (levelitems) levelitems->prev = li;
	li->prev = NULL;
	li->next = levelitems;
	levelitems = li;
} //end of the function AddLevelItemToList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveLevelItemFromList(levelitem_t *li)
{
	if (li->prev) li->prev->next = li->next;
	else levelitems = li->next;
	if (li->next) li->next->prev = li->prev;
} //end of the function RemoveLevelItemFromList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotInitInfoEntities(void)
{
	static qboolean bot_loadedInfoEntities = qfalse;
	char classname[MAX_EPAIRKEY];
	campspot_t *cs;
	int ent, numcampspots;

	if ( bot_loadedInfoEntities ) {
		return;
	}
	bot_loadedInfoEntities = qtrue;
	//
	numcampspots = 0;
	for (ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent))
	{
		if (!trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;

		if (!strcmp(classname, "info_camp"))
		{
			vec3_t origin;
			int areanum;

			trap_AAS_VectorForBSPEpairKey(ent, "origin", origin);
			//origin[2] += 16;

			areanum = trap_AAS_PointAreaNum(origin);
			if (!areanum)
			{
				BotAI_Print(PRT_MESSAGE, "camp spot at %1.1f %1.1f %1.1f in solid\n", origin[0], origin[1], origin[2]);
				continue;
			} //end if

			cs = (campspot_t *) G_Alloc(sizeof(campspot_t));
			VectorCopy(origin, cs->origin);
			trap_AAS_ValueForBSPEpairKey(ent, "message", cs->name, sizeof(cs->name));
			trap_AAS_FloatForBSPEpairKey(ent, "range", &cs->range);
			trap_AAS_FloatForBSPEpairKey(ent, "weight", &cs->weight);
			trap_AAS_FloatForBSPEpairKey(ent, "wait", &cs->wait);
			trap_AAS_FloatForBSPEpairKey(ent, "random", &cs->random);
			cs->areanum = areanum;
			//
			cs->next = campspots;
			campspots = cs;
			//AAS_DrawPermanentCross(cs->origin, 4, LINECOLOR_YELLOW);
			numcampspots++;
		} //end else if
	} //end for
	BotAI_Print(PRT_DEVELOPER, "%d camp spots\n", numcampspots);
} //end of the function BotInitInfoEntities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotInitLevelItems(void)
{
	int i, spawnflags, value;
	char classname[MAX_EPAIRKEY];
	vec3_t origin, end;
	int ent, goalareanum;
	itemconfig_t *ic;
	levelitem_t *li;
	trace_t trace;

	//initialize the map locations and camp spots
	BotInitInfoEntities();

	//initialize the level item heap
	InitLevelItemHeap();
	levelitems = NULL;
	numlevelitems = 0;
	//
	ic = itemconfig;
	if (!ic) return;

	//if there's no AAS file loaded
	if (!trap_AAS_Loaded()) return;

	//validate the modelindexes of the item info
	for (i = 0; i < ic->numiteminfo; i++)
	{
		if (!ic->iteminfo[i].modelindex)
		{
			BotAI_Print(PRT_DEVELOPER, "item %s has modelindex 0", ic->iteminfo[i].classname);
		} //end if
	} //end for

	for (ent = trap_AAS_NextBSPEntity(0); ent; ent = trap_AAS_NextBSPEntity(ent))
	{
		if (!trap_AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		//
		spawnflags = 0;
		trap_AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		//
		for (i = 0; i < ic->numiteminfo; i++)
		{
			if (!strcmp(classname, ic->iteminfo[i].classname)) break;
		} //end for
		if (i >= ic->numiteminfo)
		{
			BotAI_Print(PRT_DEVELOPER, "entity %s unknown item\r\n", classname);
			continue;
		} //end if
		//get the origin of the item
		if (!trap_AAS_VectorForBSPEpairKey(ent, "origin", origin))
		{
			BotAI_Print(PRT_ERROR, "item %s without origin\n", classname);
			continue;
		} //end else
		//
		goalareanum = 0;
		//if it is a floating item
		if (spawnflags & 1)
		{
			//if the item is not floating in water
			if (!(trap_AAS_PointContents(origin) & CONTENTS_WATER))
			{
				VectorCopy(origin, end);
				end[2] -= 32;
				trap_Trace(&trace, origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, end, -1, CONTENTS_SOLID|CONTENTS_PLAYERCLIP);
				//if the item not near the ground
				if (trace.fraction >= 1)
				{
					//if the item is not reachable from a jumppad
					goalareanum = trap_AAS_BestReachableFromJumpPadArea(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs);
					BotAI_Print(PRT_DEVELOPER, "item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
					if (!goalareanum) continue;
				} //end if
			} //end if
		} //end if

		li = AllocLevelItem();
		if (!li) return;
		//
		li->number = ++numlevelitems;
		li->timeout = 0;
		li->entitynum = 0;
		//
		li->flags = 0;
		trap_AAS_IntForBSPEpairKey(ent, "notfree", &value);
		if (value) li->flags |= IFL_NOTFREE;
		trap_AAS_IntForBSPEpairKey(ent, "notteam", &value);
		if (value) li->flags |= IFL_NOTTEAM;
		trap_AAS_IntForBSPEpairKey(ent, "notsingle", &value);
		if (value) li->flags |= IFL_NOTSINGLE;
		trap_AAS_IntForBSPEpairKey(ent, "notbot", &value);
		if (value) li->flags |= IFL_NOTBOT;
		if (!strcmp(classname, "item_botroam"))
		{
			li->flags |= IFL_ROAM;
			trap_AAS_FloatForBSPEpairKey(ent, "weight", &li->weight);
		} //end if
		//if not a stationary item
		if (!(spawnflags & 1))
		{
			if (!trap_AAS_DropToFloor(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs))
			{
				BotAI_Print(PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n",
												classname, origin[0], origin[1], origin[2]);
			} //end if
		} //end if
		//item info of the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy(origin, li->origin);
		//
		if (goalareanum)
		{
			li->goalareanum = goalareanum;
			VectorCopy(origin, li->goalorigin);
		} //end if
		else
		{
			//get the item goal area and goal origin
			li->goalareanum = trap_AAS_BestReachableArea(origin,
							ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
							li->goalorigin);
			if (!li->goalareanum)
			{
				BotAI_Print(PRT_MESSAGE, "%s not reachable for bots at (%1.1f %1.1f %1.1f)\n",
												classname, origin[0], origin[1], origin[2]);
			} //end if
		} //end else
		//
		AddLevelItemToList(li);
	} //end for
	BotAI_Print(PRT_DEVELOPER, "found %d level items\n", numlevelitems);
} //end of the function BotInitLevelItems
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotGoalName(int number, char *name, int size)
{
	levelitem_t *li;

	if (!itemconfig) return;
	//
	for (li = levelitems; li; li = li->next)
	{
		if (li->number == number)
		{
			strncpy(name, itemconfig->iteminfo[li->iteminfo].name, size-1);
			name[size-1] = '\0';
			return;
		} //end for
	} //end for
	strcpy(name, "");
} //end of the function BotGoalName
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetAvoidGoals(int goalstate)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	Com_Memset(gs->avoidgoals, 0, MAX_AVOIDGOALS * sizeof(int));
	Com_Memset(gs->avoidgoaltimes, 0, MAX_AVOIDGOALS * sizeof(float));
} //end of the function BotResetAvoidGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpAvoidGoals(int goalstate)
{
	int i;
	bot_goalstate_t *gs;
	char name[32];

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	for (i = 0; i < MAX_AVOIDGOALS; i++)
	{
		if (gs->avoidgoaltimes[i] >= trap_AAS_Time())
		{
			BotGoalName(gs->avoidgoals[i], name, 32);
			BotAI_Print(PRT_DEVELOPER, "avoid goal %s, number %d for %f seconds", name,
				gs->avoidgoals[i], gs->avoidgoaltimes[i] - trap_AAS_Time());
		} //end if
	} //end for
} //end of the function BotDumpAvoidGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotAddToAvoidGoals(bot_goalstate_t *gs, int number, float avoidtime)
{
	int i;

	for (i = 0; i < MAX_AVOIDGOALS; i++)
	{
		//if the avoid goal is already stored
		if (gs->avoidgoals[i] == number)
		{
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = trap_AAS_Time() + avoidtime;
			return;
		} //end if
	} //end for

	for (i = 0; i < MAX_AVOIDGOALS; i++)
	{
		//if this avoid goal has expired
		if (gs->avoidgoaltimes[i] < trap_AAS_Time())
		{
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = trap_AAS_Time() + avoidtime;
			return;
		} //end if
	} //end for
} //end of the function BotAddToAvoidGoals
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotRemoveFromAvoidGoals(int goalstate, int number)
{
	int i;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	//don't use the goals the bot wants to avoid
	for (i = 0; i < MAX_AVOIDGOALS; i++)
	{
		if (gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= trap_AAS_Time())
		{
			gs->avoidgoaltimes[i] = 0;
			return;
		} //end if
	} //end for
} //end of the function BotRemoveFromAvoidGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float BotAvoidGoalTime(int goalstate, int number)
{
	int i;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return 0;
	//don't use the goals the bot wants to avoid
	for (i = 0; i < MAX_AVOIDGOALS; i++)
	{
		if (gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= trap_AAS_Time())
		{
			return gs->avoidgoaltimes[i] - trap_AAS_Time();
		} //end if
	} //end for
	return 0;
} //end of the function BotAvoidGoalTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotSetAvoidGoalTime(int goalstate, int number, float avoidtime)
{
	bot_goalstate_t *gs;
	levelitem_t *li;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
		return;
	if (avoidtime < 0)
	{
		if (!itemconfig)
			return;
		//
		for (li = levelitems; li; li = li->next)
		{
			if (li->number == number)
			{
				avoidtime = itemconfig->iteminfo[li->iteminfo].respawntime;
				if (!avoidtime)
					avoidtime = AVOID_DEFAULT_TIME;
				if (avoidtime < AVOID_MINIMUM_TIME)
					avoidtime = AVOID_MINIMUM_TIME;
				BotAddToAvoidGoals(gs, number, avoidtime);
				return;
			} //end for
		} //end for
		return;
	} //end if
	else
	{
		BotAddToAvoidGoals(gs, number, avoidtime);
	} //end else
} //end of the function BotSetAvoidGoalTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotGetLevelItemGoal(int index, char *name, bot_goal_t *goal)
{
	levelitem_t *li;

	if (!itemconfig) return -1;
	li = levelitems;
	if (index >= 0)
	{
		for (; li; li = li->next)
		{
			if (li->number == index)
			{
				li = li->next;
				break;
			} //end if
		} //end for
	} //end for
	for (; li; li = li->next)
	{
		//
		if (g_gametype.integer == GT_SINGLE_PLAYER) {
			if (li->flags & IFL_NOTSINGLE) continue;
		}
		if (g_gametype.integer >= GT_TEAM) {
			if (li->flags & IFL_NOTTEAM) continue;
		}
		else {
			if (li->flags & IFL_NOTFREE) continue;
		}
		if (li->flags & IFL_NOTBOT) continue;
		//
		if (!Q_stricmp(name, itemconfig->iteminfo[li->iteminfo].name))
		{
			goal->areanum = li->goalareanum;
			VectorCopy(li->goalorigin, goal->origin);
			goal->entitynum = li->entitynum;
			VectorCopy(itemconfig->iteminfo[li->iteminfo].mins, goal->mins);
			VectorCopy(itemconfig->iteminfo[li->iteminfo].maxs, goal->maxs);
			goal->number = li->number;
			goal->flags = GFL_ITEM;
			if (li->timeout) goal->flags |= GFL_DROPPED;
			//BotAI_Print(PRT_MESSAGE, "found li %s\n", itemconfig->iteminfo[li->iteminfo].name);
			return li->number;
		} //end if
	} //end for
	return -1;
} //end of the function BotGetLevelItemGoal
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotGetMapLocationGoal(char *name, bot_goal_t *goal)
{
	gentity_t *eloc;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	if (!level.locationLinked)
		return qfalse;

	for (eloc = level.locationHead; eloc; eloc = eloc->nextTrain)
	{
		if (!Q_stricmp(eloc->message, name))
		{
			goal->areanum = eloc->areanum;
			VectorCopy(eloc->r.currentOrigin, goal->origin);
			goal->entitynum = 0;
			VectorCopy(mins, goal->mins);
			VectorCopy(maxs, goal->maxs);
			return qtrue;
		} //end if
	} //end for
	return qfalse;
} //end of the function BotGetMapLocationGoal
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotGetNextCampSpotGoal(int num, bot_goal_t *goal)
{
	int i;
	campspot_t *cs;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	if (num < 0) num = 0;
	i = num;
	for (cs = campspots; cs; cs = cs->next)
	{
		if (--i < 0)
		{
			goal->areanum = cs->areanum;
			VectorCopy(cs->origin, goal->origin);
			goal->entitynum = 0;
			VectorCopy(mins, goal->mins);
			VectorCopy(maxs, goal->maxs);
			return num+1;
		} //end if
	} //end for
	return 0;
} //end of the function BotGetNextCampSpotGoal
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotFindEntityForLevelItem(levelitem_t *li)
{
	int ent, modelindex;
	itemconfig_t *ic;
	aas_entityinfo_t entinfo;
	vec3_t dir;

	ic = itemconfig;
	if (!itemconfig) return;
	for (ent = BotNextEntity(0); ent; ent = BotNextEntity(ent))
	{
		//get the model index of the entity
		modelindex = g_entities[ent].s.modelindex;
		//
		if (!modelindex) continue;
		//get info about the entity
		BotEntityInfo(ent, &entinfo);
		//if the entity is still moving
		if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
				entinfo.origin[1] != entinfo.lastvisorigin[1] ||
				entinfo.origin[2] != entinfo.lastvisorigin[2]) continue;
		//
		if (ic->iteminfo[li->iteminfo].modelindex == modelindex)
		{
			//check if the entity is very close
			VectorSubtract(li->origin, entinfo.origin, dir);
			if (VectorLength(dir) < 30)
			{
				//found an entity for this level item
				li->entitynum = ent;
			} //end if
		} //end if
	} //end for
} //end of the function BotFindEntityForLevelItem
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotUpdateEntityItems(void)
{
	int ent, i, modelindex;
	vec3_t dir;
	levelitem_t *li, *nextli;
	aas_entityinfo_t entinfo;
	itemconfig_t *ic;

	//timeout current entity items if necessary
	for (li = levelitems; li; li = nextli)
	{
		nextli = li->next;
		//if it is an item that will time out
		if (li->timeout)
		{
			//timeout the item
			if (li->timeout < trap_AAS_Time())
			{
				RemoveLevelItemFromList(li);
				FreeLevelItem(li);
			} //end if
		} //end if
	} //end for
	//find new entity items
	ic = itemconfig;
	if (!itemconfig) return;
	//
	for (ent = BotNextEntity(0); ent; ent = BotNextEntity(ent))
	{
		if (g_entities[ent].s.eType != ET_ITEM) continue;
		//get the model index of the entity
		modelindex = g_entities[ent].s.modelindex;
		//
		if (!modelindex) continue;
		//get info about the entity
		BotEntityInfo(ent, &entinfo);
		//FIXME: don't do this
		//skip all floating items for now
		//if (entinfo.groundent != ENTITYNUM_WORLD) continue;
		//if the entity is still moving
		if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
				entinfo.origin[1] != entinfo.lastvisorigin[1] ||
				entinfo.origin[2] != entinfo.lastvisorigin[2]) continue;
		//check if the entity is already stored as a level item
		for (li = levelitems; li; li = li->next)
		{
			//if the level item is linked to an entity
			if (li->entitynum && li->entitynum == ent)
			{
				//the entity is re-used if the models are different
				if (ic->iteminfo[li->iteminfo].modelindex != modelindex)
				{
					//remove this level item
					RemoveLevelItemFromList(li);
					FreeLevelItem(li);
					li = NULL;
					break;
				} //end if
				else
				{
					if (entinfo.origin[0] != li->origin[0] ||
						entinfo.origin[1] != li->origin[1] ||
						entinfo.origin[2] != li->origin[2])
					{
						VectorCopy(entinfo.origin, li->origin);
						//also update the goal area number
						li->goalareanum = trap_AAS_BestReachableArea(li->origin,
										ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
										li->goalorigin);
					} //end if
					break;
				} //end else
			} //end if
		} //end for
		if (li) continue;
		//try to link the entity to a level item
		for (li = levelitems; li; li = li->next)
		{
			//if this level item is already linked
			if (li->entitynum) continue;
			//
			if (g_gametype.integer == GT_SINGLE_PLAYER) {
				if (li->flags & IFL_NOTSINGLE) continue;
			}
			if (g_gametype.integer >= GT_TEAM) {
				if (li->flags & IFL_NOTTEAM) continue;
			}
			else {
				if (li->flags & IFL_NOTFREE) continue;
			}
			//if the model of the level item and the entity are the same
			if (ic->iteminfo[li->iteminfo].modelindex == modelindex)
			{
				//check if the entity is very close
				VectorSubtract(li->origin, entinfo.origin, dir);
				if (VectorLength(dir) < 30)
				{
					//found an entity for this level item
					li->entitynum = ent;
					//if the origin is different
					if (entinfo.origin[0] != li->origin[0] ||
						entinfo.origin[1] != li->origin[1] ||
						entinfo.origin[2] != li->origin[2])
					{
						//update the level item origin
						VectorCopy(entinfo.origin, li->origin);
						//also update the goal area number
						li->goalareanum = trap_AAS_BestReachableArea(li->origin,
										ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
										li->goalorigin);
					} //end if
#ifdef DEBUG
					BotAI_Print(PRT_DEVELOPER, "linked item %s to an entity", ic->iteminfo[li->iteminfo].classname);
#endif //DEBUG
					break;
				} //end if
			} //end else
		} //end for
		if (li) continue;
		//check if the model is from a known item
		for (i = 0; i < ic->numiteminfo; i++)
		{
			if (ic->iteminfo[i].modelindex == modelindex)
			{
				break;
			} //end if
		} //end for
		//if the model is not from a known item
		if (i >= ic->numiteminfo) continue;
		//allocate a new level item
		li = AllocLevelItem();
		//
		if (!li) continue;
		//entity number of the level item
		li->entitynum = ent;
		//number for the level item
		li->number = numlevelitems + ent;
		//set the item info index for the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy(entinfo.origin, li->origin);
		//get the item goal area and goal origin
		li->goalareanum = trap_AAS_BestReachableArea(li->origin,
									ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
									li->goalorigin);
		//never go for items dropped into jumppads
		if (trap_AAS_AreaJumpPad(li->goalareanum))
		{
			FreeLevelItem(li);
			continue;
		} //end if
		//time this item out after 30 seconds
		//dropped items disappear after 30 seconds
		li->timeout = trap_AAS_Time() + 30;
		//add the level item to the list
		AddLevelItemToList(li);
		//BotAI_Print(PRT_MESSAGE, "found new level item %s\n", ic->iteminfo[i].classname);
	} //end for
	/*
	for (li = levelitems; li; li = li->next)
	{
		if (!li->entitynum)
		{
			BotFindEntityForLevelItem(li);
		} //end if
	} //end for*/
} //end of the function BotUpdateEntityItems
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpGoalStack(int goalstate)
{
	int i;
	bot_goalstate_t *gs;
	char name[32];

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	for (i = 1; i <= gs->goalstacktop; i++)
	{
		BotGoalName(gs->goalstack[i].number, name, 32);
		BotAI_Print(PRT_MESSAGE, "%d: %s", i, name);
	} //end for
} //end of the function BotDumpGoalStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotPushGoal(int goalstate, bot_goal_t *goal)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	if (gs->goalstacktop >= MAX_GOALSTACK-1)
	{
		BotAI_Print(PRT_ERROR, "goal heap overflow\n");
		BotDumpGoalStack(goalstate);
		return;
	} //end if
	gs->goalstacktop++;
	Com_Memcpy(&gs->goalstack[gs->goalstacktop], goal, sizeof(bot_goal_t));
} //end of the function BotPushGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotPopGoal(int goalstate)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	if (gs->goalstacktop > 0) gs->goalstacktop--;
} //end of the function BotPopGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotEmptyGoalStack(int goalstate)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	gs->goalstacktop = 0;
} //end of the function BotEmptyGoalStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotGetTopGoal(int goalstate, bot_goal_t *goal)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return qfalse;
	if (!gs->goalstacktop) return qfalse;
	Com_Memcpy(goal, &gs->goalstack[gs->goalstacktop], sizeof(bot_goal_t));
	return qtrue;
} //end of the function BotGetTopGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotGetSecondGoal(int goalstate, bot_goal_t *goal)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return qfalse;
	if (gs->goalstacktop <= 1) return qfalse;
	Com_Memcpy(goal, &gs->goalstack[gs->goalstacktop-1], sizeof(bot_goal_t));
	return qtrue;
} //end of the function BotGetSecondGoal
//===========================================================================
// pops a new long term goal on the goal stack in the goalstate
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags)
{
	int areanum, t, weightnum;
	float weight, bestweight, avoidtime;
	iteminfo_t *iteminfo;
	itemconfig_t *ic;
	levelitem_t *li, *bestitem;
	bot_goal_t goal;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
		return qfalse;
	if (!gs->itemweightconfig)
		return qfalse;
	//get the area the bot is in
	areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	if (!areanum || !trap_AAS_AreaReachability(areanum))
	{
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	} //end if
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if (!areanum)
		return qfalse;
	//the item configuration
	ic = itemconfig;
	if (!itemconfig)
		return qfalse;
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	Com_Memset(&goal, 0, sizeof(bot_goal_t));
	//go through the items in the level
	for (li = levelitems; li; li = li->next)
	{
		if (g_gametype.integer == GT_SINGLE_PLAYER) {
			if (li->flags & IFL_NOTSINGLE)
				continue;
		}
		if (g_gametype.integer >= GT_TEAM) {
			if (li->flags & IFL_NOTTEAM)
				continue;
		}
		else {
			if (li->flags & IFL_NOTFREE)
				continue;
		}
		if (li->flags & IFL_NOTBOT)
			continue;
		//if the item is not in a possible goal area
		if (!li->goalareanum)
			continue;
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if (!li->entitynum && !(li->flags & IFL_ROAM))
			continue;
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if (weightnum < 0)
			continue;

#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided(inventory, gs->itemweightconfig, weightnum);
#else
		weight = FuzzyWeight(inventory, gs->itemweightconfig, weightnum);
#endif //UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT
		//HACK: to make dropped items more attractive
		if (li->timeout)
			weight += bot_droppedweight.value;
#endif //DROPPEDWEIGHT
		//use weight scale for item_botroam
		if (li->flags & IFL_ROAM) weight *= li->weight;
		//
		if (weight > 0)
		{
			//get the travel time towards the goal area
			t = trap_AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			//if the goal is reachable
			if (t > 0)
			{
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime(goalstate, li->number);
				if (avoidtime - t * 0.009 > 0)
					continue;
				//
				weight /= (float) t * TRAVELTIME_SCALE;
				//
				if (weight > bestweight)
				{
					bestweight = weight;
					bestitem = li;
				} //end if
			} //end if
		} //end if
	} //end for
	//if no goal item found
	if (!bestitem)
	{
		/*
		//if not in lava or slime
		if (!AAS_AreaLava(areanum) && !AAS_AreaSlime(areanum))
		{
			if (AAS_RandomGoalArea(areanum, travelflags, &goal.areanum, goal.origin))
			{
				VectorSet(goal.mins, -15, -15, -15);
				VectorSet(goal.maxs, 15, 15, 15);
				goal.entitynum = 0;
				goal.number = 0;
				goal.flags = GFL_ROAM;
				goal.iteminfo = 0;
				//push the goal on the stack
				BotPushGoal(gs, &goal);
				//
#ifdef DEBUG
				BotAI_Print(PRT_MESSAGE, "chosen roam goal area %d\n", goal.areanum);
#endif //DEBUG
				return qtrue;
			} //end if
		} //end if
		*/
		return qfalse;
	} //end if
	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy(bestitem->goalorigin, goal.origin);
	VectorCopy(iteminfo->mins, goal.mins);
	VectorCopy(iteminfo->maxs, goal.maxs);
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if (bestitem->timeout)
		goal.flags |= GFL_DROPPED;
	if (bestitem->flags & IFL_ROAM)
		goal.flags |= GFL_ROAM;
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if (bestitem->timeout)
	{
		avoidtime = AVOID_DROPPED_TIME;
	} //end if
	else
	{
		avoidtime = iteminfo->respawntime;
		if (!avoidtime)
			avoidtime = AVOID_DEFAULT_TIME;
		if (avoidtime < AVOID_MINIMUM_TIME)
			avoidtime = AVOID_MINIMUM_TIME;
	} //end else
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals(gs, bestitem->number, avoidtime);
	//push the goal on the stack
	BotPushGoal(goalstate, &goal);
	//
	return qtrue;
} //end of the function BotChooseLTGItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags,
														bot_goal_t *ltg, float maxtime)
{
	int areanum, t, weightnum, ltg_time;
	float weight, bestweight, avoidtime;
	iteminfo_t *iteminfo;
	itemconfig_t *ic;
	levelitem_t *li, *bestitem;
	bot_goal_t goal;
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
		return qfalse;
	if (!gs->itemweightconfig)
		return qfalse;
	//get the area the bot is in
	areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	if (!areanum || !trap_AAS_AreaReachability(areanum))
	{
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	} //end if
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if (!areanum)
		return qfalse;
	//
	if (ltg) ltg_time = trap_AAS_AreaTravelTimeToGoalArea(areanum, origin, ltg->areanum, travelflags);
	else ltg_time = 99999;
	//the item configuration
	ic = itemconfig;
	if (!itemconfig)
		return qfalse;
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	Com_Memset(&goal, 0, sizeof(bot_goal_t));
	//go through the items in the level
	for (li = levelitems; li; li = li->next)
	{
		if (g_gametype.integer == GT_SINGLE_PLAYER) {
			if (li->flags & IFL_NOTSINGLE)
				continue;
		}
		if (g_gametype.integer >= GT_TEAM) {
			if (li->flags & IFL_NOTTEAM)
				continue;
		}
		else {
			if (li->flags & IFL_NOTFREE)
				continue;
		}
		if (li->flags & IFL_NOTBOT)
			continue;
		//if the item is in a possible goal area
		if (!li->goalareanum)
			continue;
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if (!li->entitynum && !(li->flags & IFL_ROAM))
			continue;
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if (weightnum < 0)
			continue;
		//
#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided(inventory, gs->itemweightconfig, weightnum);
#else
		weight = FuzzyWeight(inventory, gs->itemweightconfig, weightnum);
#endif //UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT
		//HACK: to make dropped items more attractive
		if (li->timeout)
			weight += bot_droppedweight.value;
#endif //DROPPEDWEIGHT
		//use weight scale for item_botroam
		if (li->flags & IFL_ROAM) weight *= li->weight;
		//
		if (weight > 0)
		{
			//get the travel time towards the goal area
			t = trap_AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			//if the goal is reachable
			if (t > 0 && t < maxtime)
			{
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime(goalstate, li->number);
				if (avoidtime - t * 0.009 > 0)
					continue;
				//
				weight /= (float) t * TRAVELTIME_SCALE;
				//
				if (weight > bestweight)
				{
					t = 0;
					if (ltg && !li->timeout)
					{
						//get the travel time from the goal to the long term goal
						t = trap_AAS_AreaTravelTimeToGoalArea(li->goalareanum, li->goalorigin, ltg->areanum, travelflags);
					} //end if
					//if the travel back is possible and doesn't take too long
					if (t <= ltg_time)
					{
						bestweight = weight;
						bestitem = li;
					} //end if
				} //end if
			} //end if
		} //end if
	} //end for
	//if no goal item found
	if (!bestitem)
		return qfalse;
	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy(bestitem->goalorigin, goal.origin);
	VectorCopy(iteminfo->mins, goal.mins);
	VectorCopy(iteminfo->maxs, goal.maxs);
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if (bestitem->timeout)
		goal.flags |= GFL_DROPPED;
	if (bestitem->flags & IFL_ROAM)
		goal.flags |= GFL_ROAM;
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if (bestitem->timeout)
	{
		avoidtime = AVOID_DROPPED_TIME;
	} //end if
	else
	{
		avoidtime = iteminfo->respawntime;
		if (!avoidtime)
			avoidtime = AVOID_DEFAULT_TIME;
		if (avoidtime < AVOID_MINIMUM_TIME)
			avoidtime = AVOID_MINIMUM_TIME;
	} //end else
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals(gs, bestitem->number, avoidtime);
	//push the goal on the stack
	BotPushGoal(goalstate, &goal);
	//
	return qtrue;
} //end of the function BotChooseNBGItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotTouchingGoal(vec3_t origin, bot_goal_t *goal)
{
	int i;
	vec3_t boxmins, boxmaxs;
	vec3_t absmins, absmaxs;
	vec3_t safety_maxs = {0, 0, 0}; //{4, 4, 10};
	vec3_t safety_mins = {0, 0, 0}; //{-4, -4, 0};

	trap_AAS_PresenceTypeBoundingBox(PRESENCE_NORMAL, boxmins, boxmaxs);
	VectorSubtract(goal->mins, boxmaxs, absmins);
	VectorSubtract(goal->maxs, boxmins, absmaxs);
	VectorAdd(absmins, goal->origin, absmins);
	VectorAdd(absmaxs, goal->origin, absmaxs);
	//make the box a little smaller for safety
	VectorSubtract(absmaxs, safety_maxs, absmaxs);
	VectorSubtract(absmins, safety_mins, absmins);

	for (i = 0; i < 3; i++)
	{
		if (origin[i] < absmins[i] || origin[i] > absmaxs[i]) return qfalse;
	} //end for
	return qtrue;
} //end of the function BotTouchingGoal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t *goal)
{
	aas_entityinfo_t entinfo;
	trace_t trace;
	vec3_t middle;

	if (!(goal->flags & GFL_ITEM)) return qfalse;
	//
	VectorAdd(goal->mins, goal->mins, middle);
	VectorScale(middle, 0.5, middle);
	VectorAdd(goal->origin, middle, middle);
	//
	trap_Trace(&trace, eye, NULL, NULL, middle, viewer, CONTENTS_SOLID);
	//if the goal middle point is visible
	if (trace.fraction >= 1)
	{
		//the goal entity number doesn't have to be valid
		//just assume it's valid
		if (goal->entitynum <= 0)
			return qfalse;
		//
		//if the entity data isn't valid
		BotEntityInfo(goal->entitynum, &entinfo);
		//NOTE: for some wacko reason entities are sometimes
		// not updated
		//if (!entinfo.valid) return qtrue;
		if (entinfo.ltime < trap_AAS_Time() - 0.5)
			return qtrue;
	} //end if
	return qfalse;
} //end of the function BotItemGoalInVisButNotVisible
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetGoalState(int goalstate)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	Com_Memset(gs->goalstack, 0, MAX_GOALSTACK * sizeof(bot_goal_t));
	gs->goalstacktop = 0;
	BotResetAvoidGoals(goalstate);
} //end of the function BotResetGoalState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadItemWeights(int goalstate, char *filename)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return BLERR_CANNOTLOADITEMWEIGHTS;
	//load the weight configuration
	gs->itemweightconfig = ReadWeightConfig(filename);
	if (!gs->itemweightconfig)
	{
		BotAI_Print(PRT_FATAL, "couldn't load weights\n");
		return BLERR_CANNOTLOADITEMWEIGHTS;
	} //end if
	//if there's no item configuration
	if (!itemconfig) return BLERR_CANNOTLOADITEMWEIGHTS;
	//create the item weight index
	ItemWeightIndex(itemconfig, gs->itemweightconfig, gs->itemweightindex);
	//everything went ok
	return BLERR_NOERROR;
} //end of the function BotLoadItemWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeItemWeights(int goalstate)
{
	bot_goalstate_t *gs;

	gs = BotGoalStateFromHandle(goalstate);
	if (!gs) return;
	if (gs->itemweightconfig) FreeWeightConfig(gs->itemweightconfig);
} //end of the function BotFreeItemWeights
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotAllocGoalState(int client)
{
	botgoalstates[client+1].client = client;
	return client+1;
} //end of the function BotAllocGoalState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeGoalState(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		BotAI_Print(PRT_FATAL, "goal state handle %d out of range\n", handle);
		return;
	} //end if
	BotFreeItemWeights(handle);
} //end of the function BotFreeGoalState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupGoalAI(void)
{
	//load the item configuration
	itemconfig = LoadItemConfig("items.c");
	if (!itemconfig)
	{
		BotAI_Print(PRT_FATAL, "couldn't load item config\n");
		return BLERR_CANNOTLOADITEMCONFIG;
	} //end if

	//everything went ok
	return BLERR_NOERROR;
} //end of the function BotSetupGoalAI
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotShutdownGoalAI(void)
{
	itemconfig = NULL;
	levelitemheap = NULL;

	freelevelitems = NULL;
	levelitems = NULL;
	numlevelitems = 0;
} //end of the function BotShutdownGoalAI
