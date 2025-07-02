
#include "xprogman.h"

#define WIDTH 400
#define HEIGHT 400

static struct {
	xcb_connection_t *connection;
	const xcb_setup_t *setup;
	xcb_screen_t *screen;
	xcb_gcontext_t gcontext;
	xcb_window_t window;
} app;

void app_cleanup(void)
{
	if (app.connection)
		xcb_disconnect(app.connection);
}

void app_init(void)
{
	memset(&app, 0, sizeof(app));
	atexit(app_cleanup);
}

int main(void)
{
	/* setup */
	app_init();

	/* open connection */
	app.connection = xcb_connect(NULL, NULL);
	if (!app.connection)
		die("couldn't connect to an X server");

	/* get the current screen */
	app.setup = xcb_get_setup(app.connection);
	app.screen = xcb_setup_roots_iterator(app.setup).data;
	if (!app.screen)
		die("couldn't get the current screen");

	/* create graphics context */
	app.gcontext = xcb_generate_id(app.connection);
	xcb_create_gc(
		app.connection,
		app.gcontext,
		app.screen->root,
		XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES,
		(uint32_t[]){app.screen->black_pixel, 0}
	);

	/* create window */
	app.window = xcb_generate_id(app.connection);
	xcb_create_window(
		app.connection,
		app.screen->root_depth,
		app.window,
		app.screen->root,
		10,
		10,
		WIDTH,
		HEIGHT,
		1,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		app.screen->root_visual,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(uint32_t[]){app.screen->white_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS}
	);

	/* show the window */
	xcb_map_window(app.connection, app.window);
	xcb_flush(app.connection);

	/* main loop */
	xcb_generic_event_t *event = NULL;
	int done = 0;
	xcb_rectangle_t rect = {20, 20, 60, 60};
	while (!done && (event = xcb_wait_for_event(app.connection)))
	{
		switch (event->response_type & ~0x80)
		{
			case XCB_EXPOSE:
				xcb_poly_fill_rectangle(app.connection, app.window, app.gcontext, 1, &rect);
				xcb_flush(app.connection);
				break;
			case XCB_KEY_PRESS:
				done = 1;
				break;
		}

		free(event);
	}

	return EXIT_SUCCESS;

#if 0
	/* copy data dirs env variable */
	if (getenv("XDG_DATA_DIRS"))
	{
		char env[1024];
		char *dirs[128];
		int num_dirs;
		int i;

		/* make a copy of the environment variable */
		snprintf(env, sizeof(env), "%s", getenv("XDG_DATA_DIRS"));

		/* split it up */
		num_dirs = split_dirs(env, dirs, 128);

		/* spew */
		say("%d dirs found", num_dirs);
		for (i = 0; i < num_dirs; i++)
		{
			say("%d: %s", i, dirs[i]);
		}
	}
	else
	{
		warn("the environment variable $XDG_DATA_DIRS is not defined");
	}

	return 0;
#endif
}
