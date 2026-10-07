//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hunger, thirst and stamina bars for the survival mod.
//
//=============================================================================//

#include "cbase.h"
#include "hud_survival.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include "hl2/survival_inventory.h"
#include "cliententitylist.h"
#include "engine/IEngineSound.h"
#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/IVGui.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudSurvival );

CHudSurvival::CHudSurvival( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSurvival" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	// Visible without the HEV suit. Death and cl_drawhud still hide it.
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	m_hFont = 0;
}

void CHudSurvival::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetPaintBackgroundEnabled( false );

	m_hFont = pScheme->GetFont( "DefaultSmall", true );
	if ( !m_hFont )
	{
		m_hFont = pScheme->GetFont( "Default", true );
	}
}

bool CHudSurvival::ShouldDraw( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
		return false;

	return CHudElement::ShouldDraw();
}

static const wchar_t *Survival_Loc( const char *pszToken, const wchar_t *pszFallback )
{
	if ( g_pVGuiLocalize )
	{
		const wchar_t *pText = g_pVGuiLocalize->Find( pszToken );
		if ( pText && pText[0] )
			return pText;
	}
	return pszFallback;
}

void CHudSurvival::DrawNeed( int y, const wchar_t *wszLabel, float flValue, Color col, bool bLowIsDanger, bool bSoften )
{
	flValue = clamp( flValue, 0.0f, 100.0f );
	if ( bSoften )
	{
		// Slow breathe. The bar stays purple and only leans red near 100.
		float flT = flValue / 100.0f;
		float flPulse = 0.82f + 0.18f * sinf( gpGlobals->curtime * 1.7f );
		int nAlpha = (int)( 210.0f + 45.0f * flPulse );
		col = Color(
			(int)( 140.0f + 50.0f * flT ),
			(int)( 96.0f - 36.0f * flT ),
			(int)( 170.0f - 70.0f * flT ),
			nAlpha );
	}
	else if ( ( bLowIsDanger && flValue <= 25.0f ) || ( !bLowIsDanger && flValue >= 50.0f ) )
	{
		col = Color( 220, 50, 50, 255 );
	}

	surface()->DrawSetTextFont( m_hFont );
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextPos( 0, y );
	surface()->DrawPrintText( wszLabel, wcslen( wszLabel ) );

	int nTextW = 0;
	int nTextH = 0;
	surface()->GetTextSize( m_hFont, wszLabel, nTextW, nTextH );
	const int nLabelW = nTextW + 8;
	const int nNumberW = 28;
	int nBarX = nLabelW;
	int nBarW = GetWide() - nLabelW - nNumberW;
	int nBarH = 6;
	int nBarY = y + 4;
	if ( nBarW < 8 )
		nBarW = 8;

	int nFilled = (int)( nBarW * ( flValue / 100.0f ) );

	surface()->DrawSetColor( Color( 0, 0, 0, 160 ) );
	surface()->DrawFilledRect( nBarX, nBarY, nBarX + nBarW, nBarY + nBarH );
	surface()->DrawSetColor( col );
	if ( nFilled > 0 )
	{
		surface()->DrawFilledRect( nBarX, nBarY, nBarX + nFilled, nBarY + nBarH );
	}

	wchar_t wszNum[8];
	V_snwprintf( wszNum, ARRAYSIZE( wszNum ), L"%d", (int)flValue );
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextPos( nBarX + nBarW + 4, y );
	surface()->DrawPrintText( wszNum, wcslen( wszNum ) );
}

