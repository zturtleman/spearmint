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



void CG_TargetCommand_f( int localPlayerNum ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer( localPlayerNum );
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_Cmd_ExecuteText(EXEC_NOW, va( "%s %i %i", Com_LocalClientCvarName( localPlayerNum, "gc" ), targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	// manually clamp here so cvar range warning isn't show
	trap_Cvar_SetValue("cg_viewsize", Com_Clamp( 30, 100, (int)(cg_viewsize.integer+10) ) );
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	// manually clamp here so cvar range warning isn't show
	trap_Cvar_SetValue("cg_viewsize", Com_Clamp( 30, 100, (int)(cg_viewsize.integer-10) ) );
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f( int localPlayerNum ) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.localClients[localPlayerNum].lastViewPos[0],
		(int)cg.localClients[localPlayerNum].lastViewPos[1],
		(int)cg.localClients[localPlayerNum].lastViewPos[2],
		(int)cg.localClients[localPlayerNum].lastViewAngles[YAW]);
}

/*
=============
CG_ScoresDown
=============
*/
static void CG_ScoresDown_f(int localClientNum) {
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
CG_ScoresUp_f
=============
*/
static void CG_ScoresUp_f( int localClientNum ) {
	cglc_t *lc = &cg.localClients[localClientNum];

	if ( lc->showScores ) {
		lc->showScores = qfalse;
		lc->scoreFadeTime = cg.time;
	}
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

static void CG_CameraOrbit( int speed, int delay ) {
	int i;

	trap_Cvar_SetValue( "cg_cameraOrbit", speed );
	if ( delay > 0 ) {
		trap_Cvar_SetValue( "cg_cameraOrbitDelay", delay );
	}

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		trap_Cvar_SetValue(Com_LocalClientCvarName( i, "cg_thirdPerson" ), speed == 0 ? 0 : 1 );
		trap_Cvar_SetValue(Com_LocalClientCvarName( i, "cg_thirdPersonAngle" ), 0 );
		trap_Cvar_SetValue(Com_LocalClientCvarName( i, "cg_thirdPersonRange" ), 100 );
	}
}

#ifdef MISSIONPACK
static void CG_spWin_f( void) {
	CG_CameraOrbit( 2, 35 );
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_GlobalCenterPrint("YOU WIN!", SCREEN_HEIGHT/2, 2.0);
}

static void CG_spLose_f( void) {
	CG_CameraOrbit( 2, 35 );
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_GlobalCenterPrint("YOU LOSE...", SCREEN_HEIGHT/2, 2.0);
}

#endif

static void CG_TellTarget_f( int localPlayerNum ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer( localPlayerNum );
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "%s %i %s", Com_LocalClientCvarName( localPlayerNum, "tell" ), clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( int localPlayerNum ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker( localPlayerNum );
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "%s %i %s", Com_LocalClientCvarName( localPlayerNum, "tell" ), clientNum, message );
	trap_SendClientCommand( command );
}

#ifdef MISSIONPACK
static void CG_VoiceTellTarget_f( int localPlayerNum ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer( localPlayerNum );
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "%s %i %s", Com_LocalClientCvarName( localPlayerNum, "vtell" ), clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( int localPlayerNum ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker( localPlayerNum );
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "%s %i %s", Com_LocalClientCvarName( localPlayerNum, "vtell" ), clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( int localPlayerNum ) {
  CG_SelectNextPlayer( localPlayerNum );
}

static void CG_PrevTeamMember_f( int localPlayerNum ) {
  CG_SelectPrevPlayer( localPlayerNum );
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( int localPlayerNum ) {
	cglc_t			*localClient;
	clientInfo_t	*ci;
	int				clientNum;
	int				team;

	localClient = &cg.localClients[ localPlayerNum ];

	if ( localClient->clientNum == -1 || cg.snap->lcIndex[localPlayerNum] == -1 ) {
		return;
	}

	clientNum = cg.snap->pss[ cg.snap->lcIndex[ localPlayerNum ] ].clientNum;
	team = cg.snap->pss[ cg.snap->lcIndex[ localPlayerNum ] ].persistant[PERS_TEAM];

	ci = cgs.clientinfo + clientNum;

	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[team][cg_currentSelectedPlayer[localPlayerNum].integer] != clientNum) {
			return;
		}
	}
	if (localClient->currentOrder < TEAMTASK_CAMP) {
		localClient->currentOrder++;

		if (localClient->currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				localClient->currentOrder++;
			}
		}

		if (localClient->currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				localClient->currentOrder++;
			}
		}

	} else {
		localClient->currentOrder = TEAMTASK_OFFENSE;
	}
	localClient->orderPending = qtrue;
	localClient->orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f( int localPlayerNum ) {
	cglc_t			*localClient;

	localClient = &cg.localClients[ localPlayerNum ];

	if ( localClient->clientNum == -1 ) {
		return;
	}

	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %d %s\n", Com_LocalClientCvarName(localPlayerNum, "vtell"), localClient->acceptLeader, VOICECHAT_YES));
	trap_Cmd_ExecuteText(EXEC_NOW, "+button5; wait; -button5");
	if (cg.time < localClient->acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", localClient->acceptTask));
		localClient->acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f( int localPlayerNum ) {
	cglc_t			*localClient;

	localClient = &cg.localClients[ localPlayerNum ];

	if ( localClient->clientNum == -1 ) {
		return;
	}

	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %d %s\n", Com_LocalClientCvarName(localPlayerNum, "vtell"), localClient->acceptLeader, VOICECHAT_NO));
	trap_Cmd_ExecuteText(EXEC_NOW, va("%s; wait; %s", Com_LocalClientCvarName(localPlayerNum, "+button6"), Com_LocalClientCvarName(localPlayerNum, "-button6")));
	if (cg.time < localClient->acceptOrderTime) {
		localClient->acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f( int localPlayerNum ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF) {
		trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONGETFLAG));
	} else {
		trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("%s %d\n", Com_LocalClientCvarName(localPlayerNum, "teamtask"), TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("%s %d\n", Com_LocalClientCvarName(localPlayerNum, "teamtask"), TEAMTASK_PATROL));
}

static void CG_TaskCamp_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("%s %d\n", Com_LocalClientCvarName(localPlayerNum, "teamtask"), TEAMTASK_CAMP));
}

static void CG_TaskFollow_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("%s %d\n", Com_LocalClientCvarName(localPlayerNum, "teamtask"), TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("%s %d\n", Com_LocalClientCvarName(localPlayerNum, "teamtask"), TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("%s %d\n", Com_LocalClientCvarName(localPlayerNum, "teamtask"), TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay_team"), VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay"), VOICECHAT_KILLINSULT));
}

static void CG_TauntPraise_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay"), VOICECHAT_PRAISE));
}

