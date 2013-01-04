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
#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
//NOTE: include the ui_public.h from the new UI
#include "../ui/ui_public.h"
#include "../client/keycodes.h"
#include "../game/bg_misc.h"

typedef void (*voidfunc_f)(void);

extern vmCvar_t	ui_ffa_fraglimit;
extern vmCvar_t	ui_ffa_timelimit;

extern vmCvar_t	ui_tourney_fraglimit;
extern vmCvar_t	ui_tourney_timelimit;

extern vmCvar_t	ui_team_fraglimit;
extern vmCvar_t	ui_team_timelimit;
extern vmCvar_t	ui_team_friendly;

extern vmCvar_t	ui_ctf_capturelimit;
extern vmCvar_t	ui_ctf_timelimit;
extern vmCvar_t	ui_ctf_friendly;

#ifdef MISSIONPACK
extern vmCvar_t	ui_1flag_capturelimit;
extern vmCvar_t	ui_1flag_timelimit;
extern vmCvar_t	ui_1flag_friendly;

extern vmCvar_t	ui_obelisk_capturelimit;
extern vmCvar_t	ui_obelisk_timelimit;
extern vmCvar_t	ui_obelisk_friendly;

extern vmCvar_t	ui_harvester_capturelimit;
extern vmCvar_t	ui_harvester_timelimit;
extern vmCvar_t	ui_harvester_friendly;
#endif

extern vmCvar_t	ui_publicServer;

extern vmCvar_t	ui_arenasFile;
extern vmCvar_t	ui_botsFile;
extern vmCvar_t	ui_spScores1;
extern vmCvar_t	ui_spScores2;
extern vmCvar_t	ui_spScores3;
extern vmCvar_t	ui_spScores4;
extern vmCvar_t	ui_spScores5;
extern vmCvar_t	ui_spAwards;
extern vmCvar_t	ui_spVideos;
extern vmCvar_t	ui_spSkill;

extern vmCvar_t	ui_spSelection;

extern vmCvar_t	ui_browserMaster;
extern vmCvar_t	ui_browserGameType;
extern vmCvar_t	ui_browserSortKey;
extern vmCvar_t	ui_browserShowFull;
extern vmCvar_t	ui_browserShowEmpty;

extern vmCvar_t	ui_brassTime;
extern vmCvar_t	ui_drawCrosshair;
extern vmCvar_t	ui_drawCrosshairNames;
extern vmCvar_t	ui_marks;

extern vmCvar_t	ui_server1;
extern vmCvar_t	ui_server2;
extern vmCvar_t	ui_server3;
extern vmCvar_t	ui_server4;
extern vmCvar_t	ui_server5;
extern vmCvar_t	ui_server6;
extern vmCvar_t	ui_server7;
extern vmCvar_t	ui_server8;
extern vmCvar_t	ui_server9;
extern vmCvar_t	ui_server10;
extern vmCvar_t	ui_server11;
extern vmCvar_t	ui_server12;
extern vmCvar_t	ui_server13;
extern vmCvar_t	ui_server14;
extern vmCvar_t	ui_server15;
extern vmCvar_t	ui_server16;

extern vmCvar_t	ui_ioq3;


//
// ui_qmenu.c
//

#define RCOLUMN_OFFSET			( BIGCHAR_WIDTH )
#define LCOLUMN_OFFSET			(-BIGCHAR_WIDTH )

#define SLIDER_RANGE			10
#define	MAX_EDIT_LINE			256

#define MAX_MENUDEPTH			8
#define MAX_MENUITEMS			64

#define MTYPE_NULL				0
#define MTYPE_SLIDER			1	
#define MTYPE_ACTION			2
#define MTYPE_SPINCONTROL		3
#define MTYPE_FIELD				4
#define MTYPE_RADIOBUTTON		5
#define MTYPE_BITMAP			6	
#define MTYPE_TEXT				7
#define MTYPE_SCROLLLIST		8
#define MTYPE_PTEXT				9
#define MTYPE_BTEXT				10

