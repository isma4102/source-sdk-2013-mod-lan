//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Packed food/water slots shared by the server, the client HUD and pickup.
//
//=============================================================================//
#ifndef SURVIVAL_INVENTORY_H
#define SURVIVAL_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#define SURVIVAL_INVENTORY_SLOTS	8
#define SURVIVAL_STASH_SLOTS		8
#define SURVIVAL_ITEM_EMPTY			0
#define SURVIVAL_ITEM_FOOD			1
#define SURVIVAL_ITEM_WATER			2
#define SURVIVAL_ITEM_ANTIDOTE		3

inline int Survival_PackSlot( int nType, int nAmount )
{
	if ( nAmount < 0 )
		nAmount = 0;
	if ( nAmount > 100 )
		nAmount = 100;

	return ( nType & 0xFF ) | ( ( nAmount & 0xFF ) << 8 );
}

inline int Survival_SlotType( int nPacked )
{
	return nPacked & 0xFF;
}

inline int Survival_SlotAmount( int nPacked )
{
	return ( nPacked >> 8 ) & 0xFF;
}

#endif // SURVIVAL_INVENTORY_H
