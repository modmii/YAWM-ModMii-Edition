#ifndef _MENU_H_
#define _MENU_H_

/* Prototypes */
void WaitPrompt(const char *prompt);
u32 WaitButtons(void);
void WiiLightControl (int state);
void Menu_Loop(void);
void SetPriiloaderOption(bool enabled);
bool MenuTestDevice();

#endif

