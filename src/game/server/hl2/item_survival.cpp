//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Food, water, antidote and junk picked up with +use (E) into the backpack. Walking into them does nothing.
//
//=============================================================================//

#include "cbase.h"
#include "items.h"
#include "hl2_player.h"

extern ConVar sv_survival_noise_radius_crate;
#include "engine/IEngineSound.h"
#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

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
	bool TryPickupType( CBaseEntity *pActivator, int nType );
	float m_flRestoreAmount;
};

BEGIN_DATADESC( CItemSurvivalConsumable )
	DEFINE_KEYFIELD( m_flRestoreAmount, FIELD_FLOAT, "restore_amount" ),
END_DATADESC()

bool CItemSurvivalConsumable::TryPickup( CBaseEntity *pActivator, bool bFood )
{
	return TryPickupType( pActivator, bFood ? SURVIVAL_ITEM_FOOD : SURVIVAL_ITEM_WATER );
}

bool CItemSurvivalConsumable::TryPickupType( CBaseEntity *pActivator, int nType )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( ToBasePlayer( pActivator ) );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	int nAmount = (int)m_flRestoreAmount;
	if ( nAmount <= 0 )
		nAmount = 25;

	if ( nType == SURVIVAL_ITEM_ANTIDOTE || Survival_IsJunkItem( nType ) )
		nAmount = 1;

	if ( !pPlayer->SurvivalInventory_Add( nType, nAmount ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_BackpackFull" );
		return false;
	}

	const char *pszSound = "ItemBattery.Touch";
	if ( nType == SURVIVAL_ITEM_FOOD )
		pszSound = "HealthKit.Touch";
	else if ( nType == SURVIVAL_ITEM_WATER || nType == SURVIVAL_ITEM_ANTIDOTE )
		pszSound = "HealthVial.Touch";

	CPASAttenuationFilter filter( pPlayer, pszSound );
	EmitSound( filter, pPlayer->entindex(), pszSound );

	const char *pszMsg = "#Survival_ItemStored";
	if ( nType == SURVIVAL_ITEM_FOOD )
		pszMsg = "#Survival_FoodStored";
	else if ( nType == SURVIVAL_ITEM_WATER )
		pszMsg = "#Survival_WaterStored";
	else if ( nType == SURVIVAL_ITEM_ANTIDOTE )
		pszMsg = "#Survival_AntidoteStored";
	else if ( Survival_IsJunkItem( nType ) )
		pszMsg = "#Survival_JunkStored";

	ClientPrint( pPlayer, HUD_PRINTCENTER, pszMsg );
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


class CItemAntidote : public CItemSurvivalConsumable
{
public:
	DECLARE_CLASS( CItemAntidote, CItemSurvivalConsumable );

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
		TryPickupType( pActivator, SURVIVAL_ITEM_ANTIDOTE );
	}
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote );
PRECACHE_REGISTER( item_antidote );

//-----------------------------------------------------------------------------
// Junk / filler pickups. Stored in the backpack; using them does nothing useful.
//-----------------------------------------------------------------------------
class CItemSurvivalJunk : public CItemSurvivalConsumable
{
public:
	DECLARE_CLASS( CItemSurvivalJunk, CItemSurvivalConsumable );

	CItemSurvivalJunk()
	{
		m_nJunkType = SURVIVAL_ITEM_CAN;
		m_pszModel = "models/props_junk/garbage_metalcan001a.mdl";
	}

	void Spawn( void )
	{
		Precache();
		SetModel( m_pszModel );
		BaseClass::Spawn();
	}

	void Precache( void )
	{
		PrecacheModel( m_pszModel );
		PrecacheScriptSound( "ItemBattery.Touch" );
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		TryPickupType( pActivator, m_nJunkType );
	}

protected:
	int m_nJunkType;
	const char *m_pszModel;
};

class CItemJunkCan : public CItemSurvivalJunk
{
public:
	DECLARE_CLASS( CItemJunkCan, CItemSurvivalJunk );
	CItemJunkCan() { m_nJunkType = SURVIVAL_ITEM_CAN; m_pszModel = "models/props_junk/garbage_metalcan001a.mdl"; }
};
LINK_ENTITY_TO_CLASS( item_junk_can, CItemJunkCan );
PRECACHE_REGISTER( item_junk_can );

class CItemJunkPaper : public CItemSurvivalJunk
{
public:
	DECLARE_CLASS( CItemJunkPaper, CItemSurvivalJunk );
	CItemJunkPaper() { m_nJunkType = SURVIVAL_ITEM_PAPER; m_pszModel = "models/props_junk/garbage_newspaper001a.mdl"; }
};
LINK_ENTITY_TO_CLASS( item_junk_paper, CItemJunkPaper );
PRECACHE_REGISTER( item_junk_paper );

class CItemJunkRadio : public CItemSurvivalJunk
{
public:
	DECLARE_CLASS( CItemJunkRadio, CItemSurvivalJunk );
	CItemJunkRadio() { m_nJunkType = SURVIVAL_ITEM_RADIO; m_pszModel = "models/props_lab/citizenradio.mdl"; }
};
LINK_ENTITY_TO_CLASS( item_junk_radio, CItemJunkRadio );
PRECACHE_REGISTER( item_junk_radio );

