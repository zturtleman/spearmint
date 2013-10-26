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
equ tan							-12
equ atan						-13
equ pow							-14
equ exp							-15
equ log							-16
equ log10						-17

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
equ trap_Cvar_CheckRange					-43

equ trap_FS_FOpenFile					-44
equ trap_FS_Read						-45
equ trap_FS_Write						-46
equ trap_FS_Seek						-47
equ trap_FS_Tell						-48
equ trap_FS_FCloseFile					-49
equ trap_FS_GetFileList					-50
equ trap_FS_Delete						-51
equ trap_FS_Rename						-52

equ trap_PC_AddGlobalDefine				-53
equ trap_PC_RemoveAllGlobalDefines		-54
equ trap_PC_LoadSource					-55
equ trap_PC_FreeSource					-56
equ trap_PC_ReadToken					-57
equ trap_PC_UnreadToken					-58
equ trap_PC_SourceFileAndLine			-59

equ trap_Alloc							-60


equ	trap_LocateGameData					-101
equ	trap_DropClient						-102
equ	trap_SendServerCommandEx			-103
equ	trap_SetConfigstring				-104
equ	trap_GetConfigstring				-105
equ	trap_SetConfigstringRestrictions	-106
equ	trap_GetUserinfo					-107
equ	trap_SetUserinfo					-108
equ	trap_GetServerinfo					-109
equ	trap_SetBrushModel					-110
equ	trap_Trace							-111
equ	trap_PointContents					-112
equ trap_InPVS							-113
equ	trap_InPVSIgnorePortals				-114
equ	trap_AdjustAreaPortalState			-115
equ	trap_AreasConnected					-116
equ	trap_LinkEntity						-117
equ	trap_UnlinkEntity					-118
equ	trap_EntitiesInBox					-119
equ	trap_EntityContact					-120
equ	trap_BotAllocateClient				-121
equ	trap_BotFreeClient					-122
equ	trap_GetUsercmd						-123
equ	trap_GetEntityToken					-124
equ trap_DebugPolygonCreate				-125
equ trap_DebugPolygonDelete				-126
equ trap_TraceCapsule					-127
equ trap_EntityContactCapsule			-128
equ trap_SetNetFields					-129
equ trap_R_RegisterModel				-130
equ trap_R_LerpTag						-131
equ trap_R_ModelBounds					-132
equ trap_ClientCommand					-133


equ trap_BotLibSetup					-201
equ trap_BotLibShutdown					-202
equ trap_BotLibVarSet					-203
equ trap_BotLibVarGet					-204
equ trap_BotLibStartFrame				-205
equ trap_BotLibLoadMap					-206
equ trap_BotLibUpdateEntity				-207
equ trap_BotLibTest						-208

equ trap_BotGetSnapshotEntity			-209
equ trap_BotGetServerCommand			-210
equ trap_BotUserCommand					-211


; there is no 301.
equ trap_AAS_BBoxAreas					-302
equ trap_AAS_AreaInfo					-303
equ trap_AAS_Loaded						-304
equ trap_AAS_Initialized				-305
equ trap_AAS_PresenceTypeBoundingBox	-306
equ trap_AAS_Time						-307

equ trap_AAS_PointAreaNum				-308
equ trap_AAS_TraceClientBBox			-309
equ trap_AAS_TraceAreas					-310

equ trap_AAS_PointContents				-311
equ trap_AAS_NextBSPEntity				-312
equ trap_AAS_ValueForBSPEpairKey		-313
equ trap_AAS_VectorForBSPEpairKey		-314
equ trap_AAS_FloatForBSPEpairKey		-315
equ trap_AAS_IntForBSPEpairKey			-316


equ trap_AAS_PredictClientMovement		-326
equ trap_AAS_OnGround					-327
equ trap_AAS_Swimming					-328
equ trap_AAS_JumpReachRunStart			-329
equ trap_AAS_AgainstLadder				-330
equ trap_AAS_HorizontalVelocityForJump	-331
equ trap_AAS_DropToFloor				-332

equ trap_AAS_AreaReachability			-351
equ trap_AAS_BestReachableArea			-352
equ trap_AAS_BestReachableFromJumpPadArea	-353
equ trap_AAS_NextModelReachability		-354
equ trap_AAS_AreaGroundFaceArea			-355
equ trap_AAS_AreaCrouch					-356
equ trap_AAS_AreaSwim					-357
equ trap_AAS_AreaLiquid					-358
equ trap_AAS_AreaLava					-359
equ trap_AAS_AreaSlime					-360
equ trap_AAS_AreaGrounded				-361
equ trap_AAS_AreaLadder					-362
equ trap_AAS_AreaJumpPad				-363
equ trap_AAS_AreaDoNotEnter				-364


equ trap_AAS_TravelFlagForType			-401
equ trap_AAS_AreaContentsTravelFlags	-402
equ trap_AAS_NextAreaReachability		-403
equ trap_AAS_ReachabilityFromNum		-404
equ trap_AAS_RandomGoalArea				-405
equ trap_AAS_EnableRoutingArea			-406
equ trap_AAS_AreaTravelTime				-407
equ trap_AAS_AreaTravelTimeToGoalArea	-408
equ trap_AAS_PredictRoute				-409

equ trap_AAS_AlternativeRouteGoals		-421


equ trap_BotLoadCharacter				-501
equ trap_BotFreeCharacter				-502
equ trap_Characteristic_Float			-503
equ trap_Characteristic_BFloat			-504
equ trap_Characteristic_Integer			-505
equ trap_Characteristic_BInteger		-506
equ trap_Characteristic_String			-507


equ trap_BotAllocChatState				-508
equ trap_BotFreeChatState				-509
equ trap_BotQueueConsoleMessage			-510
equ trap_BotRemoveConsoleMessage		-511
equ trap_BotNextConsoleMessage			-512
equ trap_BotNumConsoleMessages			-513
equ trap_BotInitialChat					-514
equ trap_BotReplyChat					-515
equ trap_BotChatLength					-516
equ trap_BotEnterChat					-517
equ trap_StringContains					-518
equ trap_BotFindMatch					-519
equ trap_BotMatchVariable				-520
equ trap_UnifyWhiteSpaces				-521
equ trap_BotReplaceSynonyms				-522
equ trap_BotLoadChatFile				-523
equ trap_BotSetChatGender				-524
equ trap_BotSetChatName					-525
equ trap_BotNumInitialChats				-526
equ trap_BotGetChatMessage				-527

equ trap_GeneticParentsAndChildSelection	-528

equ trap_AAS_PointReachabilityAreaIndex	-529

