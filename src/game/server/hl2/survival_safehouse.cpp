//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Claimable safehouse, shared stash and a short rest cot.
//          Brush doors stay in the map. These classes only lock a named
//          func_door / func_door_rotating, or fire OnClaimed into a logic_relay.
//
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "triggers.h"
#include "ai_basenpc.h"
#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_survival_safehouse_require_clear( "sv_survival_safehouse_require_clear", "1", FCVAR_NOTIFY, "survival_claim_safehouse fails while a zombie or headcrab is inside the area. Per-entity require_clear 0 skips that house." );

static bool Survival_IsSafehouseHostile( CBaseEntity *pEnt )
{
	return Survival_IsZombieEntity( pEnt ) || Survival_IsHeadcrabEntity( pEnt );
}

static bool Survival_HostileInSphere( const Vector &vecCenter, float flRadius )
{
	if ( flRadius < 1.0f )
		return false;

	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();
	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[i];
		if ( !pNPC || !pNPC->IsAlive() || !Survival_IsSafehouseHostile( pNPC ) )
			continue;
		if ( pNPC->GetAbsOrigin().DistTo( vecCenter ) <= flRadius )
			return true;
	}
	return false;
}

static bool Survival_HostileInAABB( const Vector &mins, const Vector &maxs )
{
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();
	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[i];
		if ( !pNPC || !pNPC->IsAlive() || !Survival_IsSafehouseHostile( pNPC ) )
			continue;

		Vector pt = pNPC->WorldSpaceCenter();
		if ( pt.x < mins.x || pt.x > maxs.x || pt.y < mins.y || pt.y > maxs.y || pt.z < mins.z || pt.z > maxs.z )
			continue;
		return true;
	}
	return false;
}

static void Survival_LockNamedDoor( const char *pszName, CBaseEntity *pActivator, CBaseEntity *pCaller, bool bClose )
{
	if ( !pszName || !pszName[0] )
		return;

	CBaseEntity *pDoor = NULL;
	while ( ( pDoor = gEntList.FindEntityByName( pDoor, pszName ) ) != NULL )
	{
		variant_t empty;
		pDoor->AcceptInput( "Lock", pActivator, pCaller, empty, 0 );
		if ( bClose )
			pDoor->AcceptInput( "Close", pActivator, pCaller, empty, 0 );
	}
}

struct SurvivalClaimParams_t
{
	CBaseEntity *pEnt;
	CHL2_Player *pPlayer;
	Vector vecMarker;
	bool bHostiles;
	string_t iszDoor;
	string_t iszDoor2;
	bool bCloseDoors;
	COutputEvent *pOnClaimed;
	COutputEvent *pOnFailed;
};

static bool Survival_FinishClaim( const SurvivalClaimParams_t &claim )
{
	if ( claim.bHostiles )
	{
		if ( claim.pPlayer )
			ClientPrint( claim.pPlayer, HUD_PRINTCENTER, "#Survival_SafehouseHostiles" );
		if ( claim.pOnFailed )
			claim.pOnFailed->FireOutput( claim.pPlayer, claim.pEnt );
		return false;
	}

	Survival_LockNamedDoor( STRING( claim.iszDoor ), claim.pPlayer, claim.pEnt, claim.bCloseDoors );
	Survival_LockNamedDoor( STRING( claim.iszDoor2 ), claim.pPlayer, claim.pEnt, claim.bCloseDoors );

#ifdef HL2MP
	if ( HL2MPRules() )
		HL2MPRules()->Survival_SetSafehouse( claim.vecMarker );
#endif

	if ( claim.pOnClaimed )
		claim.pOnClaimed->FireOutput( claim.pPlayer, claim.pEnt );

	if ( claim.pPlayer )
	{
		ClientPrint( claim.pPlayer, HUD_PRINTCENTER, "#Survival_SafehouseClaimed" );
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#Survival_SafehouseTalk", claim.pPlayer->GetPlayerName() );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#Survival_SafehouseClaimed" );
	}

	return true;
}

class CInfoSurvivalSafehouse : public CPointEntity
{
public:
	DECLARE_CLASS( CInfoSurvivalSafehouse, CPointEntity );
	DECLARE_DATADESC();

	CInfoSurvivalSafehouse()
	{
		m_flRadius = 384.0f;
		m_bRequireClear = true;
		m_bCloseDoors = false;
	}

	void Spawn( void )
	{
		BaseClass::Spawn();
		if ( m_flRadius < 32.0f )
			m_flRadius = 32.0f;
	}

	float GetRadius( void ) const { return m_flRadius; }
	bool ClaimFrom( CHL2_Player *pPlayer );
	void InputClaim( inputdata_t &inputdata );

private:
	float m_flRadius;
	bool m_bRequireClear;
	bool m_bCloseDoors;
	string_t m_iszDoor;
	string_t m_iszDoor2;
	COutputEvent m_OnClaimed;
	COutputEvent m_OnClaimFailed;
};

