#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <ogc/es.h>
#include <ogc/aes.h>

#include "sys.h"
#include "nand.h"
#include "utils.h"
#include "mload.h"
#include "ehcmodule_elf.h"

/* Variables */
static bool gDisablePRButtons = false;

void __Sys_ResetCallback(__attribute__((unused)) u32 irq, __attribute__((unused)) void *ctx)
{
	/* Reboot console */
	if (!gDisablePRButtons)
		Sys_Reboot();
}

void __Sys_PowerCallback(void)
{
	/* Poweroff console */
	if (!gDisablePRButtons)
		Sys_Shutdown();
}

// we should be gangster and call the real ioctlv 69
// (in all honesty I really don't like the blind copies of otp.c and mini_seeprom.c I added)
bool ES_CheckHasKoreanKey(void)
{

	ATTRIBUTE_ALIGN(0x20)
	unsigned char data[16] = { 0x56, 0x52, 0x6f, 0x63, 0xa1, 0x2c, 0xd1, 0x32, 0x07, 0x99, 0x82, 0x3b, 0x1b, 0x08, 0x17, 0xd0 };
	u32 iv[4] = {};

	int ret = ES_Decrypt(11, iv, data, sizeof(data), data);

	//	return (!strcmp((char*) data, "thepikachugamer")) Just remembered that this is how the Trucha bug came to be
	return (ret == 0) && (!memcmp(data, "thepikachugamer", sizeof(data)));
}

bool Sys_CanLoadIOS(int ios)
{
	int ret;
	u32 tmd_size = 0;
	u32 boot2version = 0;
	tmd_view *view;

	ES_GetBoot2Version(&boot2version);
	if ((boot2version >= 5) && (ios == 202 || ios == 222 || ios == 223 || ios == 224))
		return true;

	ret = ES_GetTMDViewSize(0x0000000100000000ULL | ios, &tmd_size);
	if (ret < 0)
		return true;

	view = memalign32(tmd_size);
	if (!view)
		return true;

	ret = ES_GetTMDView(0x0000000100000000ULL | ios, (u8 *)view, tmd_size);
	if (ret < 0)
	{
		free(view);
		return true;
	}
	/*Stubs have a few things in common:
	- title version : it is mostly 65280 , or even better : in hex the last 2 digits are 0.
			example : IOS 60 rev 6400 = 0x1900 = 00 = stub
	- exception for IOS21 which is active, the tmd size is 592 bytes (or 140 with the views)
	- the stub ios' have 1 app of their own (type 0x1) and 2 shared apps (type 0x8001).
	eventho the 00 check seems to work fine , we'll only use other knowledge as well cause some
	people/applications install an ios with a stub rev >_> ...*/

	u16 title_version = view->title_version;
	free(view);

	if ((boot2version >= 5) && (ios == 249 || ios == 250) && (title_version < 18))
		return true;

	if ((ios == 202 || ios == 222 || ios == 223 || ios == 224) && (title_version < 4))
		return true;

	if ((title_version & 0xFF) == 0 && (ios < 200 || title_version == 0xFF00))
		return true;

	return false;
}

int Sys_LoadIOS(int ios)
{
	if (Sys_CanLoadIOS(ios))
		return -1;

	mload_close();
	int ret = IOS_ReloadIOS(ios);
	if (ret < 0)
		return ret;

	usleep(100000); // there should be a more professional way to do this

	if (IOS_GetVersion() != 249 && IOS_GetVersion() != 250)
	{
		if (mload_init() >= 0)
		{
			data_elf my_data_elf;
			mload_elf((void *)ehcmodule_elf, &my_data_elf);
			mload_run_thread(my_data_elf.start, my_data_elf.stack, my_data_elf.size_stack, 0x47);
		}
	}

	return ret;
}

void Sys_Init(void)
{
	/* Set RESET/POWER button callback */
	SYS_SetResetCallback(__Sys_ResetCallback);
	SYS_SetPowerCallback(__Sys_PowerCallback);
}

void Sys_Reboot(void)
{
	/* Restart console */
	STM_RebootSystem();
}

void Sys_Shutdown(void)
{
	/* Poweroff console */
#if 0
	/*
	STM_SetLedMode is causing a crash, and I don't even like the concept of shutting down to WiiConnect24 standby anyways,
	so let's just remove this overall

	Optimally we add a WII_Shutdown to wiilaunch.c in libogc to load system menu to shutdown. That would be cool.

	- thepikachugamer
	*/
	if (CONF_GetShutdownMode() == CONF_SHUTDOWN_IDLE)
	{
		s32 ret;

		/* Set LED mode */
		ret = CONF_GetIdleLedMode();
		if (ret >= 0 && ret <= 2)
			STM_SetLedMode(ret);

		/* Shutdown to idle */
		STM_ShutdownToIdle();
	}
#endif

	/* Shutdown to standby */
	STM_ShutdownToStandby();

}

#if 0
// this is a title.c thing
// also we're a wad manager we don't need to be reading the system certificate store
s32 Sys_GetCerts(signed_blob **certs, u32 *len)
{
	static signed_blob certificates[CERTS_LEN] ATTRIBUTE_ALIGN(32);

	s32 fd, ret;

	/* Open certificates file */
	fd = IOS_Open(certs_fs, 1);
	if (fd < 0)
		return fd;

	/* Read certificates */
	ret = IOS_Read(fd, certificates, sizeof(certificates));

	/* Close file */
	IOS_Close(fd);

	/* Set values */
	if (ret > 0)
	{
		*certs = certificates;
		*len = sizeof(certificates);
	}

	return ret;
}
#endif

void SetPRButtons(bool enabled)
{
	gDisablePRButtons = !enabled;
}