class CItemJunkRag : public CItemSurvivalJunk
{
public:
	DECLARE_CLASS( CItemJunkRag, CItemSurvivalJunk );
	CItemJunkRag() { m_nJunkType = SURVIVAL_ITEM_RAG; m_pszModel = "models/props_junk/garbage_takeoutcarton001a.mdl"; }
};
LINK_ENTITY_TO_CLASS( item_junk_rag, CItemJunkRag );
PRECACHE_REGISTER( item_junk_rag );

class CItemJunkBottle : public CItemSurvivalJunk
{
public:
	DECLARE_CLASS( CItemJunkBottle, CItemSurvivalJunk );
	CItemJunkBottle() { m_nJunkType = SURVIVAL_ITEM_BOTTLE; m_pszModel = "models/props_junk/garbage_plasticbottle001a.mdl"; }
};
LINK_ENTITY_TO_CLASS( item_junk_bottle, CItemJunkBottle );
PRECACHE_REGISTER( item_junk_bottle );
class CSurvivalCrate : public CBaseAnimating
{
public:
	DECLARE_CLASS( CSurvivalCrate, CBaseAnimating );
	DECLARE_DATADESC();

	CSurvivalCrate()
	{
		m_nFoodCount = 1;
		m_nWaterCount = 1;
		m_nFoodAmount = 25;
		m_nWaterAmount = 25;
	}

	void Precache( void )
	{
		PrecacheModel( "models/props_junk/wood_crate001a.mdl" );
		PrecacheScriptSound( "ItemBattery.Touch" );
	}

	void Spawn( void )
	{
		Precache();
		SetModel( "models/props_junk/wood_crate001a.mdl" );
		SetSolid( SOLID_VPHYSICS );
		SetMoveType( MOVETYPE_NONE );
		VPhysicsInitStatic();
	}

	int ObjectCaps( void )
	{
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
	int m_nFoodCount;
	int m_nWaterCount;
	int m_nFoodAmount;
	int m_nWaterAmount;
};

BEGIN_DATADESC( CSurvivalCrate )
	DEFINE_KEYFIELD( m_nFoodCount, FIELD_INTEGER, "food_count" ),
	DEFINE_KEYFIELD( m_nWaterCount, FIELD_INTEGER, "water_count" ),
	DEFINE_KEYFIELD( m_nFoodAmount, FIELD_INTEGER, "food_amount" ),
	DEFINE_KEYFIELD( m_nWaterAmount, FIELD_INTEGER, "water_amount" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( survival_crate, CSurvivalCrate );
PRECACHE_REGISTER( survival_crate );

void CSurvivalCrate::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( ToBasePlayer( pActivator ) );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	Survival_EmitNoise( GetAbsOrigin(), sv_survival_noise_radius_crate.GetFloat(), pPlayer );

	if ( m_nFoodCount <= 0 && m_nWaterCount <= 0 )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_CrateEmpty" );
		return;
	}

	bool bTook = false;
	while ( m_nFoodCount > 0 )
	{
		int nAmount = m_nFoodAmount > 0 ? m_nFoodAmount : 25;
		if ( !pPlayer->SurvivalInventory_Add( SURVIVAL_ITEM_FOOD, nAmount ) )
			break;
		m_nFoodCount--;
		bTook = true;
	}

	while ( m_nWaterCount > 0 )
	{
		int nAmount = m_nWaterAmount > 0 ? m_nWaterAmount : 25;
		if ( !pPlayer->SurvivalInventory_Add( SURVIVAL_ITEM_WATER, nAmount ) )
			break;
		m_nWaterCount--;
		bTook = true;
	}

	if ( !bTook )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_BackpackFull" );
		return;
	}

	CPASAttenuationFilter filter( pPlayer, "ItemBattery.Touch" );
	EmitSound( filter, pPlayer->entindex(), "ItemBattery.Touch" );

	if ( m_nFoodCount <= 0 && m_nWaterCount <= 0 )
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_CrateCleared" );
	else
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_CratePartial" );
}

#ifdef HL2MP
class CSurvivalBed : public CBaseAnimating
{
public:
	DECLARE_CLASS( CSurvivalBed, CBaseAnimating );

	CSurvivalBed()
	{
		m_flNextSleep = 0.0f;
	}

	void Precache( void )
	{
		PrecacheModel( "models/props_c17/FurnitureMattress001a.mdl" );
	}

	void Spawn( void )
	{
		Precache();
		SetModel( "models/props_c17/FurnitureMattress001a.mdl" );
		SetSolid( SOLID_VPHYSICS );
		SetMoveType( MOVETYPE_NONE );
		VPhysicsInitStatic();
	}

	int ObjectCaps( void )
	{
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		if ( gpGlobals->curtime < m_flNextSleep )
			return;

		if ( !HL2MPRules() )
			return;

		m_flNextSleep = gpGlobals->curtime + 2.0f;
		HL2MPRules()->Survival_Sleep( ToBasePlayer( pActivator ) );
	}

private:
	float m_flNextSleep;
};

LINK_ENTITY_TO_CLASS( survival_bed, CSurvivalBed );
PRECACHE_REGISTER( survival_bed );
#endif