BEGIN_DATADESC( CInfoSurvivalSafehouse )
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_bRequireClear, FIELD_BOOLEAN, "require_clear" ),
	DEFINE_KEYFIELD( m_bCloseDoors, FIELD_BOOLEAN, "close_doors" ),
	DEFINE_KEYFIELD( m_iszDoor, FIELD_STRING, "door" ),
	DEFINE_KEYFIELD( m_iszDoor2, FIELD_STRING, "door2" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Claim", InputClaim ),
	DEFINE_OUTPUT( m_OnClaimed, "OnClaimed" ),
	DEFINE_OUTPUT( m_OnClaimFailed, "OnClaimFailed" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_survival_safehouse, CInfoSurvivalSafehouse );

bool CInfoSurvivalSafehouse::ClaimFrom( CHL2_Player *pPlayer )
{
	bool bCheck = m_bRequireClear && sv_survival_safehouse_require_clear.GetBool();
	SurvivalClaimParams_t claim;
	claim.pEnt = this;
	claim.pPlayer = pPlayer;
	claim.vecMarker = GetAbsOrigin();
	claim.bHostiles = bCheck && Survival_HostileInSphere( GetAbsOrigin(), m_flRadius );
	claim.iszDoor = m_iszDoor;
	claim.iszDoor2 = m_iszDoor2;
	claim.bCloseDoors = m_bCloseDoors;
	claim.pOnClaimed = &m_OnClaimed;
	claim.pOnFailed = &m_OnClaimFailed;
	return Survival_FinishClaim( claim );
}

void CInfoSurvivalSafehouse::InputClaim( inputdata_t &inputdata )
{
	ClaimFrom( dynamic_cast<CHL2_Player *>( inputdata.pActivator ) );
}

class CTriggerSurvivalSafehouse : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerSurvivalSafehouse, CBaseTrigger );
	DECLARE_DATADESC();

	CTriggerSurvivalSafehouse()
	{
		m_bRequireClear = true;
		m_bCloseDoors = false;
	}

	void Spawn( void )
	{
		AddSpawnFlags( SF_TRIGGER_ALLOW_CLIENTS );
		BaseClass::Spawn();
		InitTrigger();
	}

	bool ClaimFrom( CHL2_Player *pPlayer );
	void InputClaim( inputdata_t &inputdata );

private:
	bool m_bRequireClear;
	bool m_bCloseDoors;
	string_t m_iszDoor;
	string_t m_iszDoor2;
	COutputEvent m_OnClaimed;
	COutputEvent m_OnClaimFailed;
};

BEGIN_DATADESC( CTriggerSurvivalSafehouse )
	DEFINE_KEYFIELD( m_bRequireClear, FIELD_BOOLEAN, "require_clear" ),
	DEFINE_KEYFIELD( m_bCloseDoors, FIELD_BOOLEAN, "close_doors" ),
	DEFINE_KEYFIELD( m_iszDoor, FIELD_STRING, "door" ),
	DEFINE_KEYFIELD( m_iszDoor2, FIELD_STRING, "door2" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Claim", InputClaim ),
	DEFINE_OUTPUT( m_OnClaimed, "OnClaimed" ),
	DEFINE_OUTPUT( m_OnClaimFailed, "OnClaimFailed" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_survival_safehouse, CTriggerSurvivalSafehouse );

bool CTriggerSurvivalSafehouse::ClaimFrom( CHL2_Player *pPlayer )
{
	Vector mins, maxs;
	CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );

	bool bCheck = m_bRequireClear && sv_survival_safehouse_require_clear.GetBool();
	SurvivalClaimParams_t claim;
	claim.pEnt = this;
	claim.pPlayer = pPlayer;
	claim.vecMarker = WorldSpaceCenter();
	claim.bHostiles = bCheck && Survival_HostileInAABB( mins, maxs );
	claim.iszDoor = m_iszDoor;
	claim.iszDoor2 = m_iszDoor2;
	claim.bCloseDoors = m_bCloseDoors;
	claim.pOnClaimed = &m_OnClaimed;
	claim.pOnFailed = &m_OnClaimFailed;
	return Survival_FinishClaim( claim );
}

void CTriggerSurvivalSafehouse::InputClaim( inputdata_t &inputdata )
{
	ClaimFrom( dynamic_cast<CHL2_Player *>( inputdata.pActivator ) );
}

