#include "StdAfx.h"

#include "Timer.h"
#include "Patterns.h"
#include "Common.h"
#include "Common_ddraw.h"
#include "General.h"

#include <memory>

struct RsGlobalType
{
	const char*		AppName;
	unsigned int	unkWidth, unkHeight;
	signed int		MaximumWidth;
	signed int		MaximumHeight;
	unsigned int	frameLimit;
	BOOL			quit;
	void*			ps;
	void*			keyboard;
	void*			mouse;
	void*			pad;
};

struct RwV2d
{
    float x;   /**< X value*/
    float y;   /**< Y value */
};

static const void*		RosieAudioFix_JumpBack;

static void (*PrintString)(float,float,const wchar_t*);

static RsGlobalType*	RsGlobal;
static const void*		SubtitlesShadowFix_JumpBack;

inline float GetWidthMult()
{
	static const float&		ResolutionWidthMult = **AddressByVersion<float**>(0x5FA15E, 0x5FA17E, 0x5F9DBE);
	return ResolutionWidthMult;
}

inline float GetHeightMult()
{
	static const float&		ResolutionHeightMult = **AddressByVersion<float**>(0x5FA148, 0x5FA168, 0x5F9DA8);
	return ResolutionHeightMult;
}


void __declspec(naked) RosiesAudioFix()
{
	_asm
	{
		mov     byte ptr [ebx+0CCh], 0
		mov     byte ptr [ebx+148h], 0
		jmp		[RosieAudioFix_JumpBack]
	}
}

static bool bGameInFocus = true;

static LRESULT (CALLBACK **OldWndProc)(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( uMsg )
	{
	case WM_KILLFOCUS:
		bGameInFocus = false;
		break;
	case WM_SETFOCUS:
		bGameInFocus = true;
		break;
	}

	return (*OldWndProc)(hwnd, uMsg, wParam, lParam);
}
static auto* const pCustomWndProc = CustomWndProc;

static void (* const RsMouseSetPos)(RwV2d*) = AddressByVersion<void(*)(RwV2d*)>(0x6030C0, 0x6030A0, 0x602CE0);
static void (*orgConstructRenderList)();
void ResetMousePos()
{
	if ( bGameInFocus )
	{
		RwV2d	vecPos = { RsGlobal->MaximumWidth * 0.5f, RsGlobal->MaximumHeight * 0.5f };
		RsMouseSetPos(&vecPos);
	}
	orgConstructRenderList();
}

void __stdcall Recalculate(float& fX, float& fY, signed int nShadow)
{
	fX = nShadow * GetWidthMult() * RsGlobal->MaximumWidth;
	fY = nShadow * GetHeightMult() * RsGlobal->MaximumHeight;
}

static void AlteredPrintString_Internal(float fX, float fY, float fMarginX, float fMarginY, const wchar_t* pText)
{
	PrintString(fX - fMarginX + (fMarginX * GetWidthMult() * RsGlobal->MaximumWidth), fY - fMarginY + (fMarginY * GetHeightMult() * RsGlobal->MaximumHeight), pText);
}

template<uintptr_t pFltX, uintptr_t pFltY>
void AlteredPrintString(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginX = **reinterpret_cast<float**>(pFltX);
	const float	fMarginY = **reinterpret_cast<float**>(pFltY);
	AlteredPrintString_Internal(fX, fY, fMarginX, fMarginY, pText);
}

template<uintptr_t pFltX, uintptr_t pFltY>
void AlteredPrintStringMinus(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginX = **reinterpret_cast<float**>(pFltX);
	const float	fMarginY = **reinterpret_cast<float**>(pFltY);
	AlteredPrintString_Internal(fX, fY, -fMarginX, -fMarginY, pText);
}

template<uintptr_t pFltX>
void AlteredPrintStringXOnly(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginX = **reinterpret_cast<float**>(pFltX);
	AlteredPrintString_Internal(fX, fY, fMarginX, 0.0f, pText);
}