void CHudSurvival::Paint()
{
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer || !m_hFont )
		return;

	const int nFooter = 28;
	int nRowH = ( GetTall() - nFooter ) / 4;
	if ( nRowH < 12 )
		nRowH = 12;

	DrawNeed( 0, Survival_Loc( "#Survival_Hunger", L"Hambre" ), pPlayer->m_HL2Local.m_flHunger, Color( 220, 140, 40, 255 ), true );
	DrawNeed( nRowH, Survival_Loc( "#Survival_Thirst", L"Sed" ), pPlayer->m_HL2Local.m_flThirst, Color( 80, 170, 230, 255 ), true );
	DrawNeed( nRowH * 2, Survival_Loc( "#Survival_Fatigue", L"Cansancio" ), pPlayer->m_HL2Local.m_flStamina, Color( 90, 200, 90, 255 ), true );
	DrawNeed( nRowH * 3, Survival_Loc( "#Survival_Infection", L"Infeccion" ), pPlayer->m_HL2Local.m_flInfection, Color( 180, 70, 190, 255 ), false, true );

	int nFood = 0;
	int nWater = 0;
	int nAntidote = 0;
	for ( int i = 0; i < SURVIVAL_INVENTORY_SLOTS; i++ )
	{
		int nType = Survival_SlotType( pPlayer->m_HL2Local.m_nInventorySlot[i] );
		if ( nType == SURVIVAL_ITEM_FOOD )
			nFood++;
		else if ( nType == SURVIVAL_ITEM_WATER )
			nWater++;
		else if ( nType == SURVIVAL_ITEM_ANTIDOTE )
			nAntidote++;
	}

	wchar_t wszFood[8];
	wchar_t wszWater[8];
	wchar_t wszAntidote[8];
	wchar_t wszInv[128];
	V_snwprintf( wszFood, ARRAYSIZE( wszFood ), L"%d", nFood );
	V_snwprintf( wszWater, ARRAYSIZE( wszWater ), L"%d", nWater );
	V_snwprintf( wszAntidote, ARRAYSIZE( wszAntidote ), L"%d", nAntidote );
	const wchar_t *pInvFmt = g_pVGuiLocalize ? g_pVGuiLocalize->Find( "#Survival_FoodWater" ) : NULL;
	if ( pInvFmt )
		g_pVGuiLocalize->ConstructString( wszInv, sizeof( wszInv ), pInvFmt, 3, wszFood, wszWater, wszAntidote );
	else
		V_snwprintf( wszInv, ARRAYSIZE( wszInv ), L"Comida %d   Agua %d   Antidoto %d", nFood, nWater, nAntidote );
	surface()->DrawSetTextFont( m_hFont );
	surface()->DrawSetTextColor( Color( 230, 230, 230, 255 ) );
	surface()->DrawSetTextPos( 0, nRowH * 4 );
	surface()->DrawPrintText( wszInv, wcslen( wszInv ) );

#ifdef HL2MP
	if ( HL2MPRules() )
	{
		float flHour = clamp( HL2MPRules()->Survival_GetHour(), 0.0f, 23.999f );
		int nHour = (int)flHour;
		int nMinute = (int)( ( flHour - nHour ) * 60.0f );
		if ( nMinute > 59 )
			nMinute = 59;

		wchar_t wszDayNum[8];
		wchar_t wszDay[48];
		V_snwprintf( wszDayNum, ARRAYSIZE( wszDayNum ), L"%d", HL2MPRules()->Survival_GetDay() );
		const wchar_t *pDayFmt = g_pVGuiLocalize ? g_pVGuiLocalize->Find( "#Survival_Day" ) : NULL;
		if ( pDayFmt )
			g_pVGuiLocalize->ConstructString( wszDay, sizeof( wszDay ), pDayFmt, 1, wszDayNum );
		else
			V_snwprintf( wszDay, ARRAYSIZE( wszDay ), L"Dia %d", HL2MPRules()->Survival_GetDay() );

		wchar_t wszClock[96];
		const wchar_t *pHouse = L"";
		if ( HL2MPRules()->Survival_HasSafehouse() )
			pHouse = Survival_Loc( "#Survival_SafehouseHud", L"Refugio" );
		if ( pHouse[0] )
			V_snwprintf( wszClock, ARRAYSIZE( wszClock ), L"%s  %02d:%02d  %s", wszDay, nHour, nMinute, pHouse );
		else
			V_snwprintf( wszClock, ARRAYSIZE( wszClock ), L"%s  %02d:%02d", wszDay, nHour, nMinute );
		surface()->DrawSetTextColor( Color( 230, 220, 160, 255 ) );
		surface()->DrawSetTextPos( 0, nRowH * 4 + 14 );
		surface()->DrawPrintText( wszClock, wcslen( wszClock ) );
	}