#define QMF_BLINK				((unsigned int) 0x00000001)
#define QMF_SMALLFONT			((unsigned int) 0x00000002)
#define QMF_LEFT_JUSTIFY		((unsigned int) 0x00000004)
#define QMF_CENTER_JUSTIFY		((unsigned int) 0x00000008)
#define QMF_RIGHT_JUSTIFY		((unsigned int) 0x00000010)
#define QMF_NUMBERSONLY			((unsigned int) 0x00000020)	// edit field is only numbers
#define QMF_HIGHLIGHT			((unsigned int) 0x00000040)
#define QMF_HIGHLIGHT_IF_FOCUS	((unsigned int) 0x00000080)	// steady focus
#define QMF_PULSEIFFOCUS		((unsigned int) 0x00000100)	// pulse if focus
#define QMF_HASMOUSEFOCUS		((unsigned int) 0x00000200)
#define QMF_NOONOFFTEXT			((unsigned int) 0x00000400)
#define QMF_MOUSEONLY			((unsigned int) 0x00000800)	// only mouse input allowed
#define QMF_HIDDEN				((unsigned int) 0x00001000)	// skips drawing
#define QMF_GRAYED				((unsigned int) 0x00002000)	// grays and disables
#define QMF_INACTIVE			((unsigned int) 0x00004000)	// disables any input
#define QMF_NODEFAULTINIT		((unsigned int) 0x00008000)	// skip default initialization
#define QMF_OWNERDRAW			((unsigned int) 0x00010000)
#define QMF_PULSE				((unsigned int) 0x00020000)
#define QMF_LOWERCASE			((unsigned int) 0x00040000)	// edit field is all lower case
#define QMF_UPPERCASE			((unsigned int) 0x00080000)	// edit field is all upper case
#define QMF_SILENT				((unsigned int) 0x00100000)

// callback notifications
#define QM_GOTFOCUS				1
#define QM_LOSTFOCUS			2
#define QM_ACTIVATED			3

typedef struct _tag_menuframework
{
	int	cursor;
	int cursor_prev;

	int	nitems;
	void *items[MAX_MENUITEMS];

	void (*draw) (void);
	sfxHandle_t (*key) (int key);

	qboolean	wrapAround;
	qboolean	fullscreen;
	qboolean	showlogo;
} menuframework_s;

typedef struct
{
	int type;
	const char *name;
	int	id;
	int x, y;
	int left;
	int	top;
	int	right;
	int	bottom;
	menuframework_s *parent;
	int menuPosition;
	unsigned int flags;

	void (*callback)( void *self, int event );
	void (*statusbar)( void *self );
	void (*ownerdraw)( void *self );
} menucommon_s;

typedef struct {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
	int		maxchars;
} mfield_t;

typedef struct
{
	menucommon_s	generic;
	mfield_t		field;
} menufield_s;

typedef struct 
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_s;

typedef struct
{
	menucommon_s generic;

	int	oldvalue;
	int curvalue;
	int	numitems;
	int	top;
		
	const char **itemnames;

	int width;
	int height;
	int	columns;
	int	seperation;
} menulist_s;

typedef struct
{
	menucommon_s generic;
} menuaction_s;

typedef struct
{
	menucommon_s generic;
	int curvalue;
} menuradiobutton_s;

typedef struct
{
	menucommon_s	generic;
	char*			focuspic;	
	char*			errorpic;
	qhandle_t		shader;
	qhandle_t		focusshader;
	int				width;
	int				height;
	float*			focuscolor;
} menubitmap_s;

typedef struct
{
	menucommon_s	generic;
	char*			string;
	int				style;
	float*			color;
} menutext_s;

