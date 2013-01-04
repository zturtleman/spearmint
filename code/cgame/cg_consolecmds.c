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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
#endif
#ifdef MISSIONPACK_HUD
extern menuDef_t *menuScoreboard;
#endif



void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer(0);
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_Cmd_ExecuteText(EXEC_NOW, va( "gc %i %i", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}

/*
=============
CG_ScoresDown
=============
*/
static void CG_ScoresDown(int localClientNum) {
	cglc_t *lc = &cg.localClients[localClientNum];

#ifdef MISSIONPACK_HUD
	CG_BuildSpectatorString();
#endif
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !CG_AnyScoreboardShowing() ) {
			cg.numScores = 0;
		}

		lc->showScores = qtrue;
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		lc->showScores = qtrue;
	}
}

/*
=============
CG_ScoresUp
=============
*/
static void CG_ScoresUp( int localClientNum ) {
	cglc_t *lc = &cg.localClients[localClientNum];

	if ( lc->showScores ) {
		lc->showScores = qfalse;
		lc->scoreFadeTime = cg.time;
	}
}

/*
=============
CG_ScoresDown_f
=============
*/
void CG_ScoresDown_f(void) {
	CG_ScoresDown(0);
}

/*
=============
CG_ScoresUp_f
=============
*/
void CG_ScoresUp_f(void) {
	CG_ScoresUp(0);
}

/*
=============
CG_2ScoresDown_f
=============
*/
void CG_2ScoresDown_f(void) {
	CG_ScoresDown(1);
}

/*
=============
CG_2ScoresUp_f
=============
*/
void CG_2ScoresUp_f(void) {
	CG_ScoresUp(1);
}

/*
=============
CG_3ScoresDown_f
=============
*/
void CG_3ScoresDown_f(void) {
	CG_ScoresDown(2);
}

/*
=============
CG_3ScoresUp_f
=============
*/
void CG_3ScoresUp_f(void) {
	CG_ScoresUp(2);
}

/*
=============
CG_4ScoresDown_f
=============
*/
void CG_4ScoresDown_f(void) {
	CG_ScoresDown(3);
}

/*
=============
CG_4ScoresUp_f
=============
*/
void CG_4ScoresUp_f(void) {
	CG_ScoresUp(3);
}

#ifdef MISSIONPACK_HUD
extern menuDef_t *menuScoreboard;
void Menu_Reset( void );			// FIXME: add to right include file

static void CG_LoadHud_f( void) {
  char buff[1024];
	const char *hudSet;
  memset(buff, 0, sizeof(buff));

	String_Init();
	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
  menuScoreboard = NULL;
}


static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && CG_AnyScoreboardShowing()) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && CG_AnyScoreboardShowing()) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}
#endif


#ifdef MISSIONPACK
static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_GlobalCenterPrint("YOU WIN!", SCREEN_HEIGHT/2, 2.0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_GlobalCenterPrint("YOU LOSE...", SCREEN_HEIGHT/2, 2.0);
}

#endif

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer(0);
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker(0);
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

