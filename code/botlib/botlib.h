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
/*****************************************************************************
 * name:		botlib.h
 *
 * desc:		bot AI library
 *
 * $Archive: /source/code/game/botai.h $
 *
 *****************************************************************************/

#define	BOTLIB_API_VERSION		3

struct aas_clientmove_s;
struct aas_areainfo_s;
struct aas_trace_s;
struct aas_altroutegoal_s;
struct aas_reachability_s;
struct aas_predictroute_s;

//debug line colors
#define LINECOLOR_NONE			0
#define LINECOLOR_RED			1
#define LINECOLOR_GREEN			2
#define LINECOLOR_YELLOW		3
#define LINECOLOR_BLUE			4
#define LINECOLOR_MAGENTA		5
#define LINECOLOR_CYAN			6
#define LINECOLOR_WHITE			7

//Print types
#define PRT_DEVELOPER			0
#define PRT_MESSAGE				1
#define PRT_WARNING				2
#define PRT_ERROR				3
#define PRT_FATAL				4
#define PRT_EXIT				5

//botlib error codes
#define BLERR_NOERROR					0	//no error
#define BLERR_LIBRARYNOTSETUP			1	//library not setup
#define BLERR_INVALIDENTITYNUMBER		2	//invalid entity number
#define BLERR_NOAASFILE					3	//no AAS file available
#define BLERR_CANNOTOPENAASFILE			4	//cannot open AAS file
#define BLERR_WRONGAASFILEID			5	//incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION		6	//incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP			7	//cannot read AAS file lump
#define BLERR_CANNOTLOADICHAT			8	//cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS		9	//cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG		10	//cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS	11	//cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG	12	//cannot load weapon config

typedef trace_t bsp_trace_t;

//entity state
typedef struct bot_entitystate_s
{
	int			type;		// entity type
	int			flags;		// entity flags
	vec3_t		origin;		// origin of the entity
	vec3_t		angles;		// angles of the model
	vec3_t		absmins;	// absolute bounding box minimums
	vec3_t		absmaxs;	// absolute bounding box maximums
	int			solid;		// solid type
	qboolean	relink;		// relink entity
} bot_entitystate_t;

