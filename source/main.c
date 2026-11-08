#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/log/log.h>

#include <asset/asset_cache.h>
#include <render/renderer.h>
#include <window/window.h>

#include <stdio.h>

#include <SDL3/SDL.h>
#include <glad/gl.h>

#define ASSET_BASE_DIR_LENGTH 512

s32 main(void)
{
        s32 running = 1;
        AssetCache app_assets;
        char asset_dir[ASSET_BASE_DIR_LENGTH];
        Window window;

        /*
         * SETUP
         */

        if(!SDL_Init(SDL_INIT_VIDEO)) {
                LOG(LOG_ERROR, "Failed to initialize SDL");
                LOG_S(LOG_ERROR, "%s", (char *)SDL_GetError());
                return 1;
        }

        if(render_setup_gl() < 0) {
                LOG(LOG_ERROR, "Failed to initialize OpenGL");
                goto err_sdl_quit;
        }

        /*
         * Studio's own assets live next to the executable, so studio finds
         * them no matter which folder it is started from. SDL_GetBasePath
         * ends with a '/' and the string belongs to SDL (don't free it).
         */
        snprintf(asset_dir, sizeof(asset_dir), "%sassets",
                        SDL_GetBasePath());

        if(asset_init(&app_assets, asset_dir) < 0) {
                LOG(LOG_ERROR, "Failed to initialize asset cache");
                goto err_sdl_quit;
        }

        if(win_init(&window, "Vasall Studio", 800, 600) < 0) {
                LOG(LOG_ERROR, "Failed to create main window");
                goto err_sdl_quit;
        }

        /*
         * UPDATE
         */

        while(running) {
                SDL_Event event;
                while(SDL_PollEvent(&event)) {
                        if(event.type == SDL_EVENT_QUIT) running = false;
                        win_handle_event(&window, &event);
                }

                win_begin_frame(&window);

                /* Rendering goes here */

                win_end_frame(&window);
        }

        /*
         * SHUTDOWN
         */

        win_deinit(&window);

err_asset_deinit:
        asset_deinit(&app_assets);

err_sdl_quit:
        SDL_Quit();

        return 0;
}