#endif
}

class CHudSurvivalStatus : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudSurvivalStatus, vgui::Panel );

public:
	CHudSurvivalStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSurvivalStatus" )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetHiddenBits( HIDEHUD_PLAYERDEAD );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
		SetPaintBackgroundEnabled( false );
		m_hFont = 0;
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		SetPaintBackgroundEnabled( false );
		m_hFont = pScheme->GetFont( "Default", true );
		SetSize( ScreenWidth(), ScreenHeight() );
	}

	virtual bool ShouldDraw( void )
	{
		C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
		if ( !pPlayer || !pPlayer->IsAlive() )
			return false;
		if ( !pPlayer->m_HL2Local.m_bSurvivalDowned && !pPlayer->m_HL2Local.m_bReviveHint && pPlayer->m_HL2Local.m_flInfection < 20.0f )
			return false;
		return CHudElement::ShouldDraw();
	}

protected:
	virtual void Paint()
	{
		C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
		if ( !pPlayer || !m_hFont )
			return;

		SetSize( ScreenWidth(), ScreenHeight() );
		surface()->DrawSetTextFont( m_hFont );

		float flInf = pPlayer->m_HL2Local.m_flInfection;
		if ( flInf > 20.0f )
		{
			float flPulse = 0.65f + 0.35f * sinf( gpGlobals->curtime * 1.6f );
			int nAlpha = (int)( ( flInf - 20.0f ) / 80.0f * 36.0f * flPulse );
			if ( nAlpha > 42 )
				nAlpha = 42;
			if ( nAlpha > 0 )
			{
				surface()->DrawSetColor( 80, 16, 36, nAlpha );
				surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
			}
		}

		int nCenterX = GetWide() / 2;
		int nY = GetTall() / 2 - 20;

		if ( pPlayer->m_HL2Local.m_bSurvivalDowned )
		{
			const wchar_t *pDown = Survival_Loc( "#Survival_Downed", L"Estas herido..." );
			int nW = 0, nH = 0;
			surface()->GetTextSize( m_hFont, pDown, nW, nH );
			surface()->DrawSetTextColor( Color( 220, 60, 60, 255 ) );
			surface()->DrawSetTextPos( nCenterX - nW / 2, nY );
			surface()->DrawPrintText( pDown, wcslen( pDown ) );

			float flLeft = pPlayer->m_HL2Local.m_flDownedEnds - gpGlobals->curtime;
			if ( flLeft < 0.0f )
				flLeft = 0.0f;
			wchar_t wszSecs[8];
			wchar_t wszTime[64];
			V_snwprintf( wszSecs, ARRAYSIZE( wszSecs ), L"%d", (int)( flLeft + 0.5f ) );
			const wchar_t *pTimeFmt = g_pVGuiLocalize ? g_pVGuiLocalize->Find( "#Survival_DownedTime" ) : NULL;
			if ( pTimeFmt )
				g_pVGuiLocalize->ConstructString( wszTime, sizeof( wszTime ), pTimeFmt, 1, wszSecs );
			else
				V_snwprintf( wszTime, ARRAYSIZE( wszTime ), L"%s s", wszSecs );
			surface()->GetTextSize( m_hFont, wszTime, nW, nH );
			surface()->DrawSetTextPos( nCenterX - nW / 2, nY + nH + 4 );
			surface()->DrawPrintText( wszTime, wcslen( wszTime ) );
			return;
		}

		if ( pPlayer->m_HL2Local.m_bReviveHint )
		{
			const wchar_t *pRevive = Survival_Loc( "#Survival_Revive", L"Revivir" );
			int nW = 0, nH = 0;
			surface()->GetTextSize( m_hFont, pRevive, nW, nH );
			surface()->DrawSetTextColor( Color( 230, 220, 160, 255 ) );
			surface()->DrawSetTextPos( nCenterX - nW / 2, nY );
			surface()->DrawPrintText( pRevive, wcslen( pRevive ) );

			float flProgress = clamp( pPlayer->m_HL2Local.m_flReviveProgress, 0.0f, 1.0f );
			int nBarW = 120;
			int nBarX = nCenterX - nBarW / 2;
			int nBarY = nY + nH + 6;
			surface()->DrawSetColor( Color( 0, 0, 0, 160 ) );
			surface()->DrawFilledRect( nBarX, nBarY, nBarX + nBarW, nBarY + 8 );
			surface()->DrawSetColor( Color( 220, 200, 80, 255 ) );
			int nFilled = (int)( nBarW * flProgress );
			if ( nFilled > 0 )
				surface()->DrawFilledRect( nBarX, nBarY, nBarX + nFilled, nBarY + 8 );
		}
	}

