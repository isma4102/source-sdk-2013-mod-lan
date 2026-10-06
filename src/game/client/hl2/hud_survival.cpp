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
#ifdef HL2MP
#include "hl2mp_gamerules.h"
#endif
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>

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

void CHudSurvival::DrawNeed( int y, const wchar_t *wszLabel, float flValue, Color col, bool bLowIsDanger )
{
	flValue = clamp( flValue, 0.0f, 100.0f );
	if ( ( bLowIsDanger && flValue <= 25.0f ) || ( !bLowIsDanger && flValue >= 50.0f ) )
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
	DrawNeed( nRowH * 3, Survival_Loc( "#Survival_Infection", L"Infeccion" ), pPlayer->m_HL2Local.m_flInfection, Color( 180, 70, 190, 255 ), false );

	int nFood = 0;
	int nWater = 0;
	for ( int i = 0; i < SURVIVAL_INVENTORY_SLOTS; i++ )
	{
		int nType = Survival_SlotType( pPlayer->m_HL2Local.m_nInventorySlot[i] );
		if ( nType == SURVIVAL_ITEM_FOOD )
			nFood++;
		else if ( nType == SURVIVAL_ITEM_WATER )
			nWater++;
	}

	wchar_t wszFood[8];
	wchar_t wszWater[8];
	wchar_t wszInv[96];
	V_snwprintf( wszFood, ARRAYSIZE( wszFood ), L"%d", nFood );
	V_snwprintf( wszWater, ARRAYSIZE( wszWater ), L"%d", nWater );
	const wchar_t *pInvFmt = g_pVGuiLocalize ? g_pVGuiLocalize->Find( "#Survival_FoodWater" ) : NULL;
	if ( pInvFmt )
		g_pVGuiLocalize->ConstructString( wszInv, sizeof( wszInv ), pInvFmt, 2, wszFood, wszWater );
	else
		V_snwprintf( wszInv, ARRAYSIZE( wszInv ), L"Comida %d   Agua %d", nFood, nWater );
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

		wchar_t wszClock[64];
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
		if ( !pPlayer->m_HL2Local.m_bSurvivalDowned && !pPlayer->m_HL2Local.m_bReviveHint )
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