extern void			Menu_Cache( void );
extern void			Menu_Focus( menucommon_s *m );
extern void			Menu_AddItem( menuframework_s *menu, void *item );
extern void			Menu_AdjustCursor( menuframework_s *menu, int dir );
extern void			Menu_Draw( menuframework_s *menu );
extern void			*Menu_ItemAtCursor( menuframework_s *m );
extern sfxHandle_t	Menu_ActivateItem( menuframework_s *s, menucommon_s* item );
extern void			Menu_SetCursor( menuframework_s *s, int cursor );
extern void			Menu_SetCursorToItem( menuframework_s *m, void* ptr );
extern sfxHandle_t	Menu_DefaultKey( menuframework_s *s, int key );
extern void			Bitmap_Init( menubitmap_s *b );
extern void			Bitmap_Draw( menubitmap_s *b );
extern void			ScrollList_Draw( menulist_s *l );
extern sfxHandle_t	ScrollList_Key( menulist_s *l, int key );
extern sfxHandle_t	menu_in_sound;
extern sfxHandle_t	menu_move_sound;
extern sfxHandle_t	menu_out_sound;
extern sfxHandle_t	menu_buzz_sound;
extern sfxHandle_t	menu_null_sound;
extern sfxHandle_t	weaponChangeSound;
extern vec4_t		menu_text_color;
extern vec4_t		menu_grayed_color;
extern vec4_t		menu_dark_color;
extern vec4_t		menu_highlight_color;
extern vec4_t		menu_red_color;
extern vec4_t		menu_black_color;
extern vec4_t		menu_dim_color;
extern vec4_t		color_black;
extern vec4_t		color_white;
extern vec4_t		color_yellow;
extern vec4_t		color_blue;
extern vec4_t		color_orange;
extern vec4_t		color_red;
extern vec4_t		color_dim;
extern vec4_t		name_color;
extern vec4_t		list_color;
extern vec4_t		listbar_color;
extern vec4_t		text_color_disabled; 
extern vec4_t		text_color_normal;
extern vec4_t		text_color_highlight;

extern char	*ui_medalNames[];
extern char	*ui_medalPicNames[];
extern char	*ui_medalSounds[];

//
// ui_mfield.c
//
extern void			MField_Clear( mfield_t *edit );
extern void			MField_KeyDownEvent( mfield_t *edit, int key );
extern void			MField_CharEvent( mfield_t *edit, int ch );
extern void			MField_Draw( mfield_t *edit, int x, int y, int style, vec4_t color );
extern void			MenuField_Init( menufield_s* m );
extern void			MenuField_Draw( menufield_s *f );
extern sfxHandle_t	MenuField_Key( menufield_s* m, int* key );

//
// ui_menu.c
//
extern void MainMenu_Cache( void );
extern void UI_MainMenu(void);
extern void UI_RegisterCvars( void );
extern void UI_UpdateCvars( void );

//
// ui_credits.c
//
extern void UI_CreditMenu( void );

//
// ui_ingame.c
//
extern void InGame_Cache( void );
extern void UI_InGameMenu(void);

//
// ui_ingame_selectplayer.c
//
extern void InSelectPlayer_Cache( void );
extern void InSelectPlayerMenu( void (*playerfunc)(int), const char *banner, qboolean disableMissingPlayers );

//
// ui_confirm.c
//
extern void ConfirmMenu_Cache( void );
extern void UI_ConfirmMenu( const char *question, void (*draw)( void ), void (*action)( qboolean result ) );
extern void UI_ConfirmMenu_Style( const char *question, int style, void (*draw)( void ), void (*action)( qboolean result ) );
extern void UI_Message( const char **lines );

//
// ui_setup.c
//
extern void UI_SetupMenu_Cache( void );
extern void UI_SetupMenu(void);

//
// ui_team.c
//
extern void UI_TeamMainMenu( int localClient );
extern void TeamMain_Cache( void );

//
// ui_connect.c
//
extern void UI_DrawConnectScreen( qboolean overlay );

//
// ui_selectplayer.c
//
extern void UI_SelectPlayerMenu( void (*playerfunc)(int), const char *banner );
extern void UI_SelectPlayer_Cache( void );

//
// ui_controls2.c
//
extern void UI_ControlsMenu( int localClient );
extern void Controls_Cache( void );
extern qboolean Controls_WantsBindKeys( void );

//
// ui_joystick.c
//
void UI_JoystickMenu( int localClient );
void UI_Joystick_Cache( void );

//
// ui_demo2.c
//
extern void UI_DemosMenu( void );
extern void Demos_Cache( void );

//
// ui_cinematics.c
//
extern void UI_CinematicsMenu( void );
extern void UI_CinematicsMenu_f( void );
extern void UI_CinematicsMenu_Cache( void );

//
// ui_mods.c
//
extern void UI_ModsMenu( void );
extern void UI_ModsMenu_Cache( void );

//
// ui_playermodel.c
//
extern void UI_PlayerModelMenu( int localClient );
extern void PlayerModel_Cache( void );

//
// ui_playersettings.c
//
extern void UI_PlayerSettingsMenu( int localClient );
extern void PlayerSettings_Cache( void );