static bool Survival_PlayerInTrigger( CTriggerSurvivalSafehouse *pTrigger, CHL2_Player *pPlayer )
{
	if ( !pTrigger || !pPlayer )
		return false;
	if ( pTrigger->IsTouching( pPlayer ) )
		return true;

	Vector mins, maxs;
	pTrigger->CollisionProp()->WorldSpaceSurroundingBounds( &mins, &maxs );
	Vector pt = pPlayer->GetAbsOrigin();
	return ( pt.x >= mins.x && pt.x <= maxs.x && pt.y >= mins.y && pt.y <= maxs.y && pt.z >= mins.z && pt.z <= maxs.z );
}

static void CC_SurvivalClaimSafehouse( const CCommand & )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( UTIL_GetCommandClient() );
	if ( !pPlayer )
	{
		Msg( "survival_claim_safehouse: must be run by a player.\n" );
		return;
	}

	CBaseEntity *pEnt = NULL;
	CTriggerSurvivalSafehouse *pBestTrigger = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "trigger_survival_safehouse" ) ) != NULL )
	{
		CTriggerSurvivalSafehouse *pTrigger = dynamic_cast<CTriggerSurvivalSafehouse *>( pEnt );
		if ( pTrigger && Survival_PlayerInTrigger( pTrigger, pPlayer ) )
		{
			pBestTrigger = pTrigger;
			break;
		}
	}

	if ( pBestTrigger )
	{
		pBestTrigger->ClaimFrom( pPlayer );
		return;
	}

	CInfoSurvivalSafehouse *pBest = NULL;
	float flBest = 1.0e9f;
	pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "info_survival_safehouse" ) ) != NULL )
	{
		CInfoSurvivalSafehouse *pHouse = dynamic_cast<CInfoSurvivalSafehouse *>( pEnt );
		if ( !pHouse )
			continue;

		float flDist = pPlayer->GetAbsOrigin().DistTo( pHouse->GetAbsOrigin() );
		if ( flDist > pHouse->GetRadius() || flDist >= flBest )
			continue;

		flBest = flDist;
		pBest = pHouse;
	}

	if ( !pBest )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_SafehouseNone" );
		return;
	}

	pBest->ClaimFrom( pPlayer );
}

static ConCommand survival_claim_safehouse( "survival_claim_safehouse", CC_SurvivalClaimSafehouse, "Claim the safehouse you are standing in. Locks named doors after the area is clear." );

#define SURVIVAL_STASH_PERSONAL_SLOTS 4

class CSurvivalStash : public CBaseAnimating
{
public:
	DECLARE_CLASS( CSurvivalStash, CBaseAnimating );
	DECLARE_DATADESC();

	CSurvivalStash()
	{
		m_bShared = true;
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

		for ( int i = 0; i < SURVIVAL_STASH_SLOTS; i++ )
			m_nShared[i] = Survival_PackSlot( SURVIVAL_ITEM_EMPTY, 0 );
		for ( int p = 0; p <= MAX_PLAYERS; p++ )
		{
			for ( int i = 0; i < SURVIVAL_STASH_PERSONAL_SLOTS; i++ )
				m_nPersonal[p][i] = Survival_PackSlot( SURVIVAL_ITEM_EMPTY, 0 );
		}
	}

	int ObjectCaps( void )
	{
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		Withdraw( dynamic_cast<CHL2_Player *>( ToBasePlayer( pActivator ) ) );
	}

	void Deposit( CHL2_Player *pPlayer );
	void Withdraw( CHL2_Player *pPlayer );

private:
	int *SlotsFor( CHL2_Player *pPlayer, int &nCount );

	bool m_bShared;
	int m_nShared[SURVIVAL_STASH_SLOTS];
	int m_nPersonal[MAX_PLAYERS + 1][SURVIVAL_STASH_PERSONAL_SLOTS];
};

BEGIN_DATADESC( CSurvivalStash )
	DEFINE_KEYFIELD( m_bShared, FIELD_BOOLEAN, "shared" ),
	DEFINE_ARRAY( m_nShared, FIELD_INTEGER, SURVIVAL_STASH_SLOTS ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( survival_stash, CSurvivalStash );
PRECACHE_REGISTER( survival_stash );

int *CSurvivalStash::SlotsFor( CHL2_Player *pPlayer, int &nCount )
{
	if ( !m_bShared && pPlayer )
	{
		int nSlot = pPlayer->entindex();
		if ( nSlot >= 1 && nSlot <= MAX_PLAYERS )
		{
			nCount = SURVIVAL_STASH_PERSONAL_SLOTS;
			return m_nPersonal[nSlot];
		}
	}

	nCount = SURVIVAL_STASH_SLOTS;
	return m_nShared;
}

void CSurvivalStash::Deposit( CHL2_Player *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->Survival_IsDowned() )
		return;

	int nCount = 0;
	int *pSlots = SlotsFor( pPlayer, nCount );
	int nEmpty = -1;
	for ( int i = 0; i < nCount; i++ )
	{
		if ( Survival_SlotType( pSlots[i] ) == SURVIVAL_ITEM_EMPTY )
		{
			nEmpty = i;
			break;
		}
	}

	if ( nEmpty < 0 )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashFull" );
		return;
	}

	int nType = 0;
	int nAmount = 0;
	if ( !pPlayer->SurvivalInventory_RemoveFirst( 0, nType, nAmount ) )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_GiveNone" );
		return;
	}

	pSlots[nEmpty] = Survival_PackSlot( nType, nAmount );
	ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashPut", Survival_ItemToken( nType ) );
}