template<uintptr_t pFltY>
void AlteredPrintStringYOnly(float fX, float fY, const wchar_t* pText)
{
	const float	fMarginY = **reinterpret_cast<float**>(pFltY);
	AlteredPrintString_Internal(fX, fY, 0.0f, fMarginY, pText);

}

float FixedRefValue()
{
	return 1.0f;
}

void __declspec(naked) SubtitlesShadowFix()
{
	_asm
	{
		mov		[esp], eax
		fild	[esp]
		push	eax
		lea		eax, [esp+20h-18h]
		push	eax
		lea		eax, [esp+24h-14h]
		push	eax
		call	Recalculate
		jmp		SubtitlesShadowFix_JumpBack
	}
}

extern char** ppUserFilesDir = AddressByVersion<char**>(0x6022AA, 0x60228A, 0x601ECA);

static LARGE_INTEGER	FrameTime;
int32_t GetTimeSinceLastFrame()
{
	LARGE_INTEGER	curTime;
	QueryPerformanceCounter(&curTime);
	return int32_t(curTime.QuadPart - FrameTime.QuadPart);
}

static int (*RsEventHandler)(int, void*);
int NewFrameRender(int nEvent, void* pParam)
{
	QueryPerformanceCounter(&FrameTime);
	return RsEventHandler(nEvent, pParam);
}


static signed int& LastTimeFireTruckCreated = **AddressByVersion<int**>(0x429435, 0x429435, 0x429405);
static signed int& LastTimeAmbulanceCreated = **AddressByVersion<int**>(0x429449, 0x429449, 0x429419);
static void (*orgCarCtrlReInit)();
void CarCtrlReInit_SilentPatch()
{
	orgCarCtrlReInit();
	LastTimeFireTruckCreated = 0;
	LastTimeAmbulanceCreated = 0;
}

static bool& RespraysAreFree = **AddressByVersion<bool**>(0x430D17, 0x430D17, 0x430CE7);
void GaragesInit_SilentPatch()
{
	RespraysAreFree = false;
}

static void (*orgPickNextNodeToChaseCar)(void*, float, float, void*);
static float PickNextNodeToChaseCarZ = 0.0f;
static void PickNextNodeToChaseCarXYZ( void* vehicle, const CVector& vec, void* chaseTarget )
{
	PickNextNodeToChaseCarZ = vec.z;
	orgPickNextNodeToChaseCar( vehicle, vec.x, vec.y, chaseTarget );
	PickNextNodeToChaseCarZ = 0.0f;
}


static char		aNoDesktopMode[64];

unsigned int __cdecl AutoPilotTimerCalculation_VC(unsigned int nTimer, int nScaleFactor, float fScaleCoef)
{
	return nTimer - static_cast<unsigned int>(nScaleFactor * fScaleCoef);
}

void __declspec(naked) AutoPilotTimerFix_VC()
{
	_asm {
		push	dword ptr[esp + 0xC]
		push	dword ptr[ebx + 0x10]
		push	eax
		call	AutoPilotTimerCalculation_VC
		add 	esp, 0xC
		mov 	[ebx + 0xC], eax
		add     esp, 0x30
		pop     ebp
		pop     ebx
		retn    4
	}
}

static void (__thiscall *orgGiveWeapon)( void* ped, unsigned int weapon, unsigned int ammo, bool flag );
static void __fastcall GiveWeapon_SP( void* ped, void*, unsigned int weapon, unsigned int ammo, bool flag )
{
	if ( ammo == 0 ) ammo = 1;
	orgGiveWeapon( ped, weapon, ammo, flag );
}