//
// ui_preferences.c
//
extern void UI_PreferencesMenu( void );
extern void Preferences_Cache( void );

//
// ui_specifyleague.c
//
extern void UI_SpecifyLeagueMenu( void );
extern void SpecifyLeague_Cache( void );

//
// ui_specifyserver.c
//
extern void UI_SpecifyServerMenu( void );
extern void SpecifyServer_Cache( void );

//
// ui_servers2.c
//
#define MAX_FAVORITESERVERS 16

extern void UI_ArenaServersMenu( void );
extern void ArenaServers_Cache( void );

//
// ui_startserver.c
//
extern void UI_StartServerMenu( qboolean multiplayer );
extern void StartServer_Cache( void );
extern void ServerOptions_Cache( void );
extern void UI_BotSelectMenu( char *bot );
extern void UI_BotSelectMenu_Cache( void );

//
// ui_serverinfo.c
//
extern void UI_ServerInfoMenu( void );
extern void ServerInfo_Cache( void );

//
// ui_video.c
//
extern void UI_GraphicsOptionsMenu( void );
extern void GraphicsOptions_Cache( void );
extern void DriverInfo_Cache( void );

//
// ui_players.c
//

//FIXME ripped from cg_local.h
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct {
	// model info
	qhandle_t		legsModel;
	qhandle_t		legsSkin;
	lerpFrame_t		legs;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;
	lerpFrame_t		torso;

	qhandle_t		headModel;
	qhandle_t		headSkin;

	animation_t		animations[MAX_ANIMATIONS];

	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;
	vec3_t			flashDlightColor;
	int				muzzleFlashTime;

	vec3_t			color1;
	byte			c1RGBA[4];

	// currently in use drawing parms
	vec3_t			viewAngles;
	vec3_t			moveAngles;
	weapon_t		currentWeapon;
	int				legsAnim;
	int				torsoAnim;

	// animation vars
	weapon_t		weapon;
	weapon_t		lastWeapon;
	weapon_t		pendingWeapon;
	int				weaponTimer;
	int				pendingLegsAnim;
	int				torsoAnimationTimer;

	int				pendingTorsoAnim;
	int				legsAnimationTimer;

	qboolean		chat;
	qboolean		newModel;

	qboolean		barrelSpinning;
	float			barrelAngle;
	int				barrelTime;

	int				realWeapon;
} playerInfo_t;

void UI_DrawPlayer( float x, float y, float w, float h, playerInfo_t *pi, int time );
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model );
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNum, qboolean chat );
qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName );

//
// ui_atoms.c
//
typedef struct {
	int					frametime;
	int					realtime;
	int					cursorx;
	int					cursory;
	int					menusp;
	menuframework_s*	activemenu;
	menuframework_s*	stack[MAX_MENUDEPTH];
	glconfig_t			glconfig;
	qboolean			debug;
	qhandle_t			whiteShader;
	qhandle_t			menuBackShader;
	qhandle_t			menuBackNoLogoShader;
	qhandle_t			charset;
	qhandle_t			charsetProp;
	qhandle_t			charsetPropGlow;
	qhandle_t			charsetPropB;
	qhandle_t			cursor;
	qhandle_t			rb_on;
	qhandle_t			rb_off;
	float				xscale;
	float				yscale;
	float				bias;
	qboolean			demoversion;
	qboolean			firstdraw;
	int					maxSplitView;
} uiStatic_t;