private:
	vgui::HFont m_hFont;
};

DECLARE_HUDELEMENT( CHudSurvivalStatus );

#ifdef HL2MP
static float Survival_NightAlpha( float flHour )
{
	if ( flHour >= 8.0f && flHour < 18.0f )
		return 0.0f;
	if ( flHour >= 20.0f || flHour < 6.0f )
		return 120.0f;
	if ( flHour >= 18.0f )
		return 120.0f * ( flHour - 18.0f ) / 2.0f;
	return 120.0f * ( 8.0f - flHour ) / 2.0f;
}

class CHudSurvivalNight : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudSurvivalNight, vgui::Panel );

public:
	CHudSurvivalNight( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSurvivalNight" )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetHiddenBits( HIDEHUD_PLAYERDEAD );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
		SetPaintBackgroundEnabled( false );
		SetZPos( -100 );
		SetSize( ScreenWidth(), ScreenHeight() );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		SetPaintBackgroundEnabled( false );
		SetSize( ScreenWidth(), ScreenHeight() );
	}

	virtual bool ShouldDraw( void )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() || !HL2MPRules() )
			return false;
		if ( Survival_NightAlpha( HL2MPRules()->Survival_GetHour() ) < 1.0f )
			return false;
		return CHudElement::ShouldDraw();
	}

protected:
	virtual void Paint()
	{
		if ( !HL2MPRules() )
			return;

		int nAlpha = (int)Survival_NightAlpha( HL2MPRules()->Survival_GetHour() );
		if ( nAlpha < 1 )
			return;

		surface()->DrawSetColor( 0, 0, 0, nAlpha );
		surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	}
};

DECLARE_HUDELEMENT_DEPTH( CHudSurvivalNight, 5 );
#endif

ConVar cl_survival_radar( "cl_survival_radar", "1", FCVAR_ARCHIVE, "Draw the survival radar." );
ConVar cl_survival_radar_range( "cl_survival_radar_range", "1500", FCVAR_ARCHIVE, "Radar range, in units." );
ConVar cl_survival_ambient( "cl_survival_ambient", "1", FCVAR_ARCHIVE, "Sparse wind, rumble and distant sirens. 0 silences the client ambient manager." );
ConVar cl_survival_ambient_volume( "cl_survival_ambient_volume", "0.35", FCVAR_ARCHIVE, "Master volume for the ambient manager. Kept low on purpose." );
ConVar cl_survival_ambient_gap( "cl_survival_ambient_gap", "18", FCVAR_ARCHIVE, "Seconds between ambient one-shots. A random extra gap of the same length is added." );

static bool Survival_RadarHostile( const char *pszClass )
{
	if ( !pszClass || !pszClass[0] )
		return false;
	if ( !Q_strnicmp( pszClass, "npc_zombie", 10 ) )
		return true;
	if ( !Q_strnicmp( pszClass, "npc_fastzombie", 14 ) )
		return true;
	if ( !Q_strnicmp( pszClass, "npc_poisonzombie", 16 ) )
		return true;
	if ( !Q_strnicmp( pszClass, "npc_zombine", 11 ) )
		return true;
	if ( !Q_strnicmp( pszClass, "npc_headcrab", 12 ) )
		return true;
	return false;
}

class CHudSurvivalRadar : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudSurvivalRadar, vgui::Panel );