void Patch_VC_10(const RECT& desktop)
{
	using namespace Memory;

	PrintString = (void(*)(float,float,const wchar_t*))0x551040;

	RsGlobal = *(RsGlobalType**)0x602D32;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551701;

	Patch<BYTE>(0x43E983, 16);
	Patch<BYTE>(0x43EC03, 16);
	Patch<BYTE>(0x43EECB, 16);
	Patch<BYTE>(0x43F52B, 16);
	Patch<BYTE>(0x43F842, 16);
	Patch<BYTE>(0x48EB27, 16);
	Patch<BYTE>(0x541E7E, 16);

	InjectHook(0x5433BD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x5516FC, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5517C4, 0xD9);
	Patch<BYTE>(0x5517DF, 0xD9);
	Patch<BYTE>(0x551832, 0xD9);
	Patch<BYTE>(0x551848, 0xD9);
	Patch<BYTE>(0x5517E2, 0x34-0x14);
	Patch<BYTE>(0x55184B, 0x34-0x14);
	Patch<BYTE>(0x5517C7, 0x28-0x18);
	Patch<BYTE>(0x551835, 0x24-0x18);
	Patch<BYTE>(0x5516FB, 0x90);

	InjectHook(0x5FA1FD, AlteredPrintString<0x5FA1F6,0x5FA1D5>);
	InjectHook(0x54474D, AlteredPrintStringMinus<0x544727,0x544727>);

	// Mouse fucking fix!
	Patch<DWORD>(0x601740, 0xC3C030);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x6004A2, RsEventHandler );
	InjectHook(0x6004A2, NewFrameRender);
	InjectHook(0x600449, GetTimeSinceLastFrame);


	// RsMouseSetPos call (SA style fix)
	ReadCall( 0x4A5E45, orgConstructRenderList );
	InjectHook(0x4A5E45, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x601727;
	Patch(0x601727, &pCustomWndProc);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x4796E5;
	Patch<const void*>(0x479410 + 0x2E0 + 0x2, sens);
	Patch<const void*>(0x47A20E + 0x27D + 0x2, sens);
	Patch<const void*>(0x47AE27 + 0x1CC + 0x2, sens);
	Patch<const void*>(0x47BE8F + 0x22E + 0x2, sens);
	Patch<const void*>(0x481AB3 + 0x4FE + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<BYTE>(0x47C11E, 0xEB);
	Patch<BYTE>(0x47CD94, 0xEB);
	Nop(0x47C15A, 2);

	// Scan for A/B drives looking for audio files
	Patch<DWORD>(0x5D7941, 'A');
	Patch<DWORD>(0x5D7B04, 'A');


	// ~x~ as cyan blip
	// Shared with GInput
	Patch<BYTE>(0x550481, 0);
	Patch<BYTE>(0x550488, 255);
	Patch<BYTE>(0x55048F, 255);

	Patch<BYTE>(0x5505FF, 0);
	Patch<BYTE>(0x550603, 255);
	Patch<BYTE>(0x550607, 255);

	
	// Corrected crime codes
	Patch<DWORD>(0x5FDDDB, 0xC5);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x4A489B, orgCarCtrlReInit );
	InjectHook(0x4A489B, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	InjectHook(0x4349BB, GaragesInit_SilentPatch, PATCH_JUMP);

	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AED14+1, 1); // katana
	Patch<BYTE>(0x4AEB74+1, 1); // chainsaw

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x418FAE, AutoPilotTimerFix_VC, PATCH_JUMP);


	// Adblocker
#if DISABLE_FLA_DONATION_WINDOW
	if ( *(DWORD*)0x5FFAE9 != 0x006A026A )
	{
		Patch<DWORD>(0x5FFAE9, 0x006A026A);
		Patch<WORD>(0x5FFAED, 0x006A);
	}
#endif

	Common::Patches::DDraw_VC_10( desktop, aNoDesktopMode );
}

