#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <ogcsys.h>
#include <ogc/aes.h>
#include <wiilight.h>
#include <wiidrc/wiidrc.h>

#include "sys.h"
#include "title.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "sys.h"
#include "video.h"
#include "wpad.h"
#include "wkb.h"
#include "fat.h"
#include "nand.h"
#include "globals.h"
#include "iospatch.h"
#include "fileops.h"

// Prototypes
extern void __exception_setreload(unsigned sec);

void CheckPassword (void);
void SetDefaultConfig (void);
int ReadConfigFile (void);
int GetIntParam (char *inputStr);
int GetStartupPath (char *startupPath, char *inputStr);
int GetStringParam (char *outParam, char *inputStr, int maxChars);

wm_config_t gConfig;

// Default password Up-Down-Left-Right-Up-Down
//#define PASSWORD "UDLRUD"
void CheckPassword (void)
{
	char curPassword [11]; // Max 10 characters password, NULL terminated
	int count = 0;

	if (strlen (gConfig.password) == 0)
		return;

	// Ask user for a password. Press "B" to restart Wii
	printf("[+] [Enter Password to Continue]:\n\n");

	printf(">>  Press A to continue.\n");
	printf(">>  Press B button to cancel.\n");

	/* Wait for user answer */
	for (;;) 
	{
		u32 buttons = WaitButtons();

		if (buttons & WPAD_BUTTON_A)
		{
			// A button, validate the pw
			curPassword [count] = 0;
			//if (strcmp (curPassword, PASSWORD) == 0)
			if (strcmp (curPassword, gConfig.password) == 0)
			{
				printf(">>  Password Accepted...\n");
				break;
			}
			else
			{
				printf ("\n");
				printf(">>  Incorrect Password. Try again...\n");
				printf("[+] [Enter Password to Continue]:\n\n");
				printf(">>  Press A to continue.\n");
				printf(">>  Press B button to cancel.\n");
				count = 0;
			}
		}
		else if (buttons & WPAD_BUTTON_B)
			// B button, restart
			Restart();
		else
		{
			if (count < 10)
			{
				// Other buttons, build the password
				if (buttons & WPAD_BUTTON_LEFT)
				{
					curPassword [count++] = 'L';
					printf ("*");
				}
				else if (buttons & WPAD_BUTTON_RIGHT)
				{
					curPassword [count++] = 'R';
					printf ("*");
				}
				else if (buttons & WPAD_BUTTON_UP)
				{
					curPassword [count++] = 'U';
					printf ("*");
				}
				else if (buttons & WPAD_BUTTON_DOWN)
				{
					curPassword [count++] = 'D';
					printf ("*");
				}
				else if (buttons & WPAD_BUTTON_1)
				{
					curPassword [count++] = '1';
					printf ("*");
				}
				else if (buttons & WPAD_BUTTON_2)
				{
					curPassword [count++] = '2';
					printf ("*");
				}
			}
		}
	}
}

void Disclaimer(void)
{
	/* Print disclaimer */
	printf("[+] [DISCLAIMER]:\n\n");

	printf("    THIS APPLICATION COMES WITH NO WARRANTY AT ALL,\n");
	printf("    NEITHER EXPRESS NOR IMPLIED.\n");
	printf("    I DO NOT TAKE ANY RESPONSIBILITY FOR ANY DAMAGE IN YOUR\n");
	printf("    WII CONSOLE BECAUSE OF A IMPROPER USAGE OF THIS SOFTWARE.\n\n");

	printf(">>  If you agree, press A button to continue.\n");
	printf(">>  Otherwise, press B button to exit.\n");

	/* Wait for user answer */
	for (;;) {
		//u32 buttons = Wpad_WaitButtons();
		u32 buttons = WaitButtons();

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			break;

		/* B button */
		if (buttons & WPAD_BUTTON_B)
			Restart();
	}
}

int main(int argc, char **argv)
{
	__exception_setreload(15);
	Sys_LoadIOS(IOS_GetVersion());

	/* Initialize display */
	Video_Init();

	/* Initialize console */
	Gui_InitConsole(NULL); // todo: custom background & console config ?

	printf("hello\n");

	s32 ret = IOSPATCH_Apply();
	if (ret != 0) {
		printf("We goofed up.... (%#X)\n", ret);
		sleep(15);
		return ret;
	}

	/* Initialize subsystems */
	Sys_Init();

	/* Initialize Wiimote and GC Controller */
	Wpad_Init();
	PAD_Init();
	WiiDRC_Init();
	WKB_Initialize();
	WIILIGHT_Init();

	FatMount();

	Title_SetupCommonKeys();

	/* Print disclaimer */
	//Disclaimer();
	
	// Set the defaults
	SetDefaultConfig ();

	// Read the config file
	ReadConfigFile();

	// Check password
	CheckPassword();

	/* Menu loop */
	Menu_Loop();

	FatUnmount();

	/* Restart Wii */
	Restart_Wait();

	return 0;
}

