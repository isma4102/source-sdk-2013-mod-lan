//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Food and water picked up with +use (E) into the backpack. Walking into them does nothing.
//
//=============================================================================//

#include "cbase.h"
#include "items.h"
#include "hl2_player.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CItemSurvivalConsumable : public CItem
{
public:
	DECLARE_CLASS( CItemSurvivalConsumable, CItem );
	DECLARE_DATADESC();

	CItemSurvivalConsumable()
	{
		m_flRestoreAmount = 25.0f;
	}

	bool MyTouch( CBasePlayer *pPlayer )
	{
		// Confirmed decision: eat and drink with E, not by walking over the item.
		return false;
	}

protected:
	bool TryPickup( CBaseEntity *pActivator, bool bFood );
	float m_flRestoreAmount;
};

BEGIN_DATADESC( CItemSurvivalConsumable )
	DEFINE_KEYFIELD( m_flRestoreAmount, FIELD_FLOAT, "restore_amount" ),
END_DATADESC()

bool CItemSurvivalConsumable::TryPickup( CBaseEntity *pActivator, bool bFood )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( ToBasePlayer( pActivator ) );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	int nAmount = (int)m_flRestoreAmount;
	if ( nAmount <= 0 )
		nAmount = 25;

	if ( !pPlayer->SurvivalInventory_Add( bFood ? SURVIVAL_ITEM_FOOD : SURVIVAL_ITEM_WATER, nAmount ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "Mochila llena" );
		return false;
	}

	const char *pszSound = bFood ? "HealthKit.Touch" : "HealthVial.Touch";
	CPASAttenuationFilter filter( pPlayer, pszSound );
	EmitSound( filter, pPlayer->entindex(), pszSound );

	ClientPrint( pPlayer, HUD_PRINTCENTER, bFood ? "Comida guardada" : "Agua guardada" );
	UTIL_Remove( this );
	return true;
}

class CItemFood : public CItemSurvivalConsumable
{
public:
	DECLARE_CLASS( CItemFood, CItemSurvivalConsumable );

	void Spawn( void )
	{
		Precache();
		SetModel( "models/items/healthkit.mdl" );
		BaseClass::Spawn();
	}

	void Precache( void )
	{
		PrecacheModel( "models/items/healthkit.mdl" );
		PrecacheScriptSound( "HealthKit.Touch" );
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		TryPickup( pActivator, true );
	}
};

LINK_ENTITY_TO_CLASS( item_food, CItemFood );
PRECACHE_REGISTER( item_food );

class CItemWater : public CItemSurvivalConsumable
{
public:
	DECLARE_CLASS( CItemWater, CItemSurvivalConsumable );

	void Spawn( void )
	{
		Precache();
		SetModel( "models/healthvial.mdl" );
		BaseClass::Spawn();
	}

	void Precache( void )
	{
		PrecacheModel( "models/healthvial.mdl" );
		PrecacheScriptSound( "HealthVial.Touch" );
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		TryPickup( pActivator, false );
	}
};

LINK_ENTITY_TO_CLASS( item_water, CItemWater );
PRECACHE_REGISTER( item_water );
