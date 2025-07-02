// Must be defined in one file, _before_ #include "clay.h"
#define CLAY_IMPLEMENTATION

#include "../../clay.h"
#include "../../renderers/x11/clay_renderer_xcb.c"
#include "../shared-layouts/clay-video-demo.c"

#include <stdio.h>
#include <stdarg.h>

const Clay_Color COLOR_LIGHT = (Clay_Color) {224, 215, 210, 255};
const Clay_Color COLOR_RED = (Clay_Color) {168, 66, 28, 255};
const Clay_Color COLOR_ORANGE = (Clay_Color) {225, 138, 50, 255};

typedef struct {
	int width, height;
	ClayVideoDemo_Data demoData;
	Clay_XCBRendererData rendererData;
	Clay_Arena clayArena;
} AppState;

void HandleClayErrors(Clay_ErrorData errorData)
{
	printf("%s", errorData.errorText.chars);
}

static void die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(1);
}

int main(void)
{
	const char font_name[] = "fixed";
	AppState app = {.width = 640, .height = 480};

	// open connection
	app.rendererData.connection = xcb_connect(NULL, NULL);
	if (!app.rendererData.connection)
		die("couldn't connect to an X server");

	// get the current screen
	const xcb_setup_t *setup = xcb_get_setup(app.rendererData.connection);
	app.rendererData.screen = xcb_setup_roots_iterator(setup).data;
	if (!app.rendererData.screen)
		die("couldn't get the current screen");

	// load font
	app.rendererData.fonts[0] = xcb_generate_id(app.rendererData.connection);
	xcb_open_font(app.rendererData.connection, app.rendererData.fonts[0], sizeof(font_name), font_name);

	// create graphics context
	app.rendererData.gcontext = xcb_generate_id(app.rendererData.connection);
	xcb_create_gc(
		app.rendererData.connection,
		app.rendererData.gcontext,
		app.rendererData.screen->root,
		XCB_GC_FOREGROUND | XCB_GC_FONT | XCB_GC_GRAPHICS_EXPOSURES,
		(uint32_t[]){app.rendererData.screen->black_pixel, app.rendererData.fonts[0], 0}
	);

	// create a window
	app.rendererData.window = xcb_generate_id(app.rendererData.connection);
	xcb_create_window(
		app.rendererData.connection,
		app.rendererData.screen->root_depth,
		app.rendererData.window,
		app.rendererData.screen->root,
		10,
		10,
		app.width,
		app.height,
		1,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		app.rendererData.screen->root_visual,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(uint32_t[]){app.rendererData.screen->black_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS}
	);

	// initialize clay
	uint32_t totalMemorySize = Clay_MinMemorySize();
	app.clayArena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
	Clay_Initialize(app.clayArena, (Clay_Dimensions){(float)app.width, (float)app.height}, (Clay_ErrorHandler){HandleClayErrors});

	// tell clay how to measure text
	Clay_SetMeasureTextFunction(Clay_XCB_MeasureText, &app.rendererData);

	// initialize clay demo
	app.demoData = ClayVideoDemo_Initialize();

	// show the window
	xcb_map_window(app.rendererData.connection, app.rendererData.window);
	xcb_flush(app.rendererData.connection);

	// main loop
	bool running = true;
	xcb_generic_event_t *event = NULL;
	while (running && (event = xcb_wait_for_event(app.rendererData.connection)))
	{
		switch (event->response_type & ~0x80)
		{
			case XCB_EXPOSE:
			{
				xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
				Clay_SetLayoutDimensions((Clay_Dimensions){(float)expose->width, (float)expose->height});
				Clay_RenderCommandArray renderCommands = ClayVideoDemo_CreateLayout(&app.demoData);
				Clay_XCB_Render(&app.rendererData, &renderCommands);
				xcb_flush(app.rendererData.connection);
				break;
			}
		}

		free(event);
	}

	xcb_disconnect(app.rendererData.connection);
}