static void CG_TauntTaunt_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s\n", Com_LocalClientCvarName(localPlayerNum, "vtaunt")));
}

static void CG_TauntDeathInsult_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay"), VOICECHAT_DEATHINSULT));
}

static void CG_TauntGauntlet_f( int localPlayerNum ) {
	trap_Cmd_ExecuteText(EXEC_NOW, va("cmd %s %s\n", Com_LocalClientCvarName(localPlayerNum, "vsay"), VOICECHAT_KILLGAUNTLET));
}

static void CG_TaskSuicide_f( int localPlayerNum ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer(0);
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "%s %i suicide", Com_LocalClientCvarName( localPlayerNum, "tell" ), clientNum );
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
		CG_CameraOrbit( 0, -1 );
	} else {
		CG_CameraOrbit( 5, -1 );
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
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
#ifdef MISSIONPACK
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

typedef struct {
	char	*cmd;
	void	(*function)(int);
} playerConsoleCommand_t;

static playerConsoleCommand_t	playerCommands[] = {
	{ "+attack", IN_Button0Down },
	{ "-attack", IN_Button0Up },
	{ "+back",IN_BackDown },
	{ "-back",IN_BackUp },
	{ "+button0", IN_Button0Down },
	{ "-button0", IN_Button0Up },
	{ "+button10", IN_Button10Down },
	{ "-button10", IN_Button10Up },
	{ "+button11", IN_Button11Down },
	{ "-button11", IN_Button11Up },
	{ "+button12", IN_Button12Down },
	{ "-button12", IN_Button12Up },
	{ "+button13", IN_Button13Down },
	{ "-button13", IN_Button13Up },
	{ "+button14", IN_Button14Down },
	{ "-button14", IN_Button14Up },
	{ "+button1", IN_Button1Down },
	{ "-button1", IN_Button1Up },
	{ "+button2", IN_Button2Down },
	{ "-button2", IN_Button2Up },
	{ "+button3", IN_Button3Down },
	{ "-button3", IN_Button3Up },
	{ "+button4", IN_Button4Down },
	{ "-button4", IN_Button4Up },
	{ "+button5", IN_Button5Down },
	{ "-button5", IN_Button5Up },
	{ "+button6", IN_Button6Down },
	{ "-button6", IN_Button6Up },
	{ "+button7", IN_Button7Down },
	{ "-button7", IN_Button7Up },
	{ "+button8", IN_Button8Down },
	{ "-button8", IN_Button8Up },
	{ "+button9", IN_Button9Down },
	{ "-button9", IN_Button9Up },
	{ "+forward",IN_ForwardDown },
	{ "-forward",IN_ForwardUp },
	{ "+left",IN_LeftDown },
	{ "-left",IN_LeftUp },
	{ "+lookdown", IN_LookdownDown },
	{ "-lookdown", IN_LookdownUp },
	{ "+lookup", IN_LookupDown },
	{ "-lookup", IN_LookupUp },
	{ "+mlook", IN_MLookDown },
	{ "-mlook", IN_MLookUp },
	{ "+movedown",IN_DownDown },
	{ "-movedown",IN_DownUp },
	{ "+moveleft", IN_MoveleftDown },
	{ "-moveleft", IN_MoveleftUp },
	{ "+moveright", IN_MoverightDown },
	{ "-moveright", IN_MoverightUp },
	{ "+moveup",IN_UpDown },
	{ "-moveup",IN_UpUp },
	{ "+right",IN_RightDown },
	{ "-right",IN_RightUp },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+speed", IN_SpeedDown },
	{ "-speed", IN_SpeedUp },
	{ "+strafe", IN_StrafeDown },
	{ "-strafe", IN_StrafeUp },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "centerview", IN_CenterView },
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
#endif
	{ "viewpos", CG_Viewpos_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f }
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
	int		localPlayerNum;
	const char	*baseCmd;

	cmd = CG_Argv(0);

	localPlayerNum = Com_LocalClientForCvarName( cmd );
	baseCmd = Com_LocalClientBaseCvarName( cmd );

	for ( i = 0 ; i < ARRAY_LEN( playerCommands ) ; i++ ) {
		if ( !Q_stricmp( baseCmd, playerCommands[i].cmd )) {
			playerCommands[i].function( localPlayerNum );
			return qtrue;
		}
	}

	if ( localPlayerNum != 0 )
		return qfalse;

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
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
	int		i, j;

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	for ( i = 0 ; i < ARRAY_LEN( playerCommands ) ; i++ ) {
		for ( j = 0; j < CG_MaxSplitView(); j++ ) {
			trap_AddCommand( Com_LocalClientCvarName( j, playerCommands[i].cmd ) );
		}
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