void Patch_VC_11(const RECT& desktop)
{
	using namespace Memory;

	PrintString = (void(*)(float,float,const wchar_t*))0x551060;

	RsGlobal = *(RsGlobalType**)0x602D12;
	RosieAudioFix_JumpBack = (void*)0x42BFFE;
	SubtitlesShadowFix_JumpBack = (void*)0x551721;

	Patch<BYTE>(0x43E983, 16);
	Patch<BYTE>(0x43EC03, 16);
	Patch<BYTE>(0x43EECB, 16);
	Patch<BYTE>(0x43F52B, 16);
	Patch<BYTE>(0x43F842, 16);
	Patch<BYTE>(0x48EB37, 16);
	Patch<BYTE>(0x541E9E, 16);

	InjectHook(0x5433DD, FixedRefValue);

	InjectHook(0x42BFF7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x55171C, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5517E4, 0xD9);
	Patch<BYTE>(0x5517FF, 0xD9);
	Patch<BYTE>(0x551852, 0xD9);
	Patch<BYTE>(0x551868, 0xD9);
	Patch<BYTE>(0x551802, 0x34-0x14);
	Patch<BYTE>(0x55186B, 0x34-0x14);
	Patch<BYTE>(0x5517E7, 0x28-0x18);
	Patch<BYTE>(0x551855, 0x24-0x18);
	Patch<BYTE>(0x55171B, 0x90);

	InjectHook(0x5FA21D, AlteredPrintString<0x5FA216,0x5FA1F5>);
	InjectHook(0x54476D, AlteredPrintStringMinus<0x544747,0x544747>);

	// Mouse fucking fix!
	Patch<DWORD>(0x601770, 0xC3C030);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x6004C2, RsEventHandler );
	InjectHook(0x6004C2, NewFrameRender);
	InjectHook(0x600469, GetTimeSinceLastFrame);

	// RsMouseSetPos call (SA style fix)
	ReadCall( 0x4A5E65, orgConstructRenderList );
	InjectHook(0x4A5E65, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x601757;
	Patch(0x601757, &pCustomWndProc);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x4796E5;
	Patch<const void*>(0x479410 + 0x2E0 + 0x2, sens);
	Patch<const void*>(0x47A20E + 0x27D + 0x2, sens);
	Patch<const void*>(0x47AE27 + 0x1CC + 0x2, sens);
	Patch<const void*>(0x47BE8F + 0x22E + 0x2, sens);
	Patch<const void*>(0x481AB3 + 0x4FE + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<BYTE>(0x47C11E, 0xEB);
	Patch<BYTE>(0x47CD94, 0xEB);
	Nop(0x47C15A, 2);

	// Scan for A/B drives looking for audio files
	Patch<DWORD>(0x5D7961, 'A');
	Patch<DWORD>(0x5D7B24, 'A');


	// ~x~ as cyan blip
	// Shared with GInput
	Patch<BYTE>(0x5504A1, 0);
	Patch<BYTE>(0x5504A8, 255);
	Patch<BYTE>(0x5504AF, 255);

	Patch<BYTE>(0x55061F, 0);
	Patch<BYTE>(0x550623, 255);
	Patch<BYTE>(0x550627, 255);


	// Corrected crime codes
	Patch<DWORD>(0x5FDDFB, 0xC5);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x4A48BB, orgCarCtrlReInit );
	InjectHook(0x4A48BB, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	InjectHook(0x4349BB, GaragesInit_SilentPatch, PATCH_JUMP);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AED34+1, 1); // katana
	Patch<BYTE>(0x4AEB94+1, 1); // chainsaw

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x418FAE, AutoPilotTimerFix_VC, PATCH_JUMP);

	Common::Patches::DDraw_VC_11( desktop, aNoDesktopMode );
}