int ReadConfigFile()
{
	FILE* fptr;
	char tmpStr[256];
	char path[128];

	if (__system_argv->argvMagic == ARGV_MAGIC) {
		strncpy(path, __system_argv->argv[0], sizeof(path));
		char *fpath = strrchr(path, '/');
		if (fpath)
		{
			strcpy(fpath, "/config.txt");
			if (FSOPFileExists(path))
				goto found_the_file;
		}
	}

	for (int i = 0; i < FatGetDeviceCount(); i++)
	{	
		snprintf(path, sizeof(path), "%s:%s", FatGetDevicePrefix(i), WM_CONFIG_FILE_PATH);
		if (FSOPFileExists(path))
			goto found_the_file;
	}
	
	return -1;

found_the_file:
	// Read the file
	// fptr = fopen(path, "rb"); // umm, why are we opening with mode rb and then using fgets?
	fptr = fopen(path, "r");
	if (!fptr) {
		perror(path);
		return -1;
	}

	// Read the options
	while (true)
	{
		char *option, *value;
		unsigned valueLen;

		if (!fgets(tmpStr, sizeof(tmpStr), fptr))
			break;

		option = tmpStr;
		while (isspace((int)*option))
			option++;

		if (*option == ';' || *option == '#' || *option == '\0')
			continue;

		value = strchr(tmpStr, '=');
		if (!value)
			continue;

		while (isspace((int)*++value))
			;

		if (!*value)
			continue;

		valueLen = (strpbrk(value, "#;\r\n") ?: strchr(value, 0)) - value;
		while (isspace((int)value[valueLen]))
			valueLen--;

		// Get the password
		if (strncasecmp(option, "password", 8) == 0)
		{
			int i;

			// Validate the length
			if (valueLen >= MAX_PASSWORD_LENGTH)
			{
				WaitPrompt("Password is too long....\n");
				continue;
			}

			// Validate the characters
			const char* const validPasswordChars = "UDLR12";
			for (i = 0; i < valueLen; i++) {
				if (!strchr(validPasswordChars, value[i]))
					break;
			}

			if (i != valueLen)
			{
				printf("Password has invalid characters! (?)\n");
				printf("	Valid characters:  %s\n", validPasswordChars);
				printf("	Provided password: %s\n", value);
				printf("	                   %.*s^\n\n", i, "");

				WaitPrompt(NULL);
				continue;
			}

			strncpy(gConfig.password, value, i);
		}

		// Get startup path
		else if (strncasecmp(option, "startupPath", 11) == 0)
		{
			value[valueLen] = '\0';

			if (FSOPFolderExists(value))
				strncpy(gConfig.startupPath, value, sizeof(gConfig.startupPath));
		}

		// cIOS
		else if (strncasecmp(option, "cIOSVersion", 11) == 0)
		{
			char *endPtr;
			long num = strtol(value, &endPtr, 10);

			if (num < 3 || num >= 256)
			{
				printf("Invalid cIOSVersion %lu (%s)\n", num, value);
				WaitPrompt(NULL);
			}

			// Get cIOSVersion
			gConfig.cIOSVersion = num;
		}

		// FatDevice
		else if (strncasecmp(option, "fatDevice", 9) == 0)
		{
			// Get fatDevice
			for (int i = 0; i < FatGetDeviceCount(); i++)
			{
				if (strcmp(FatGetDevicePrefix(i), value) == 0)
					gConfig.fatDeviceIndex = i;
			}
		}

		// NandDevice
		else if (strncasecmp(tmpStr, "NANDDevice", 10) == 0)
		{
			for (int i = 0; i < 3; i++)
			{
				if (strcmp(ndevList[i].name, value) == 0)
					gConfig.nandDeviceIndex = i;
			}
		}
	} // EndWhile
			
	// Close the config file
	fclose (fptr);

	return 0;
} // ReadConfig


void SetDefaultConfig (void)
{
	memset(&gConfig, 0, sizeof(gConfig));
	
	// Default startup folder
	strcpy (gConfig.startupPath, DEFAULT_WAD_DIRECTORY);
	
	gConfig.cIOSVersion = CIOS_VERSION_INVALID;            // Means that user has to select later
	gConfig.fatDeviceIndex = FAT_DEVICE_INDEX_INVALID;     // Means that user has to select
	gConfig.nandDeviceIndex = NAND_DEVICE_INDEX_INVALID;   // Means that user has to select

} // SetDefaultConfig
