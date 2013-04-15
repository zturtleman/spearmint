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
 * name:		ai_ea.c
 *
 * desc:		elementary actions
 *
 * $Archive: /MissionPack/code/game/ai_ea.c $
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

#define MAX_USERMOVE				400
#define MAX_COMMANDARGUMENTS		10

bot_input_t botinputs[MAX_CLIENTS];

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Say(int client, char *str)
{
	trap_ClientCommand(client, va("say %s", str));
} //end of the function EA_Say
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_SayTeam(int client, char *str)
{
	trap_ClientCommand(client, va("say_team %s", str));
} //end of the function EA_SayTeam
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Tell(int client, int clientto, char *str)
{
	trap_ClientCommand(client, va("tell %d, %s", clientto, str));
} //end of the function EA_SayTeam
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseItem(int client, char *it)
{
	trap_ClientCommand(client, va("use %s", it));
} //end of the function EA_UseItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_DropItem(int client, char *it)
{
	trap_ClientCommand(client, va("drop %s", it));
} //end of the function EA_DropItem
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_UseInv(int client, char *inv)
{
	trap_ClientCommand(client, va("invuse %s", inv));
} //end of the function EA_UseInv
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DropInv(int client, char *inv)
{
	trap_ClientCommand(client, va("invdrop %s", inv));
} //end of the function EA_DropInv
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Gesture(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_GESTURE;
} //end of the function EA_Gesture
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void EA_Command(int client, char *command)
{
	trap_ClientCommand(client, command);
} //end of the function EA_Command
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_SelectWeapon(int client, int weapon)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->weapon = weapon;
} //end of the function EA_SelectWeapon
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Attack(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_ATTACK;
} //end of the function EA_Attack
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Talk(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_TALK;
} //end of the function EA_Talk
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Use(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_USE;
} //end of the function EA_Use
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Respawn(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_RESPAWN;
} //end of the function EA_Respawn
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Jump(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	if (bi->actionflags & ACTION_JUMPEDLASTFRAME)
	{
		bi->actionflags &= ~ACTION_JUMP;
	} //end if
	else
	{
		bi->actionflags |= ACTION_JUMP;
	} //end if
} //end of the function EA_Jump
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_DelayedJump(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	if (bi->actionflags & ACTION_JUMPEDLASTFRAME)
	{
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	} //end if
	else
	{
		bi->actionflags |= ACTION_DELAYEDJUMP;
	} //end if
} //end of the function EA_DelayedJump
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Crouch(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_CROUCH;
} //end of the function EA_Crouch
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Walk(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_WALK;
} //end of the function EA_Walk
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Action(int client, int action)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= action;
} //end of function EA_Action
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveUp(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEUP;
} //end of the function EA_MoveUp
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveDown(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEDOWN;
} //end of the function EA_MoveDown
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveForward(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEFORWARD;
} //end of the function EA_MoveForward
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveBack(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVEBACK;
} //end of the function EA_MoveBack
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveLeft(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVELEFT;
} //end of the function EA_MoveLeft
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_MoveRight(int client)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	bi->actionflags |= ACTION_MOVERIGHT;
} //end of the function EA_MoveRight
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Move(int client, vec3_t dir, float speed)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	VectorCopy(dir, bi->dir);
	//cap speed
	if (speed > MAX_USERMOVE) speed = MAX_USERMOVE;
	else if (speed < -MAX_USERMOVE) speed = -MAX_USERMOVE;
	bi->speed = speed;
} //end of the function EA_Move
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_View(int client, vec3_t viewangles)
{
	bot_input_t *bi;

	bi = &botinputs[client];

	VectorCopy(viewangles, bi->viewangles);
} //end of the function EA_View
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_EndRegular(int client, float thinktime)
{
} //end of the function EA_EndRegular
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_GetInput(int client, float thinktime, bot_input_t *input)
{
	bot_input_t *bi;

	bi = &botinputs[client];
	bi->thinktime = thinktime;
	Com_Memcpy(input, bi, sizeof(bot_input_t));
} //end of the function EA_GetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_ResetInput(int client)
{
	bot_input_t *bi;
	int jumped = qfalse;

	bi = &botinputs[client];

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
} //end of the function EA_ResetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int EA_Setup(void)
{
	//initialize the bot inputs
	Com_Memset(botinputs, 0, sizeof (botinputs));
	return BLERR_NOERROR;
} //end of the function EA_Setup
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Shutdown(void)
{

} //end of the function EA_Shutdown
