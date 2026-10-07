//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Stock HL2 waves for the survival ambient bed.
//          Every name is referenced by scripts/soundscapes_*.txt shipped
//          with this mod, so the file lives in the HL2 / HL2MP VPKs
//          mounted by gameinfo.txt (hl2_sound_misc / hl2_complete_sound_misc).
//
//=============================================================================//
#ifndef SURVIVAL_AMBIENT_H
#define SURVIVAL_AMBIENT_H
#ifdef _WIN32
#pragma once
#endif

// Looping outdoor wind. soundscapes_strike.txt "strike_outside".
#define SURVIVAL_AMBIENT_BED "ambient/wind/wind1.wav"

// Short gusts. soundscapes_streetwar.txt rndwave lists.
static const char *const g_pszSurvivalAmbientGusts[] =
{
	"ambient/wind/wind_med1.wav",
	"ambient/wind/wind_med2.wav",
	"ambient/wind/wind_hit1.wav",
	"ambient/wind/wind_hit2.wav",
	"ambient/wind/wind_snippet3.wav",
	"ambient/wind/wind_snippet4.wav",
	"ambient/wind/wind_snippet5.wav",
};

// Occasional distant events from the same streetwar / strike lists.
static const char *const g_pszSurvivalAmbientEvents[] =
{
	"ambient/atmosphere/city_skybeam1.wav",
	"ambient/atmosphere/city_skypass1.wav",
	"ambient/levels/streetwar/building_rubble1.wav",
	"ambient/levels/streetwar/building_rubble2.wav",
	"ambient/alarms/apc_alarm_pass1.wav",
	"ambient/alarms/scanner_alert_pass1.wav",
	"ambient/machines/heli_pass1.wav",
	"ambient/levels/streetwar/apc_distant1.wav",
};

inline bool Survival_AmbientFileExists( const char *pszWave )
{
	if ( !pszWave || !pszWave[0] || !filesystem )
		return false;

	char szPath[MAX_PATH];
	Q_snprintf( szPath, sizeof( szPath ), "sound/%s", pszWave );
	return filesystem->FileExists( szPath, "GAME" );
}

#endif // SURVIVAL_AMBIENT_H
