#ifndef _SYS_H_
#define _SYS_H_

#define IS_WIIU (*(vu16*)0xCD8005A0 == 0xCAFE)

/* Prototypes */
void Sys_Init(void);
void Sys_Reboot(void);
void Sys_Shutdown(void);
bool Sys_CanLoadIOS(int ios);
int  Sys_LoadIOS(int ios);
bool ES_CheckHasKoreanKey(void);
void SetPRButtons(bool enabled);

#endif
