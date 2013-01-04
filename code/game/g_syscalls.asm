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


equ	trap_LocateGameData					-101
equ	trap_DropClient						-102
equ	trap_SendServerCommandEx			-103
equ	trap_SetConfigstring				-104
equ	trap_GetConfigstring				-105
equ	trap_GetUserinfo					-106
equ	trap_SetUserinfo					-107
equ	trap_GetServerinfo					-108
equ	trap_SetBrushModel					-109
equ	trap_Trace							-110
equ	trap_PointContents					-111
equ trap_InPVS							-112
equ	trap_InPVSIgnorePortals				-113
equ	trap_AdjustAreaPortalState			-114
equ	trap_AreasConnected					-115
equ	trap_LinkEntity						-116
equ	trap_UnlinkEntity					-117
equ	trap_EntitiesInBox					-118
equ	trap_EntityContact					-119
equ	trap_BotAllocateClient				-120
equ	trap_BotFreeClient					-121
equ	trap_GetUsercmd						-122
equ	trap_GetEntityToken					-123
equ trap_DebugPolygonCreate				-124
equ trap_DebugPolygonDelete				-125
equ trap_TraceCapsule					-126
equ trap_EntityContactCapsule			-127


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



equ trap_AAS_EnableRoutingArea		-301
equ trap_AAS_BBoxAreas				-302
equ trap_AAS_AreaInfo				-303
equ trap_AAS_EntityInfo					-304

equ trap_AAS_Initialized				-305
equ trap_AAS_PresenceTypeBoundingBox	-306
equ trap_AAS_Time						-307

equ trap_AAS_PointAreaNum				-308
equ trap_AAS_TraceAreas					-309

equ trap_AAS_PointContents				-310
equ trap_AAS_NextBSPEntity				-311
equ trap_AAS_ValueForBSPEpairKey		-312
equ trap_AAS_VectorForBSPEpairKey		-313
equ trap_AAS_FloatForBSPEpairKey		-314
equ trap_AAS_IntForBSPEpairKey			-315

equ trap_AAS_AreaReachability			-316

equ trap_AAS_AreaTravelTimeToGoalArea	-317

equ trap_AAS_Swimming					-318
equ trap_AAS_PredictClientMovement		-319

equ trap_AAS_BestReachableArea			-320



equ trap_EA_Say							-401
equ trap_EA_SayTeam						-402
equ trap_EA_Command						-403

equ trap_EA_Action						-404
equ trap_EA_Gesture						-405
equ trap_EA_Talk						-406
equ trap_EA_Attack						-407
equ trap_EA_Use							-408
equ trap_EA_Respawn						-409
equ trap_EA_Crouch						-410
equ trap_EA_MoveUp						-411
equ trap_EA_MoveDown					-412
equ trap_EA_MoveForward					-413
equ trap_EA_MoveBack					-414
equ trap_EA_MoveLeft					-415
equ trap_EA_MoveRight					-416

equ trap_EA_SelectWeapon				-417
equ trap_EA_Jump						-418
equ trap_EA_DelayedJump					-419
equ trap_EA_Move						-420
equ trap_EA_View						-421

equ trap_EA_EndRegular					-422
equ trap_EA_GetInput					-423
equ trap_EA_ResetInput					-424



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

equ trap_BotResetGoalState				-526
equ trap_BotResetAvoidGoals				-527
equ trap_BotPushGoal					-528
equ trap_BotPopGoal						-529
equ trap_BotEmptyGoalStack				-530
equ trap_BotDumpAvoidGoals				-531
equ trap_BotDumpGoalStack				-532
equ trap_BotGoalName					-533
equ trap_BotGetTopGoal					-534
equ trap_BotGetSecondGoal				-535
equ trap_BotChooseLTGItem				-536
equ trap_BotChooseNBGItem				-537
equ trap_BotTouchingGoal				-538
equ trap_BotItemGoalInVisButNotVisible	-539
equ trap_BotGetLevelItemGoal			-540
equ trap_BotAvoidGoalTime				-541
equ trap_BotInitLevelItems				-542
equ trap_BotUpdateEntityItems			-543
equ trap_BotLoadItemWeights				-544
equ trap_BotFreeItemWeights				-546
equ trap_BotSaveGoalFuzzyLogic			-546
equ trap_BotAllocGoalState				-547
equ trap_BotFreeGoalState				-548

equ trap_BotResetMoveState				-549
equ trap_BotMoveToGoal					-550
equ trap_BotMoveInDirection				-551
equ trap_BotResetAvoidReach				-552
equ trap_BotResetLastAvoidReach			-553
equ trap_BotReachabilityArea			-554
equ trap_BotMovementViewTarget			-555
equ trap_BotAllocMoveState				-556
equ trap_BotFreeMoveState				-557
equ trap_BotInitMoveState				-558

equ trap_BotChooseBestFightWeapon		-559
equ trap_BotGetWeaponInfo				-560
equ trap_BotLoadWeaponWeights			-561
equ trap_BotAllocWeaponState			-562
equ trap_BotFreeWeaponState				-563
equ trap_BotResetWeaponState			-564
equ trap_GeneticParentsAndChildSelection -565
equ trap_BotInterbreedGoalFuzzyLogic	-566
equ trap_BotMutateGoalFuzzyLogic		-567
equ trap_BotGetNextCampSpotGoal			-568
equ trap_BotGetMapLocationGoal			-569
equ trap_BotNumInitialChats				-570
equ trap_BotGetChatMessage				-571
equ trap_BotRemoveFromAvoidGoals		-572
equ trap_BotPredictVisiblePosition		-573
equ trap_BotSetAvoidGoalTime			-574
equ trap_BotAddAvoidSpot				-575
equ trap_AAS_AlternativeRouteGoals		-576
equ trap_AAS_PredictRoute				-577
equ trap_AAS_PointReachabilityAreaIndex	-578

