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

void CHudSurvival::DrawNeed( int y, const wchar_t *wszLabel, float flValue, Color col )
{
	flValue = clamp( flValue, 0.0f, 100.0f );
	if ( flValue <= 25.0f )
	{
		col = Color( 220, 50, 50, 255 );
	}

	surface()->DrawSetTextFont( m_hFont );
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextPos( 0, y );
	surface()->DrawPrintText( wszLabel, wcslen( wszLabel ) );

	const int nLabelW = 56;
	const int nNumberW = 24;
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
	surface()->DrawSetTextPos( nBarX + nBarW + 4, y );
	surface()->DrawPrintText( wszNum, wcslen( wszNum ) );
}

void CHudSurvival::Paint()
{
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer || !m_hFont )
		return;

	const int nInvH = 14;
	int nRowH = ( GetTall() - nInvH ) / 3;
	if ( nRowH < 12 )
		nRowH = 12;

	DrawNeed( 0, L"Hambre", pPlayer->m_HL2Local.m_flHunger, Color( 220, 140, 40, 255 ) );
	DrawNeed( nRowH, L"Sed", pPlayer->m_HL2Local.m_flThirst, Color( 80, 170, 230, 255 ) );
	DrawNeed( nRowH * 2, L"Stamina", pPlayer->m_HL2Local.m_flStamina, Color( 90, 200, 90, 255 ) );

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

	wchar_t wszInv[64];
	V_snwprintf( wszInv, ARRAYSIZE( wszInv ), L"Comida %d   Agua %d", nFood, nWater );
	surface()->DrawSetTextFont( m_hFont );
	surface()->DrawSetTextColor( Color( 230, 230, 230, 255 ) );
	surface()->DrawSetTextPos( 0, nRowH * 3 );
	surface()->DrawPrintText( wszInv, wcslen( wszInv ) );

#ifdef HL2MP
	if ( HL2MPRules() )
	{
		float flHour = clamp( HL2MPRules()->Survival_GetHour(), 0.0f, 23.999f );
		int nHour = (int)flHour;
		int nMinute = (int)( ( flHour - nHour ) * 60.0f );
		if ( nMinute > 59 )
			nMinute = 59;

		wchar_t wszClock[16];
		V_snwprintf( wszClock, ARRAYSIZE( wszClock ), L"%02d:%02d", nHour, nMinute );
		surface()->DrawSetTextColor( Color( 230, 220, 160, 255 ) );
		surface()->DrawSetTextPos( 0, nRowH * 3 + nInvH );
		surface()->DrawPrintText( wszClock, wcslen( wszClock ) );
	}
#endif
}

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