extern void			UI_Init( qboolean inGameLoad, int maxSplitView );
extern void			UI_Shutdown( void );
extern void			UI_KeyEvent( int key, int down );
extern void			UI_MouseEvent( int localClientNum, int dx, int dy );
extern int			UI_MousePosition( int localClientNum );
extern void			UI_SetMousePosition( int localClientNum, int x, int y );
extern void			UI_Refresh( int realtime );
extern qboolean		UI_ConsoleCommand( int realTime );
extern float		UI_ClampCvar( float min, float max, float value );
extern void			UI_DrawNamedPic( float x, float y, float width, float height, const char *picname );
extern void			UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ); 
extern void			UI_FillRect( float x, float y, float width, float height, const float *color );
extern void			UI_DrawRect( float x, float y, float width, float height, const float *color );
extern void			UI_UpdateScreen( void );
extern void			UI_SetColor( const float *rgba );
extern void			UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t);
extern void			UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color );
extern float		UI_ProportionalSizeScale( int style );
extern void			UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
extern void			UI_DrawProportionalString_AutoWrapped( int x, int ystart, int xmax, int ystep, const char* str, int style, vec4_t color );
extern int			UI_ProportionalStringWidth( const char* str );
extern void			UI_DrawString( int x, int y, const char* str, int style, vec4_t color );
extern void			UI_DrawChar( int x, int y, int ch, int style, vec4_t color );
extern qboolean 	UI_CursorInRect (int x, int y, int width, int height);
extern void			UI_AdjustFrom640( float *x, float *y, float *w, float *h );
extern void			UI_DrawTextBox (int x, int y, int width, int lines);
extern qboolean		UI_IsFullscreen( void );
extern void			UI_SetActiveMenu( uiMenuCommand_t menu );
extern void			UI_PushMenu ( menuframework_s *menu );
extern void			UI_PopMenu (void);
extern void			UI_ForceMenuOff (void);
extern char			*UI_Argv( int arg );
extern char			*UI_Cvar_VariableString( const char *var_name );
extern void			UI_Refresh( int time );
extern int			UI_MaxSplitView(void);
extern int			UI_NumLocalClients(uiClientState_t *cs);
extern void			UI_StartDemoLoop( void );
extern qboolean		m_entersound;
extern uiStatic_t	uis;

//
// ui_spLevel.c
//
void UI_SPLevelMenu_Cache( void );
void UI_SPLevelMenu( void );
void UI_SPLevelMenu_f( void );
void UI_SPLevelMenu_ReInit( void );

//
// ui_spArena.c
//
void UI_SPArena_Start( const char *arenaInfo );

//
// ui_spPostgame.c
//
void UI_SPPostgameMenu_Cache( void );
void UI_SPPostgameMenu_f( void );

//
// ui_spSkill.c
//
void UI_SPSkillMenu( const char *arenaInfo );
void UI_SPSkillMenu_Cache( void );

//
// ui_syscalls.c
//

// Additional shared traps in ../game/bg_misc.h

void			trap_GetClipboardData( char *buf, int bufsize );
void			trap_GetGlconfig( glconfig_t *glconfig );
void			trap_UpdateScreen( void );
int				trap_MemoryRemaining( void );
int				trap_GetVoipTime( int clientNum );
float			trap_GetVoipPower( int clientNum );
float			trap_GetVoipGain( int clientNum );
qboolean		trap_GetVoipMute( int clientNum );
qboolean		trap_GetVoipMuteAll( void );

void			trap_GetClientState( uiClientState_t *state );
int				trap_GetConfigString( int index, char* buff, int buffsize );

int				trap_LAN_GetPingQueueCount( void );
void			trap_LAN_ClearPing( int n );
void			trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime );
void			trap_LAN_GetPingInfo( int n, char *buf, int buflen );

int				trap_LAN_GetServerCount( int source );
void			trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void			trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int				trap_LAN_GetServerPing( int source, int n );
void			trap_LAN_LoadCachedServers( void );
void			trap_LAN_SaveCachedServers( void );
void			trap_LAN_MarkServerVisible(int source, int n, qboolean visible);
int				trap_LAN_ServerIsVisible( int source, int n);
qboolean		trap_LAN_UpdateVisiblePings( int source );
int				trap_LAN_AddServer(int source, const char *name, const char *addr);
void			trap_LAN_RemoveServer(int source, const char *addr);
void			trap_LAN_ResetPings(int n);
int				trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
int				trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );

qhandle_t		trap_R_RegisterModel( const char *name );
qhandle_t		trap_R_RegisterSkin( const char *name );
qhandle_t		trap_R_RegisterShader( const char *name );
qhandle_t		trap_R_RegisterShaderNoMip( const char *name );
void			trap_R_RegisterFont(const char *pFontname, int pointSize, fontInfo_t *font);
void			trap_R_ClearScene( void );
void			trap_R_AddRefEntityToScene( const refEntity_t *re );
void			trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void			trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void			trap_R_AddPolyBufferToScene( polyBuffer_t* pPolyBuffer );
void			trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void			trap_R_RenderScene( const refdef_t *fd );
void			trap_R_SetColor( const float *rgba );
void			trap_R_SetClipRegion( const float *region );
void			trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void			trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void			trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader,
					const float *gradientColor, int gradientType );
