//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hunger, thirst and stamina bars for the survival mod.
//
//=============================================================================//
#ifndef HUD_SURVIVAL_H
#define HUD_SURVIVAL_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

class CHudSurvival : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudSurvival, vgui::Panel );

public:
	CHudSurvival( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool ShouldDraw( void );

protected:
	virtual void Paint();

private:
	void DrawNeed( int y, const wchar_t *wszLabel, float flValue, Color col, bool bLowIsDanger, bool bSoften = false );

	vgui::HFont m_hFont;
};

#endif // HUD_SURVIVAL_H
