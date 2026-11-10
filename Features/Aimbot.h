#pragma once

// Priority order:
//   1. chest    (BONE_UPPERTORSO)
//   2. abdomen  (BONE_SPINE1)
//   3. head     (BONE_HEAD cluster)
//   4. everything else
// Lower fBonePriority = higher priority.

static int iBoneList[26]
{
	BONE_UPPERTORSO,        // 0   chest
	BONE_SPINE1,            // 1   abdomen

	BONE_HEAD,              // 2   head
	BONE_HEAD1,             // 3
	BONE_HEAD2,             // 4
	BONE_NECK,              // 5

	BONE_RIGHTUPPERTORSO,   // 6
	BONE_LEFTUPPERTORSO,    // 7
	BONE_RIGHTSHOULDER,     // 8
	BONE_LEFTSHOULDER,      // 9
	BONE_RIGHTELBOW,        // 10
	BONE_LEFTELBOW,         // 11
	BONE_RIGHTWRIST,        // 12
	BONE_LEFTWRIST,         // 13
	BONE_RIGHTHAND,         // 14
	BONE_LEFTHAND,          // 15

	BONE_PELVIS,            // 16
	BONE_PELVIS1,           // 17

	BONE_LEFTHIP,           // 18
	BONE_RIGHTHIP,          // 19
	BONE_LEFTKNEE,          // 20
	BONE_RIGHTKNEE,         // 21
	BONE_LEFTANKLE,         // 22
	BONE_RIGHTANKLE,        // 23
	BONE_LEFTFOOT,          // 24
	BONE_RIGHTFOOT,         // 25
};

static float fBonePriority[26]
{
	0.f,   // BONE_UPPERTORSO      chest      #1
	10.f,  // BONE_SPINE1          abdomen    #2

	20.f,  // BONE_HEAD            head       #3
	20.f,  // BONE_HEAD1
	20.f,  // BONE_HEAD2
	25.f,  // BONE_NECK

	40.f,  // BONE_RIGHTUPPERTORSO
	40.f,  // BONE_LEFTUPPERTORSO
	45.f,  // BONE_RIGHTSHOULDER
	45.f,  // BONE_LEFTSHOULDER
	50.f,  // BONE_RIGHTELBOW
	50.f,  // BONE_LEFTELBOW
	55.f,  // BONE_RIGHTWRIST
	55.f,  // BONE_LEFTWRIST
	60.f,  // BONE_RIGHTHAND
	60.f,  // BONE_LEFTHAND

	70.f,  // BONE_PELVIS
	75.f,  // BONE_PELVIS1

	80.f,  // BONE_LEFTHIP
	80.f,  // BONE_RIGHTHIP
	85.f,  // BONE_LEFTKNEE
	85.f,  // BONE_RIGHTKNEE
	90.f,  // BONE_LEFTANKLE
	90.f,  // BONE_RIGHTANKLE
	95.f,  // BONE_LEFTFOOT
	95.f,  // BONE_RIGHTFOOT
};

class CAimbot
{
public:
	CAimbot()
	{
		iTargetBone = BONE_UPPERTORSO;
		iTargetPlayer = -1;
		bCrosshair = false;
		vecTargetBone = { 0.f, 0.f, 0.f };
		vecCrosshair = { 0.f, 0.f, 0.f };
		iStickyPlayer = -1;
		iStickyBone = -1;

		Orginal_FireInstantHit = (Prototype_FireInstantHit)(pSAMP->g_dwSAMP_Addr + 0xB05A0);
		Orginal_AddBullet = (Prototype_AddBullet)(pSAMP->g_dwSAMP_Addr + 0xA0BB0);
		Orginal_TargetWeaponRangeMultiplier = (Prototype_TargetWeaponRangeMultiplier)0x73B380;
		DetourRestoreAfterWith();
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		pSecure->SDetourAttach(&(PVOID&)Orginal_FireInstantHit, Hooked_FireInstantHit);
		pSecure->SDetourAttach(&(PVOID&)Orginal_AddBullet, Hooked_AddBullet);
		DetourTransactionCommit();
		pSecure->HookInstallCall((DWORD)0x73FF11, (DWORD)Hooked_TargetWeaponRangeMultiplier);
	};

	~CAimbot()
	{
		DetourRestoreAfterWith();
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)Orginal_FireInstantHit, Hooked_FireInstantHit);
		DetourDetach(&(PVOID&)Orginal_AddBullet, Hooked_AddBullet);
		DetourTransactionCommit();
		Memory::memcpy_safe((void*)0x73FF11, "\xE8\x6A\xB4\xFF\xFF", 5);
	};

	void Update();
	void Render();

	CVector vecCrosshair;

private:
	bool bCrosshair;
	int iTargetPlayer, iTargetBone;
	CVector vecTargetBone;

	// sticky lock state — remember last target+bone so we hold lock until cover
	int iStickyPlayer;
	int iStickyBone;

	void GetAimingPlayer();
	void SmoothAimbot();
	void ProAimbot();
	void Triggerbot();

	typedef bool(__stdcall* Prototype_FireInstantHit)(void*, CEntity*, CVector*, CVector*, CEntity*, CVector*, CVector*, bool, bool);
	typedef bool(__cdecl* Prototype_AddBullet)(CEntity*, eWeaponType, CVector, CVector);
	typedef float(__cdecl* Prototype_TargetWeaponRangeMultiplier)(CEntity*, CEntity*);

	static bool __stdcall Hooked_FireInstantHit(void* this_, CEntity* pFiringEntity, CVector* pOrigin, CVector* pMuzzle, CEntity* pTargetEntity, CVector* pTarget, CVector* pVec, bool bCrossHairGun, bool bCreateGunFx);
	static bool __cdecl Hooked_AddBullet(CEntity* pCreator, eWeaponType weaponType, CVector vecPosition, CVector vecVelocity);
	static float __cdecl Hooked_TargetWeaponRangeMultiplier(CEntity* pVictim, CEntity* pOwner);

	Prototype_FireInstantHit Orginal_FireInstantHit;
	Prototype_AddBullet Orginal_AddBullet;
	Prototype_TargetWeaponRangeMultiplier Orginal_TargetWeaponRangeMultiplier;
};

extern CAimbot* pAimbot;