void			trap_R_Add2dPolys( polyVert_t* verts, int numverts, qhandle_t hShader );
void			trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int				trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName );
void			trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

sfxHandle_t		trap_S_RegisterSound( const char *sample, qboolean compressed );
int				trap_S_SoundDuration( sfxHandle_t handle );
void			trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void			trap_S_StopBackgroundTrack( void );
void			trap_S_StartBackgroundTrack( const char *intro, const char *loop);

void			trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void			trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void			trap_Key_SetBinding( int keynum, const char *binding );
qboolean		trap_Key_IsDown( int keynum );
qboolean		trap_Key_GetOverstrikeMode( void );
void			trap_Key_SetOverstrikeMode( qboolean state );
void			trap_Key_ClearStates( void );
int				trap_Key_GetCatcher( void );
void			trap_Key_SetCatcher( int catcher );
int				trap_Key_GetKey( const char *binding, int startKey );

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int				trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status		trap_CIN_StopCinematic(int handle);

// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status		trap_CIN_RunCinematic (int handle);

// draws the current frame
void			trap_CIN_DrawCinematic (int handle);

// allows you to resize the animation dynamically
void			trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

//
// ui_addbots.c
//
void UI_AddBots_Cache( void );
void UI_AddBotsMenu( void );

//
// ui_removebots.c
//
void UI_RemoveBots_Cache( void );
void UI_RemoveBotsMenu( void );

//
// ui_teamorders.c
//
extern void UI_TeamOrdersMenu( void );
extern void UI_TeamOrdersMenu_f( void );
extern void UI_TeamOrdersMenu_Cache( void );

//
// ui_loadconfig.c
//
void UI_LoadConfig_Cache( void );
void UI_LoadConfigMenu( void );

//
// ui_saveconfig.c
//
void UI_SaveConfigMenu_Cache( void );
void UI_SaveConfigMenu( void );

//
// ui_display.c
//
void UI_DisplayOptionsMenu_Cache( void );
void UI_DisplayOptionsMenu( void );

//
// ui_sound.c
//
void UI_SoundOptionsMenu_Cache( void );
void UI_SoundOptionsMenu( void );

//
// ui_network.c
//
void UI_NetworkOptionsMenu_Cache( void );
void UI_NetworkOptionsMenu( void );

//
// ui_gameinfo.c
//
typedef enum {
	AWARD_ACCURACY,
	AWARD_IMPRESSIVE,
	AWARD_EXCELLENT,
	AWARD_GAUNTLET,
	AWARD_FRAGS,
	AWARD_PERFECT
} awardType_t;

const char *UI_GetArenaInfoByNumber( int num );
const char *UI_GetArenaInfoByMap( const char *map );
const char *UI_GetSpecialArenaInfo( const char *tag );
int UI_GetNumArenas( void );
int UI_GetNumSPArenas( void );
int UI_GetNumSPTiers( void );

char *UI_GetBotInfoByNumber( int num );
char *UI_GetBotInfoByName( const char *name );
int UI_GetNumBots( void );

void UI_GetBestScore( int level, int *score, int *skill );
void UI_SetBestScore( int level, int score );
int UI_TierCompleted( int levelWon );
qboolean UI_ShowTierVideo( int tier );
qboolean UI_CanShowTierVideo( int tier );
int  UI_GetCurrentGame( void );
void UI_NewGame( void );
void UI_LogAwardData( int award, int data );
int UI_GetAwardLevel( int award );

void UI_SPUnlock_f( void );
void UI_SPUnlockMedals_f( void );

void UI_InitGameinfo( void );

//GRank

//
// ui_rankings.c
//
void Rankings_DrawText( void* self );
void Rankings_DrawName( void* self );
void Rankings_DrawPassword( void* self );
void Rankings_Cache( void );
void UI_RankingsMenu( void );

//
// ui_login.c
//
void Login_Cache( void );
void UI_LoginMenu( void );

//
// ui_signup.c
//
void Signup_Cache( void );
void UI_SignupMenu( void );

//
// ui_rankstatus.c
//
void RankStatus_Cache( void );
void UI_RankStatusMenu( void );

#endif
