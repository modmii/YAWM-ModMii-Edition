#ifndef _GLOBALS_H_
#define _GLOBALS_H_

// Constants
#define CIOS_VERSION		249
#define MAX_FILE_PATH_LEN	1024
#define DEFAULT_WAD_DIRECTORY  "/wad/"

#define MAX_PASSWORD_LENGTH  	16
#define MAX_FAT_DEVICE_LENGTH  	10
#define MAX_NAND_DEVICE_LENGTH  10

#define WM_CONFIG_FILE_PATH "/wad/wm_config.txt"
#define WM_BACKGROUND_PATH "/wad/background.png"

#define FAT_DEVICE_INDEX_INVALID -1
#define NAND_DEVICE_INDEX_INVALID   -1
#define CIOS_VERSION_INVALID        -1

typedef struct wm_config
{
	char password[MAX_PASSWORD_LENGTH];
	char startupPath[256];
	int  cIOSVersion;
	int  fatDeviceIndex;
	int  nandDeviceIndex;
/*
	const char *smbuser;
	const char *smbpassword;
	const char *share;
	const char *ip;
*/
} CONFIG, wm_config_t;


extern wm_config_t gConfig;


#endif
