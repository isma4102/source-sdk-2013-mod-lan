//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Food and water consumed with +use (E). Walking into them does nothing.
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
	bool TryConsume( CBaseEntity *pActivator, bool bFood );
	float m_flRestoreAmount;
};

BEGIN_DATADESC( CItemSurvivalConsumable )
	DEFINE_KEYFIELD( m_flRestoreAmount, FIELD_FLOAT, "restore_amount" ),
END_DATADESC()

bool CItemSurvivalConsumable::TryConsume( CBaseEntity *pActivator, bool bFood )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( ToBasePlayer( pActivator ) );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	float flAmount = m_flRestoreAmount;
	if ( flAmount <= 0.0f )
		flAmount = 25.0f;

	bool bApplied = bFood ? pPlayer->ApplyFood( flAmount ) : pPlayer->ApplyWater( flAmount );
	if ( !bApplied )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, bFood ? "Ya no tienes hambre" : "Ya no tienes sed" );
		return false;
	}

	const char *pszSound = bFood ? "HealthKit.Touch" : "HealthVial.Touch";
	CPASAttenuationFilter filter( pPlayer, pszSound );
	EmitSound( filter, pPlayer->entindex(), pszSound );

	char szMsg[64];
	Q_snprintf( szMsg, sizeof( szMsg ), bFood ? "Hambre +%d" : "Sed +%d", (int)flAmount );
	ClientPrint( pPlayer, HUD_PRINTCENTER, szMsg );

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
		TryConsume( pActivator, true );
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
		TryConsume( pActivator, false );
	}
};

LINK_ENTITY_TO_CLASS( item_water, CItemWater );
PRECACHE_REGISTER( item_water );