//bot AI library exported functions
typedef struct botlib_import_s
{
	//get time for measuring time lapse
	int			(*MilliSeconds)(void);
	//print messages from the bot library
	void		(QDECL *Print)(int type, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
	//trace a bbox through the world
	void		(*Trace)(bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask);
	//trace a bbox against a specific entity
	void		(*EntityTrace)(bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask);
	//retrieve the contents at the given point
	int			(*PointContents)(vec3_t point);
	//check if the point is in potential visible sight
	int			(*inPVS)(vec3_t p1, vec3_t p2);
	//retrieve the BSP entity data lump
	qboolean	(*GetEntityToken)(int *offset, char *token, int tokenSize);
	//
	void		(*BSPModelMinsMaxsOrigin)(int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin);
	//send a bot client command
	void		(*BotClientCommand)(int playerNum, const char *command);
	//memory allocation
	void		*(*GetMemory)(int size);		// allocate from Zone
	void		(*FreeMemory)(void *ptr);		// free memory from Zone
	int			(*AvailableMemory)(void);		// available Zone memory
	void		*(*HunkAlloc)(int size);		// allocate from hunk
	//file system access
	int			(*FS_FOpenFile)( const char *qpath, fileHandle_t *file, fsMode_t mode );
	int			(*FS_Read)( void *buffer, int len, fileHandle_t f );
	int			(*FS_Write)( const void *buffer, int len, fileHandle_t f );
	void		(*FS_FCloseFile)( fileHandle_t f );
	int			(*FS_Seek)( fileHandle_t f, long offset, int origin );
	//debug visualisation stuff
	int			(*DebugLineCreate)(void);
	void		(*DebugLineDelete)(int line);
	void		(*DebugLineShow)(int line, vec3_t start, vec3_t end, int color);
	//
	int			(*DebugPolygonCreate)(int color, int numPoints, vec3_t *points);
	void		(*DebugPolygonDelete)(int id);
} botlib_import_t;

typedef struct aas_export_s
{
	//-----------------------------------
	// be_aas_main.c
	//-----------------------------------
	int			(*AAS_Loaded)(void);
	int			(*AAS_Initialized)(void);
	void		(*AAS_PresenceTypeBoundingBox)(int presencetype, vec3_t mins, vec3_t maxs);
	float		(*AAS_Time)(void);
	//-----------------------------------
	// be_aas_debug.c
	//-----------------------------------
	void		(*AAS_ClearShownDebugLines)(void);
	void		(*AAS_ClearShownPolygons)(void);
	void		(*AAS_DebugLine)(vec3_t start, vec3_t end, int color);
	void		(*AAS_PermanentLine)(vec3_t start, vec3_t end, int color);
	void		(*AAS_DrawPermanentCross)(vec3_t origin, float size, int color);
	void		(*AAS_DrawPlaneCross)(vec3_t point, vec3_t normal, float dist, int type, int color);
	void		(*AAS_ShowBoundingBox)(vec3_t origin, vec3_t mins, vec3_t maxs);
	void		(*AAS_ShowFace)(int facenum);
	void		(*AAS_ShowArea)(int areanum, int groundfacesonly);
	void		(*AAS_ShowAreaPolygons)(int areanum, int color, int groundfacesonly);
	void		(*AAS_DrawCross)(vec3_t origin, float size, int color);
	void		(*AAS_PrintTravelType)(int traveltype);
	void		(*AAS_DrawArrow)(vec3_t start, vec3_t end, int linecolor, int arrowcolor);
	void		(*AAS_ShowReachability)(struct aas_reachability_s *reach, int contentmask);
	void		(*AAS_ShowReachableAreas)(int areanum, int contentmask);
	void		(*AAS_FloodAreas)(vec3_t origin);
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	int			(*AAS_PointAreaNum)(vec3_t point);
	int			(*AAS_PointReachabilityAreaIndex)( vec3_t point );
	void		(*AAS_TracePlayerBBox)(struct aas_trace_s *trace, vec3_t start, vec3_t end, int presencetype, int passent, int contentmask);
	int			(*AAS_TraceAreas)(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);
	int			(*AAS_BBoxAreas)(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
	int			(*AAS_AreaInfo)( int areanum, struct aas_areainfo_s *info );
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	int			(*AAS_PointContents)(vec3_t point);
	int			(*AAS_NextBSPEntity)(int ent);
	int			(*AAS_ValueForBSPEpairKey)(int ent, char *key, char *value, int size);
	int			(*AAS_VectorForBSPEpairKey)(int ent, char *key, vec3_t v);
	int			(*AAS_FloatForBSPEpairKey)(int ent, char *key, float *value);
	int			(*AAS_IntForBSPEpairKey)(int ent, char *key, int *value);
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	int			(*AAS_AreaReachability)(int areanum);
	int			(*AAS_BestReachableArea)(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin);
	int			(*AAS_BestReachableFromJumpPadArea)(vec3_t origin, vec3_t mins, vec3_t maxs);
	int			(*AAS_NextModelReachability)(int num, int modelnum);
	float		(*AAS_AreaGroundFaceArea)(int areanum);
	int			(*AAS_AreaCrouch)(int areanum);
	int			(*AAS_AreaSwim)(int areanum);
	int			(*AAS_AreaLiquid)(int areanum);
	int			(*AAS_AreaLava)(int areanum);
	int			(*AAS_AreaSlime)(int areanum);
	int			(*AAS_AreaGrounded)(int areanum);
	int			(*AAS_AreaLadder)(int areanum);
	int			(*AAS_AreaJumpPad)(int areanum);
	int			(*AAS_AreaDoNotEnter)(int areanum);
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	int			(*AAS_TravelFlagForType)(int traveltype);
	int			(*AAS_AreaContentsTravelFlags)(int areanum);
	int			(*AAS_NextAreaReachability)(int areanum, int reachnum);
	void		(*AAS_ReachabilityFromNum)(int num, struct aas_reachability_s *reach);
	int			(*AAS_RandomGoalArea)(int areanum, int travelflags, int contentmask, int *goalareanum, vec3_t goalorigin);
	int			(*AAS_EnableRoutingArea)(int areanum, int enable);
	unsigned short int (*AAS_AreaTravelTime)(int areanum, vec3_t start, vec3_t end);
	int			(*AAS_AreaTravelTimeToGoalArea)(int areanum, vec3_t origin, int goalareanum, int travelflags);
	int			(*AAS_PredictRoute)(struct aas_predictroute_s *route, int areanum, vec3_t origin,
							int goalareanum, int travelflags, int maxareas, int maxtime,
							int stopevent, int stopcontents, int stoptfl, int stopareanum);
	//--------------------------------------------
	// be_aas_altroute.c
	//--------------------------------------------
	int			(*AAS_AlternativeRouteGoals)(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										struct aas_altroutegoal_s *altroutegoals, int maxaltroutegoals,
										int type);
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	int			(*AAS_PredictPlayerMovement)(struct aas_clientmove_s *move,
											int entnum, vec3_t origin,
											int presencetype, int onground,
											vec3_t velocity, vec3_t cmdmove,
											int cmdframes,
											int maxframes, float frametime,
											int stopevent, int stopareanum, int visualize, int contentmask);
	int			(*AAS_OnGround)(vec3_t origin, int presencetype, int passent, int contentmask);
	int			(*AAS_Swimming)(vec3_t origin);
	void		(*AAS_JumpReachRunStart)(struct aas_reachability_s *reach, vec3_t runstart, int contentmask);
	int			(*AAS_AgainstLadder)(vec3_t origin);
	int			(*AAS_HorizontalVelocityForJump)(float zvel, vec3_t start, vec3_t end, float *velocity);
	int			(*AAS_DropToFloor)(vec3_t origin, vec3_t mins, vec3_t maxs, int passent, int contentmask);
} aas_export_t;

//bot AI library imported functions
typedef struct botlib_export_s
{
	//Area Awareness System functions
	aas_export_t aas;
	//setup the bot library, returns BLERR_
	int (*BotLibSetup)(void);
	//shutdown the bot library, returns BLERR_
	int (*BotLibShutdown)(void);
	//sets a library variable returns BLERR_
	int (*BotLibVarSet)(const char *var_name, const char *value);
	//gets a library variable returns BLERR_
	int (*BotLibVarGet)(const char *var_name, char *value, int size);

	//start a frame in the bot library
	int (*BotLibStartFrame)(float time);
	//load a new map in the bot library
	int (*BotLibLoadMap)(const char *mapname);
	//entity updates
	int (*BotLibUpdateEntity)(int ent, bot_entitystate_t *state);
} botlib_export_t;

//linking of bot library
botlib_export_t *GetBotLibAPI( int apiVersion, botlib_import_t *import );

/* Library variables:

name:						default:			module(s):			description:

"basedir"					""					-					base directory
"homedir"					""					be_interface.c		home directory
"gamedir"					""					be_interface.c		game directory

"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"4"					be_interface.c		maximum number of clients
"maxentities"				"1024"				be_interface.c		maximum number of entities
"bot_developer"				"0"					be_interface.c		bot developer mode (it's "botDeveloper" in C to prevent symbol clash).

"phys_friction"				"6"					be_aas_move.c		ground friction
"phys_stopspeed"			"100"				be_aas_move.c		stop speed
"phys_gravity"				"800"				be_aas_move.c		gravity value
"phys_waterfriction"		"1"					be_aas_move.c		water friction
"phys_watergravity"			"400"				be_aas_move.c		gravity in water
"phys_maxvelocity"			"320"				be_aas_move.c		maximum velocity
"phys_maxwalkvelocity"		"320"				be_aas_move.c		maximum walk velocity
"phys_maxcrouchvelocity"	"100"				be_aas_move.c		maximum crouch velocity
"phys_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"phys_walkaccelerate"		"10"				be_aas_move.c		walk acceleration
"phys_airaccelerate"		"1"					be_aas_move.c		air acceleration
"phys_swimaccelerate"		"4"					be_aas_move.c		swim acceleration
"phys_maxstep"				"18"				be_aas_move.c		maximum step height
"phys_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"phys_maxbarrier"			"32"				be_aas_move.c		maximum barrier height
"phys_maxwaterjump"			"19"				be_aas_move.c		maximum waterjump height
"phys_jumpvel"				"270"				be_aas_move.c		jump z velocity
"phys_falldelta5"			"40"				be_aas_move.c
"phys_falldelta10"			"60"				be_aas_move.c
"phys_strafejumping"		"1"					be_aas_move.c		use strafe jump maxspeed bug
"rs_waterjump"				"400"				be_aas_move.c
"rs_teleport"				"50"				be_aas_move.c
"rs_barrierjump"			"100"				be_aas_move.c
"rs_startcrouch"			"300"				be_aas_move.c
"rs_startgrapple"			"500"				be_aas_move.c
"rs_startwalkoffledge"		"70"				be_aas_move.c
"rs_startjump"				"300"				be_aas_move.c
"rs_rocketjump"				"500"				be_aas_move.c
"rs_bfgjump"				"500"				be_aas_move.c
"rs_jumppad"				"250"				be_aas_move.c
"rs_aircontrolledjumppad"	"300"				be_aas_move.c
"rs_funcbob"				"300"				be_aas_move.c
"rs_startelevator"			"50"				be_aas_move.c
"rs_falldamage5"			"300"				be_aas_move.c
"rs_falldamage10"			"500"				be_aas_move.c
"rs_maxjumpfallheight"		"450"				be_aas_move.c

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_routingcache"			"4096"				be_aas_route.c		maximum routing cache size in KB
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"aasoptimize"				"0"					be_aas_main.c		enable aas optimization
"sv_mapChecksum"			"0"					be_aas_main.c		BSP file checksum
"bot_visualizejumppads"		"0"					be_aas_reach.c		visualize jump pads

*/