public:
	CHudSurvivalRadar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSurvivalRadar" )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetHiddenBits( HIDEHUD_PLAYERDEAD );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
		SetPaintBackgroundEnabled( false );
		SetZPos( 50 );
		m_hFont = 0;
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		SetPaintBackgroundEnabled( false );
		m_hFont = pScheme->GetFont( "DefaultSmall", true );
		if ( !m_hFont )
			m_hFont = pScheme->GetFont( "Default", true );
	}

	virtual bool ShouldDraw( void )
	{
		if ( !cl_survival_radar.GetBool() )
			return false;
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() )
			return false;
		return CHudElement::ShouldDraw();
	}

protected:
	void DrawBlip( float x, float y, int nSize, Color col )
	{
		int nX = (int)x;
		int nY = (int)y;
		surface()->DrawSetColor( col );
		surface()->DrawFilledRect( nX - nSize, nY - nSize, nX + nSize, nY + nSize );
	}

	bool WorldToRadar( const Vector &vecWorld, const Vector &vecOrigin, float flYaw, float flRange, float &x, float &y, bool &bClamped )
	{
		Vector delta = vecWorld - vecOrigin;
		float flDist = delta.Length2D();
		if ( flDist < 1.0f )
		{
			x = GetWide() * 0.5f;
			y = GetTall() * 0.5f;
			bClamped = false;
			return true;
		}

		float flWorldYaw = RAD2DEG( atan2f( delta.y, delta.x ) );
		float flRel = flWorldYaw - flYaw;
		float flRad = DEG2RAD( flRel );
		float flRadius = ( GetWide() * 0.5f ) - 8.0f;
		if ( flRadius < 8.0f )
			flRadius = 8.0f;
		float flScale = flRadius / flRange;
		float dx = sinf( flRad ) * flDist * flScale;
		float dy = -cosf( flRad ) * flDist * flScale;
		bClamped = false;
		float flLen = sqrtf( dx * dx + dy * dy );
		if ( flLen > flRadius && flLen > 0.0f )
		{
			dx *= flRadius / flLen;
			dy *= flRadius / flLen;
			bClamped = true;
		}

		x = GetWide() * 0.5f + dx;
		y = GetTall() * 0.5f + dy;
		return true;
	}

	virtual void Paint()
	{
		C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
		if ( !pLocal )
			return;

		int nW = GetWide();
		int nH = GetTall();
		if ( nW < 16 || nH < 16 )
			return;

		surface()->DrawSetColor( 0, 0, 0, 140 );
		surface()->DrawFilledRect( 0, 0, nW, nH );
		surface()->DrawSetColor( 180, 180, 160, 180 );
		surface()->DrawOutlinedRect( 0, 0, nW, nH );
		surface()->DrawOutlinedRect( 2, 2, nW - 2, nH - 2 );

		if ( m_hFont )
		{
			const wchar_t *pLabel = Survival_Loc( "#Survival_Radar", L"Radar" );
			surface()->DrawSetTextFont( m_hFont );
			surface()->DrawSetTextColor( 220, 220, 200, 220 );
			surface()->DrawSetTextPos( 6, 4 );
			surface()->DrawPrintText( pLabel, wcslen( pLabel ) );
		}

		float flRange = cl_survival_radar_range.GetFloat();
		if ( flRange < 128.0f )
			flRange = 128.0f;

		Vector vecOrigin = pLocal->GetAbsOrigin();
		float flYaw = pLocal->EyeAngles().y;
		float x, y;
		bool bClamped = false;

		for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
		{
			if ( pEnt == pLocal || pEnt->IsDormant() || !pEnt->IsAlive() )
				continue;
			if ( !Survival_RadarHostile( pEnt->GetClassname() ) )
				continue;
			if ( pEnt->GetAbsOrigin().DistTo( vecOrigin ) > flRange )
				continue;
			WorldToRadar( pEnt->GetAbsOrigin(), vecOrigin, flYaw, flRange, x, y, bClamped );
			DrawBlip( x, y, 2, Color( 210, 50, 50, 230 ) );
		}

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			C_BasePlayer *pOther = UTIL_PlayerByIndex( i );
			if ( !pOther || pOther == pLocal || !pOther->IsAlive() || pOther->IsDormant() )
				continue;
			WorldToRadar( pOther->GetAbsOrigin(), vecOrigin, flYaw, flRange, x, y, bClamped );
			Color col = bClamped ? Color( 80, 140, 80, 180 ) : Color( 80, 220, 90, 230 );
			DrawBlip( x, y, 3, col );
		}