#ifdef MISSIONPACK
static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer(0);
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker(0);
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	int clientNum = cg.snap->pss[0].clientNum;
	int team = cg.snap->pss[0].persistant[PERS_TEAM];
	clientInfo_t *ci = cgs.clientinfo + clientNum;

	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[team][cg_currentSelectedPlayer.integer] != clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_Cmd_ExecuteText(EXEC_NOW, "+button5; wait; -button5");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_Cmd_ExecuteText(EXEC_NOW, "+button6; wait; -button6");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, "cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, "cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, "cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, "cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_Cmd_ExecuteText(EXEC_NOW, "cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer(0);
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/

#endif

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/


void CG_GenerateTracemap(void)
{
	bgGenTracemap_t gen;

	if ( !cg.mapcoordsValid ) {
		CG_Printf( "Need valid mapcoords in the worldspawn to be able to generate a tracemap.\n" );
		return;
	}

	gen.trace = CG_Trace;
	gen.pointcontents = CG_PointContents;

	BG_GenerateTracemap(cgs.mapname, cg.mapcoordsMins, cg.mapcoordsMaxs, &gen);
}

typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+2scores", CG_2ScoresDown_f },
	{ "-2scores", CG_2ScoresUp_f },
	{ "+3scores", CG_3ScoresDown_f },
	{ "-3scores", CG_3ScoresUp_f },
	{ "+4scores", CG_4ScoresDown_f },
	{ "-4scores", CG_4ScoresUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "+2zoom", CG_2ZoomDown_f },
	{ "-2zoom", CG_2ZoomUp_f },
	{ "+3zoom", CG_3ZoomDown_f },
	{ "-3zoom", CG_3ZoomUp_f },
	{ "+4zoom", CG_4ZoomDown_f },
	{ "-4zoom", CG_4ZoomUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "2weapnext", CG_2NextWeapon_f },
	{ "2weapprev", CG_2PrevWeapon_f },
	{ "2weapon", CG_2Weapon_f },
	{ "3weapnext", CG_3NextWeapon_f },
	{ "3weapprev", CG_3PrevWeapon_f },
	{ "3weapon", CG_3Weapon_f },
	{ "4weapnext", CG_4NextWeapon_f },
	{ "4weapprev", CG_4PrevWeapon_f },
	{ "4weapon", CG_4Weapon_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
#ifdef MISSIONPACK
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
#ifdef MISSIONPACK_HUD
	{ "loadhud", CG_LoadHud_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
#endif
#endif
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "generateTracemap", CG_GenerateTracemap }
};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		if (Com_LocalClientForCvarName(commands[i].cmd) >= CG_MaxSplitView()) {
			continue;
		}

		if ( !Q_stricmp( cmd, commands[i].cmd )) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		if (Com_LocalClientForCvarName(commands[i].cmd) >= CG_MaxSplitView()) {
			continue;
		}

		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	for (i = 0; i < CG_MaxSplitView(); i++) {
		trap_AddCommand(Com_LocalClientCvarName(i, "say"));
		trap_AddCommand(Com_LocalClientCvarName(i, "say_team"));
		trap_AddCommand(Com_LocalClientCvarName(i, "tell"));
#ifdef MISSIONPACK
		trap_AddCommand(Com_LocalClientCvarName(i, "vsay"));
		trap_AddCommand(Com_LocalClientCvarName(i, "vsay_team"));
		trap_AddCommand(Com_LocalClientCvarName(i, "vtell"));
		trap_AddCommand(Com_LocalClientCvarName(i, "vosay"));
		trap_AddCommand(Com_LocalClientCvarName(i, "vosay_team"));
		trap_AddCommand(Com_LocalClientCvarName(i, "votell"));
		trap_AddCommand(Com_LocalClientCvarName(i, "vtaunt"));
#endif
		trap_AddCommand(Com_LocalClientCvarName(i, "give"));
		trap_AddCommand(Com_LocalClientCvarName(i, "god"));
		trap_AddCommand(Com_LocalClientCvarName(i, "notarget"));
		trap_AddCommand(Com_LocalClientCvarName(i, "noclip"));
		trap_AddCommand(Com_LocalClientCvarName(i, "where"));
		trap_AddCommand(Com_LocalClientCvarName(i, "kill"));
		trap_AddCommand(Com_LocalClientCvarName(i, "teamtask"));
		trap_AddCommand(Com_LocalClientCvarName(i, "levelshot"));
		trap_AddCommand(Com_LocalClientCvarName(i, "follow"));
		trap_AddCommand(Com_LocalClientCvarName(i, "follownext"));
		trap_AddCommand(Com_LocalClientCvarName(i, "followprev"));
		trap_AddCommand(Com_LocalClientCvarName(i, "team"));
		trap_AddCommand(Com_LocalClientCvarName(i, "callvote"));
		trap_AddCommand(Com_LocalClientCvarName(i, "vote"));
		trap_AddCommand(Com_LocalClientCvarName(i, "callteamvote"));
		trap_AddCommand(Com_LocalClientCvarName(i, "teamvote"));
		trap_AddCommand(Com_LocalClientCvarName(i, "setviewpos"));
		trap_AddCommand(Com_LocalClientCvarName(i, "stats"));
	}

	trap_AddCommand ("addbot");
}
