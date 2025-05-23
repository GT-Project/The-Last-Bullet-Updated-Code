/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS(weapon_teterev, CTeterev);
//LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock);


void CTeterev::Spawn()
{
	pev->classname = MAKE_STRING("weapon_teterev"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_TETEREV;
	SET_MODEL(ENT(pev), "models/w_teterev.mdl");

	m_iDefaultAmmo = TETEREV_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CTeterev::Precache(void)
{
	PRECACHE_MODEL("models/v_teterev.mdl");
	PRECACHE_MODEL("models/w_teterev.mdl");
	PRECACHE_MODEL("models/p_teterev.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/teterev/TT_Shoot.wav");//silenced handgun
	PRECACHE_SOUND("weapons/teterev/TT_Shoot.wav");//silenced handgun
	PRECACHE_SOUND("weapons/teterev/TT_Shoot.wav");//handgun

	m_usTeterevFire = PRECACHE_EVENT(1, "events/teterev.sc");
	/*m_usTeterevFire2 = PRECACHE_EVENT(1, "events/makarov2.sc");*/
}

int CTeterev::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ppsh";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 12;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TETEREV;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

BOOL CTeterev::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy("models/v_teterev.mdl", "models/p_teterev.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0);
}

//void CGlock::SecondaryAttack( void )
//{
//	GlockFire( 0.1, 0.2, FALSE );
//}

void CTeterev::PrimaryAttack(void)
{
	TeterevFireAttack(0.01, 0.3, TRUE);
}

void CTeterev::TeterevFireAttack(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usTeterevFire, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CTeterev::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 12)
		return;
	if (m_pPlayer->ammo_ppsh <= 0)
		return;
	
	if (m_iClip == 12)
		return;

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(17, GLOCK_RELOAD, 1.5);
	else
		iResult = DefaultReload(17, GLOCK_RELOAD_NOT_EMPTY, 1.5);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		//SetThink(&CGlock::SpawnClip);
		//pev->nextthink = gpGlobals->time + 0.8;//���������� ����� �������� ����� ������� ������(������� ���������� ��� ��������)
	}
}

//void CGlock::SpawnClip(void)
//{
//	int m_iClip9mm;
//	if (m_iClip == 0)
//	{
//		m_iClip9mm = PRECACHE_MODEL("models/w_9mmclip_empty.mdl");// ������ ������ ��������.
//	}
//	else
//	{
//		m_iClip9mm = PRECACHE_MODEL("models/w_9mmclip.mdl");
//	}
//	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
//	Vector	vecClipVelocity = m_pPlayer->pev->velocity
//		+ gpGlobals->v_right * RANDOM_FLOAT(0, 5)
//		+ gpGlobals->v_up * RANDOM_FLOAT(-10, -15)
//		+ gpGlobals->v_forward * 1;
//	EjectBrass(pev->origin + gpGlobals->v_up * -4 + gpGlobals->v_forward * 1, vecClipVelocity, pev->angles.y, m_iClip9mm, TE_BOUNCE_NULL);//���������� ������ TE_BOUNCE_NULL ������ ��������� ���� ������ ���������, � ����� �������� ���...(���� ������ ����� ������� ������� ���� ��� �������).
//}


void CTeterev::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim(iAnim, 1);
	}
}








//class CTeterevAmmo : public CBasePlayerAmmo
//{
//	void Spawn(void)
//	{
//		Precache();
//		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
//		CBasePlayerAmmo::Spawn();
//	}
//	void Precache(void)
//	{
//		PRECACHE_MODEL("models/w_9mmclip_empty.mdl");
//		PRECACHE_MODEL("models/w_9mmclip.mdl");
//		PRECACHE_SOUND("items/9mmclip1.wav");
//	}
//	BOOL AddAmmo(CBaseEntity* pOther)
//	{
//		if (pOther->GiveAmmo(GLOCK_MAX_CLIP, "makarov", _9MM_MAX_CARRY) != -1)
//		{
//			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
//			return TRUE;
//		}
//		return FALSE;
//	}
//};
//LINK_ENTITY_TO_CLASS(ammo_pm, CTeterevAmmo);
//LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);