#ifdef HL2MP
		if ( HL2MPRules() && HL2MPRules()->Survival_HasSafehouse() )
		{
			WorldToRadar( HL2MPRules()->Survival_GetSafehouseOrigin(), vecOrigin, flYaw, flRange, x, y, bClamped );
			Color col = bClamped ? Color( 220, 180, 60, 160 ) : Color( 240, 210, 80, 240 );
			DrawBlip( x, y, bClamped ? 3 : 4, col );
		}
#endif

		DrawBlip( nW * 0.5f, nH * 0.5f, 2, Color( 240, 240, 240, 255 ) );
		surface()->DrawSetColor( 240, 240, 240, 180 );
		surface()->DrawFilledRect( (int)( nW * 0.5f ) - 1, (int)( nH * 0.5f ) - 10, (int)( nW * 0.5f ) + 1, (int)( nH * 0.5f ) - 4 );
	}

private:
	vgui::HFont m_hFont;
};

DECLARE_HUDELEMENT( CHudSurvivalRadar );

class CHudSurvivalAmbient : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudSurvivalAmbient, vgui::Panel );

public:
	CHudSurvivalAmbient( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSurvivalAmbient" )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
		m_flNext = 0.0f;
		m_nPick = 0;
		vgui::ivgui()->AddTickSignal( GetVPanel(), 500 );
	}

	virtual bool ShouldDraw( void )
	{
		return false;
	}

	virtual void OnTick( void )
	{
		if ( !cl_survival_ambient.GetBool() || !enginesound )
			return;

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer || !pPlayer->IsAlive() )
			return;
		if ( gpGlobals->curtime < m_flNext )
			return;

		// Stock HL2 waves. No Spanish VO ships with the SDK, so these stay wordless.
		static const char *s_pszWind[] =
		{
			"ambient/wind/wind_snippet3.wav",
			"ambient/wind/wind_snippet4.wav",
			"ambient/wind/wind_snippet5.wav",
			"ambient/wind/wind_med1.wav",
			"ambient/wind/wind_hit1.wav",
		};
		static const char *s_pszFar[] =
		{
			"ambient/atmosphere/city_skybeam1.wav",
			"ambient/atmosphere/city_skypass1.wav",
			"ambient/levels/streetwar/building_rubble1.wav",
			"ambient/alarms/apc_alarm_pass1.wav",
		};

		const char *psz = s_pszWind[m_nPick % ARRAYSIZE( s_pszWind )];
		float flVol = 0.22f;
		// About one event in five is a distant siren or a rumble, never a loop.
		if ( ( m_nPick % 5 ) == 4 )
		{
			psz = s_pszFar[( m_nPick / 5 ) % ARRAYSIZE( s_pszFar )];
			flVol = ( Q_stristr( psz, "alarm" ) != NULL ) ? 0.12f : 0.16f;
		}
		m_nPick++;

		float flMaster = cl_survival_ambient_volume.GetFloat();
		if ( flMaster < 0.0f )
			flMaster = 0.0f;
		if ( flMaster > 1.0f )
			flMaster = 1.0f;
		enginesound->EmitAmbientSound( psz, flVol * flMaster, 96 + ( m_nPick % 9 ) );

		float flGap = cl_survival_ambient_gap.GetFloat();
		if ( flGap < 6.0f )
			flGap = 6.0f;
		m_flNext = gpGlobals->curtime + flGap + random->RandomFloat( 0.0f, flGap );
	}

private:
	float m_flNext;
	int m_nPick;
};

DECLARE_HUDELEMENT( CHudSurvivalAmbient );