void Patch_VC_Steam(const RECT& desktop)
{
	using namespace Memory;

	PrintString = (void(*)(float,float,const wchar_t*))0x550F30;

	RsGlobal = *(RsGlobalType**)0x602952;
	RosieAudioFix_JumpBack = (void*)0x42BFCE;
	SubtitlesShadowFix_JumpBack = (void*)0x5515F1;

	Patch<BYTE>(0x43E8F3, 16);
	Patch<BYTE>(0x43EB73, 16);
	Patch<BYTE>(0x43EE3B, 16);
	Patch<BYTE>(0x43F49B, 16);
	Patch<BYTE>(0x43F7B2, 16);
	Patch<BYTE>(0x48EA37, 16);
	Patch<BYTE>(0x541D6E, 16);

	InjectHook(0x5432AD, FixedRefValue);

	InjectHook(0x42BFC7, RosiesAudioFix, PATCH_JUMP);

	InjectHook(0x5515EC, SubtitlesShadowFix, PATCH_JUMP);
	Patch<BYTE>(0x5516B4, 0xD9);
	Patch<BYTE>(0x5516CF, 0xD9);
	Patch<BYTE>(0x551722, 0xD9);
	Patch<BYTE>(0x551738, 0xD9);
	Patch<BYTE>(0x5516D2, 0x34-0x14);
	Patch<BYTE>(0x55173B, 0x34-0x14);
	Patch<BYTE>(0x5516B7, 0x28-0x18);
	Patch<BYTE>(0x551725, 0x24-0x18);
	Patch<BYTE>(0x5515EB, 0x90);

	InjectHook(0x5F9E5D, AlteredPrintString<0x5F9E56,0x5F9E35>);
	InjectHook(0x54463D, AlteredPrintStringMinus<0x544617,0x544617>);

	// Mouse fucking fix!
	Patch<DWORD>(0x6013B0, 0xC3C030);

	// (Hopefully) more precise frame limiter
	ReadCall( 0x600102, RsEventHandler );
	InjectHook(0x600102, NewFrameRender);
	InjectHook(0x6000A9, GetTimeSinceLastFrame);

	// RsMouseSetPos call (SA style fix)
	ReadCall( 0x4A5D15, orgConstructRenderList );
	InjectHook(0x4A5D15, ResetMousePos);

	// New wndproc
	OldWndProc = *(LRESULT (CALLBACK***)(HWND, UINT, WPARAM, LPARAM))0x601397;
	Patch(0x601397, &pCustomWndProc);

	// Y axis sensitivity fix
	// By ThirteenAG
	float* sens = *(float**)0x4795C5;
	Patch<const void*>(0x4792F0 + 0x2E0 + 0x2, sens);
	Patch<const void*>(0x47A0EE + 0x27D + 0x2, sens);
	Patch<const void*>(0x47AD07 + 0x1CC + 0x2, sens);
	Patch<const void*>(0x47BD6F + 0x22E + 0x2, sens);
	Patch<const void*>(0x481993 + 0x4FE + 0x2, sens);

	// Don't lock mouse Y axis during fadeins
	Patch<BYTE>(0x47BFFE, 0xEB);
	Patch<BYTE>(0x47CC74, 0xEB);
	Nop(0x47C03A, 2);

	// Scan for A/B drives looking for audio files
	Patch<DWORD>(0x5D7764, 'A');


	// ~x~ as cyan blip
	// Shared with GInput
	Patch<BYTE>(0x550371, 0);
	Patch<BYTE>(0x550378, 255);
	Patch<BYTE>(0x55037F, 255);

	Patch<BYTE>(0x5504EF, 0);
	Patch<BYTE>(0x5504F3, 255);
	Patch<BYTE>(0x5504F7, 255);


	// Corrected crime codes
	Patch<DWORD>(0x5FDA3B, 0xC5);


	// Reinit CCarCtrl fields (firetruck and ambulance generation)
	ReadCall( 0x4A475B, orgCarCtrlReInit );
	InjectHook(0x4A475B, CarCtrlReInit_SilentPatch);


	// Reinit free resprays flag
	InjectHook(0x43497B, GaragesInit_SilentPatch, PATCH_JUMP);


	// Fixed ammo for melee weapons in cheats
	Patch<BYTE>(0x4AEA44+1, 1); // katana
	Patch<BYTE>(0x4AEBE4+1, 1); // chainsaw

	// Fixed crash related to autopilot timing calculations
	InjectHook(0x418FAE, AutoPilotTimerFix_VC, PATCH_JUMP);

	Common::Patches::DDraw_VC_Steam( desktop, aNoDesktopMode );
}

