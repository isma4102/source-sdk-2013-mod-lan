//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Packed inventory slots shared by the server, the client HUD and pickup.
//
//=============================================================================//
#ifndef SURVIVAL_INVENTORY_H
#define SURVIVAL_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#define SURVIVAL_INVENTORY_SLOTS	12

#define SURVIVAL_ITEM_EMPTY			0
#define SURVIVAL_ITEM_FOOD			1
#define SURVIVAL_ITEM_WATER			2
#define SURVIVAL_ITEM_ANTIDOTE		3
#define SURVIVAL_ITEM_CAN			4
#define SURVIVAL_ITEM_PAPER			5
#define SURVIVAL_ITEM_RADIO			6
#define SURVIVAL_ITEM_RAG			7
#define SURVIVAL_ITEM_BOTTLE			8

inline bool Survival_IsJunkItem( int nType )
{
	return nType == SURVIVAL_ITEM_CAN
		|| nType == SURVIVAL_ITEM_PAPER
		|| nType == SURVIVAL_ITEM_RADIO
		|| nType == SURVIVAL_ITEM_RAG
		|| nType == SURVIVAL_ITEM_BOTTLE;
}

inline bool Survival_IsValidItemType( int nType )
{
	return nType == SURVIVAL_ITEM_FOOD
		|| nType == SURVIVAL_ITEM_WATER
		|| nType == SURVIVAL_ITEM_ANTIDOTE
		|| Survival_IsJunkItem( nType );
}

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