void CSurvivalStash::Withdraw( CHL2_Player *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	int nCount = 0;
	int *pSlots = SlotsFor( pPlayer, nCount );
	for ( int i = 0; i < nCount; i++ )
	{
		int nType = Survival_SlotType( pSlots[i] );
		if ( nType == SURVIVAL_ITEM_EMPTY )
			continue;

		int nAmount = Survival_SlotAmount( pSlots[i] );
		if ( !pPlayer->SurvivalInventory_Add( nType, nAmount ) )
		{
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashBackpack" );
			return;
		}

		pSlots[i] = Survival_PackSlot( SURVIVAL_ITEM_EMPTY, 0 );
		CPASAttenuationFilter filter( pPlayer, "ItemBattery.Touch" );
		EmitSound( filter, pPlayer->entindex(), "ItemBattery.Touch" );
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashTake", Survival_ItemToken( nType ) );
		return;
	}

	ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashEmpty" );
}

static CSurvivalStash *Survival_FindStash( CHL2_Player *pPlayer )
{
	if ( !pPlayer )
		return NULL;

	Vector vecStart = pPlayer->EyePosition();
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );

	trace_t tr;
	UTIL_TraceLine( vecStart, vecStart + vecForward * 128.0f, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	CSurvivalStash *pHit = dynamic_cast<CSurvivalStash *>( tr.m_pEnt );
	if ( pHit )
		return pHit;

	CBaseEntity *pEnt = NULL;
	CSurvivalStash *pBest = NULL;
	float flBest = 128.0f;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "survival_stash" ) ) != NULL )
	{
		float flDist = pEnt->GetAbsOrigin().DistTo( pPlayer->GetAbsOrigin() );
		if ( flDist >= flBest )
			continue;
		CSurvivalStash *pStash = dynamic_cast<CSurvivalStash *>( pEnt );
		if ( !pStash )
			continue;
		flBest = flDist;
		pBest = pStash;
	}
	return pBest;
}

static void CC_SurvivalStashPut( const CCommand & )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( UTIL_GetCommandClient() );
	if ( !pPlayer )
	{
		Msg( "survival_stash_put: must be run by a player.\n" );
		return;
	}

	CSurvivalStash *pStash = Survival_FindStash( pPlayer );
	if ( !pStash )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashNone" );
		return;
	}

	pStash->Deposit( pPlayer );
}

static void CC_SurvivalStashTake( const CCommand & )
{
	CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( UTIL_GetCommandClient() );
	if ( !pPlayer )
	{
		Msg( "survival_stash_take: must be run by a player.\n" );
		return;
	}

	CSurvivalStash *pStash = Survival_FindStash( pPlayer );
	if ( !pStash )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_StashNone" );
		return;
	}

	pStash->Withdraw( pPlayer );
}

static ConCommand survival_stash_put( "survival_stash_put", CC_SurvivalStashPut, "Store one backpack item in the survival_stash you are looking at." );
static ConCommand survival_stash_take( "survival_stash_take", CC_SurvivalStashTake, "Take one item from the nearest survival_stash. +use does the same." );

class CSurvivalCot : public CBaseAnimating
{
public:
	DECLARE_CLASS( CSurvivalCot, CBaseAnimating );

	CSurvivalCot()
	{
		m_flNextRest = 0.0f;
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
		if ( gpGlobals->curtime < m_flNextRest )
			return;

		CHL2_Player *pPlayer = dynamic_cast<CHL2_Player *>( ToBasePlayer( pActivator ) );
		if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->Survival_IsDowned() )
			return;

		m_flNextRest = gpGlobals->curtime + 3.0f;
#ifdef HL2MP
		if ( HL2MPRules() )
		{
			HL2MPRules()->Survival_RestBrief( pPlayer );
			return;
		}
#endif
		pPlayer->Survival_Rest();
		ClientPrint( pPlayer, HUD_PRINTCENTER, "#Survival_RestHeal" );
	}

private:
	float m_flNextRest;
};

LINK_ENTITY_TO_CLASS( survival_cot, CSurvivalCot );
PRECACHE_REGISTER( survival_cot );
