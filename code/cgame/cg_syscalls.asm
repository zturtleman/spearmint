code

equ memset						-1
equ memcpy						-2
equ strncpy						-3
equ sin							-4
equ cos							-5
equ atan2						-6
equ sqrt						-7
equ floor						-8
equ ceil						-9
equ Q_acos						-10
equ Q_asin						-11

equ trap_Print							-21
equ trap_Error							-22
equ trap_Milliseconds					-23
equ trap_RealTime						-24
equ trap_SnapVector						-25

equ trap_Argc							-26
equ trap_Argv							-27
equ trap_Args							-28
equ trap_LiteralArgs					-29

equ trap_AddCommand						-30
equ trap_RemoveCommand					-31
equ trap_Cmd_ExecuteText				-32

equ trap_Cvar_Register						-33
equ trap_Cvar_Update						-34
equ trap_Cvar_Set							-35
equ trap_Cvar_SetValue						-36
equ trap_Cvar_Reset							-37
equ trap_Cvar_VariableValue					-38
equ trap_Cvar_VariableIntegerValue			-39
equ trap_Cvar_VariableStringBuffer			-40
equ trap_Cvar_LatchedVariableStringBuffer	-41
equ trap_Cvar_InfoStringBuffer				-42

equ trap_FS_FOpenFile					-43
equ trap_FS_Read						-44
equ trap_FS_Write						-45
equ trap_FS_Seek						-46
equ trap_FS_FCloseFile					-47
equ trap_FS_GetFileList					-48
equ trap_FS_Delete						-49
equ trap_FS_Rename						-50

equ trap_PC_AddGlobalDefine				-51
equ trap_PC_RemoveAllGlobalDefines		-52
equ trap_PC_LoadSource					-53
equ trap_PC_FreeSource					-54
equ trap_PC_ReadToken					-55
equ trap_PC_UnreadToken					-56
equ trap_PC_SourceFileAndLine			-57



equ	trap_GetClipboardData				-101
equ	trap_GetGlconfig					-102
equ trap_MemoryRemaining				-103
equ	trap_UpdateScreen					-104
equ	trap_GetVoipTime					-105
equ	trap_GetVoipPower					-106
equ	trap_GetVoipGain					-107
equ	trap_GetVoipMute					-108
equ	trap_GetVoipMuteAll					-109

equ	trap_GetGameState					-151
equ	trap_GetCurrentSnapshotNumber		-152
equ	trap_GetSnapshot					-153
equ	trap_GetServerCommand				-154
equ	trap_GetCurrentCmdNumber			-155
equ	trap_GetUserCmd						-156
equ	trap_SetUserCmdValue				-157
equ	trap_SendClientCommand				-158



equ	trap_CM_LoadMap						-201
equ	trap_CM_NumInlineModels				-202
equ	trap_CM_InlineModel					-203
equ	trap_CM_MarkFragments				-204
equ	trap_CM_PointContents				-205
equ	trap_CM_TransformedPointContents	-206
equ	trap_CM_TempBoxModel				-207
equ	trap_CM_BoxTrace					-208
equ	trap_CM_TransformedBoxTrace			-209
equ trap_CM_TempCapsuleModel			-210
equ trap_CM_CapsuleTrace				-211
equ trap_CM_TransformedCapsuleTrace		-212
equ trap_CM_BiSphereTrace				-213
equ trap_CM_TransformedBiSphereTrace	-214



equ	trap_R_RegisterModel				-301
equ	trap_R_RegisterSkin					-302
equ	trap_R_RegisterShader				-303
equ	trap_R_RegisterShaderNoMip			-304
equ trap_R_RegisterFont					-305
equ	trap_R_ClearScene					-306
equ	trap_R_AddRefEntityToScene			-307
equ	trap_R_AddPolyToScene				-308
equ	trap_R_AddLightToScene				-309
equ	trap_R_RenderScene					-310
equ	trap_R_SetColor						-311
equ	trap_R_DrawStretchPic				-312
equ	trap_R_LerpTag						-313
equ trap_R_ModelBounds					-314
equ trap_R_RemapShader					-315
equ	trap_R_SetClipRegion				-316
equ trap_R_DrawRotatedPic				-317
equ trap_R_DrawStretchPicGradient		-318
equ trap_R_Add2dPolys					-319
equ	trap_R_AddPolysToScene				-320
equ	trap_R_AddPolyBufferToScene			-321

equ	trap_R_LoadWorldMap					-351
equ trap_GetEntityToken					-352
equ trap_R_LightForPoint				-353
equ trap_R_inPVS						-354
equ trap_R_AddAdditiveLightToScene		-355
equ trap_R_GetGlobalFog					-356
equ trap_R_GetWaterFog					-357



equ	trap_S_RegisterSound				-401
equ trap_S_SoundDuration				-402
equ	trap_S_StartLocalSound				-403
equ trap_S_StartBackgroundTrack			-404
equ trap_S_StopBackgroundTrack			-405

equ	trap_S_StartSound					-451
equ	trap_S_ClearLoopingSounds			-452
equ	trap_S_AddLoopingSound				-453
equ	trap_S_UpdateEntityPosition			-454
equ	trap_S_Respatialize					-455
equ	trap_S_AddRealLoopingSound			-456
equ trap_S_StopLoopingSound				-457



equ trap_Key_KeynumToStringBuf			-501
equ trap_Key_GetBindingBuf				-502
equ trap_Key_SetBinding					-503
equ trap_Key_IsDown						-504
equ trap_Key_GetOverstrikeMode			-505
equ trap_Key_SetOverstrikeMode			-506
equ trap_Key_ClearStates				-507
equ trap_Key_GetCatcher					-508
equ trap_Key_SetCatcher					-509
equ trap_Key_GetKey						-510



equ trap_CIN_PlayCinematic				-601
equ trap_CIN_StopCinematic				-602
equ trap_CIN_RunCinematic 				-603
equ trap_CIN_DrawCinematic				-604
equ trap_CIN_SetExtents					-605

