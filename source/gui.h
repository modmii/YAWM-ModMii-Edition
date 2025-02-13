#ifndef _GUI_H_
#define _GUI_H_

typedef struct { u16 x, y; } point;
typedef struct GuiWindow {
	const void  *pngData;
	point        pngTarget;
	point        consoleTarget; // top left
	u16          consoleWidth, consoleHeight;
} GuiWindow;

/* Prototypes */
s32 Gui_InitConsole(const GuiWindow *);

#endif