void Patch_VC_JP()
{
	using namespace Memory;

	// Y axis sensitivity fix
	// By ThirteenAG
	Patch<DWORD>(0x4797E7 + 0x2E0 + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47A5E5 + 0x27D + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47B1FE + 0x1CC + 0x2, 0x94ABD8);
	Patch<DWORD>(0x47C266 + 0x22E + 0x2, 0x94ABD8);
	Patch<DWORD>(0x481E8A + 0x4FE + 0x2, 0x94ABD8);
}

void Patch_VC_Common()
{
	using namespace Memory;
	using namespace hook;

	// New timers fix
	{
		auto hookPoint = pattern( "83 E4 F8 89 44 24 08 C7 44 24 0C 00 00 00 00 DF 6C 24 08" ).get_one();
		auto jmpPoint = get_pattern( "DD D8 E9 31 FF FF FF" );

		InjectHook( hookPoint.get<void>( 0x21 ), CTimer::Update_SilentPatch, PATCH_CALL );
		InjectHook( hookPoint.get<void>( 0x21 + 5 ), jmpPoint, PATCH_JUMP );
	}

	// Alt+F4
	{
		auto addr = pattern( "59 59 31 C0 83 C4 70 5D 5F 5E 5B C2 10 00" ).count(2);
		auto dest = get_pattern( "53 55 56 FF B4 24 90 00 00 00 FF 15" );

		addr.for_each_result( [&]( pattern_match match ) {
			InjectHook( match.get<void>( 2 ), dest, PATCH_JUMP );
		});
	}

	// Proper panels damage
	{
		auto addr = pattern( "C6 41 09 03 C6 41 0A 03 C6 41 0B 03" ).get_one();

		// or dword ptr[ecx+14], 3300000h
		// nop
		Patch( addr.get<void>( 0x18 ), { 0x81, 0x49, 0x14, 0x00, 0x00, 0x30, 0x03 }  );
		Nop( addr.get<void>( 0x18 + 7 ), 13 );

		Nop( addr.get<void>( 0x33 ), 7 );
	}

	// Proper metric-imperial conversion constants
	{
		static const float METERS_TO_MILES = 0.0006213711922f;
		auto addr = pattern( "75 ? D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 D9 9C 24" ).count(6);
		addr.for_each_result( [&]( pattern_match match ) {
			Patch<const void*>( match.get<void>( 0x8 + 2 ), &METERS_TO_MILES );
		});

		auto sum = get_pattern( "D9 9C 24 A8 00 00 00 8D 84 24 A8 00 00 00 50", -6 + 2 );
		Patch<const void*>( sum, &METERS_TO_MILES );
	}

	// Improved pathfinding in PickNextNodeAccordingStrategy - PickNextNodeToChaseCar with XYZ coords
	{
		auto addr = pattern( "E8 ? ? ? ? 50 8D 44 24 10 50 E8" ).get_one();
		ReadCall( addr.get<void>( 0x25 ), orgPickNextNodeToChaseCar );

		const uintptr_t funcAddr = (uintptr_t)get_pattern( "8B 9C 24 BC 00 00 00 66 8B B3 A6 01 00 00 66 85 F6", -0xA );
		InjectHook( funcAddr - 5, PickNextNodeToChaseCarXYZ, PATCH_JUMP ); // For plugin-sdk

		// push PickNextNodeToChaseCarZ instead of 0.0f
		// mov ecx, [PickNextNodeToChaseCarZ]
		// mov [esp+0B8h+var_2C], ecx
		Patch( funcAddr + 0x5D, { 0x8B, 0x0D } );
		Patch<const void*>( funcAddr + 0x5D + 2, &PickNextNodeToChaseCarZ );
		Patch( funcAddr + 0x5D + 6, { 0x89, 0x8C, 0x24, 0x8C, 0x00, 0x00, 0x00 } );

		// lea eax, [ecx+edx*4] -> lea eax, [edx+edx*4]
		Patch<uint8_t>( funcAddr + 0x6E + 2, 0x92 );


		// lea eax, [esp+20h+var_10]
		// push eax
		// nop...
		Patch( addr.get<void>( 0x10 ), { 0x83, 0xC4, 0x04, 0x8D, 0x44, 0x24, 0x10, 0x50, 0xEB, 0x0A } );
		InjectHook( addr.get<void>( 0x25 ), PickNextNodeToChaseCarXYZ );
		Patch<uint8_t>( addr.get<void>( 0x2A + 2 ), 0xC );

		// push edx
		// nop...
		Patch<uint8_t>( addr.get<void>( 0x3E ), 0x52 );
		Nop( addr.get<void>( 0x3E + 1 ), 6 );
		InjectHook( addr.get<void>( 0x46 ), PickNextNodeToChaseCarXYZ );
		Patch<uint8_t>( addr.get<void>( 0x4B + 2 ), 0xC );
	}


	// No censorships
	{
		auto addr = get_pattern( "8B 43 50 85 C0 8B 53 50 74 2B 83 E8 01" );
		Patch( addr, { 0x83, 0xC4, 0x08, 0x5B, 0xC3 } );	// add     esp, 8 \ pop ebx \ retn
	}


	// 014C cargen counter fix (by spaceeinstein)
	{
		auto do_processing = pattern( "0F B7 43 28 83 F8 FF 7D 04 66 FF 4B 28" ).get_one();

		Patch<uint8_t>( do_processing.get<uint8_t*>(1), 0xBF ); // movzx   eax, word ptr [ebx+28h] -> movsx   eax, word ptr [ebx+28h]
		Patch<uint8_t>( do_processing.get<uint8_t*>(7), 0x74 ); // jge -> jz
	}


	// Fixed ammo from SCM
	{
		auto give_weapon = get_pattern( "6B C0 2E 6A 01 56 8B 3C", 0x15 );
		ReadCall( give_weapon, orgGiveWeapon );
		InjectHook( give_weapon, GiveWeapon_SP );

		give_weapon = get_pattern( "89 F9 6A 01 55 50 E8", 6 );
		InjectHook( give_weapon, GiveWeapon_SP );
	}

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(hinstDLL);
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		RECT			desktop;
		GetWindowRect(GetDesktopWindow(), &desktop);
		sprintf_s(aNoDesktopMode, "Cannot find %dx%dx32 video mode", desktop.right, desktop.bottom);

		// This scope is mandatory so Protect goes out of scope before rwcseg gets fixed
		{
			std::unique_ptr<ScopedUnprotect::Unprotect> Protect = ScopedUnprotect::UnprotectSectionOrFullModule( GetModuleHandle( nullptr ), ".text" );

			const int8_t version = Memory::GetVersion().version;
			if ( version == 0 ) Patch_VC_10(desktop);
			else if ( version == 1 ) Patch_VC_11(desktop);
			else if ( version == 2 ) Patch_VC_Steam(desktop);

			// Y axis sensitivity only
			else if (*(DWORD*)0x601048 == 0x5E5F5D60) Patch_VC_JP();

			Patch_VC_Common();
			Common::Patches::III_VC_Common();
			Common::Patches::DDraw_Common();
		}

		Common::Patches::FixRwcseg_Patterns();

		HMODULE		hDummyHandle;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&DllMain, &hDummyHandle);
	}
	return TRUE;
}