//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Classnames that do not belong in the realistic survival arsenal.
//
//=============================================================================//
#ifndef SURVIVAL_ARSENAL_H
#define SURVIVAL_ARSENAL_H
#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL
// True when this entity should not exist in the survival mod.
// sv_survival_realistic_weapons 0 turns the filter off.
bool Survival_BlockFictionalItem( const char *pszClass );
#endif

#endif // SURVIVAL_ARSENAL_H
