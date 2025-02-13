#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>

#include "gui.h"
#include "video.h"
#include "pngu.h"
#include "globals.h"

#include "background_png.h"

/* Constants */
static const GuiWindow s_defaultBackgroundData = {
	.pngData   = background_png,
	.pngTarget = { 0, 0 },

	.consoleTarget = { 70, 114 },
	.consoleWidth  = 502,
	.consoleHeight = 300,

	// .consoleTarget = { 32, 32 },
	// .consoleWidth  = 640 - 64,
	// .consoleHeight = 480 - 64,
};

s32 Gui_InitConsole(const GuiWindow *window)
{
	s32      ret = 0;
	IMGCTX   ctx = NULL;
	PNGUPROP imgProp;

	if (!window)
		window = &s_defaultBackgroundData;

	if (window->pngData)
	{
		/* Select PNG data */
		ctx = PNGU_SelectImageFromBuffer(window->pngData);
		if (!ctx)
			return -12;

		/* Get image properties */
		ret = PNGU_GetImageProperties(ctx, &imgProp);
		if (ret != PNGU_OK)
			goto out;

		/* Draw image */
		Video_DrawPng(ctx, imgProp, window->pngTarget.x, window->pngTarget.y);
	}


	/* Initialize console */
	Con_Init(window->consoleTarget.x, window->consoleTarget.y, window->consoleWidth, window->consoleHeight);
	// Con_Init(32, 32, 640 - 64, 480 - 64);

out:
	/* Free memory */
	if (ctx)
		PNGU_ReleaseImageContext(ctx);

	return -ret;
}
