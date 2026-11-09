#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/log/log.h>

#include <asset/asset_cache.h>
#include <render/renderer.h>
#include <scene/scene.h>
#include <scene/camera.h>
#include <window/window.h>

#include <stdio.h>

#include <SDL3/SDL.h>

/*
 * Build a cube with side length 1 around the origin. Every side has its own
 * 4 vertices, so each side keeps its own normal and color (hard edges).
 */
static s32 make_cube(Mesh *mesh)
{
        /* Normal, then the axes along the side's u and v direction */
        static const f32 sides[6][9] = {
                { 1, 0, 0,   0, 0,-1,   0, 1, 0 },
                {-1, 0, 0,   0, 0, 1,   0, 1, 0 },
                { 0, 1, 0,   1, 0, 0,   0, 0,-1 },
                { 0,-1, 0,   1, 0, 0,   0, 0, 1 },
                { 0, 0, 1,   1, 0, 0,   0, 1, 0 },
                { 0, 0,-1,  -1, 0, 0,   0, 1, 0 }
        };
        static const u8 colors[6][3] = {
                {230, 80, 70}, {70, 160, 230}, {90, 200, 110},
                {240, 200, 70}, {200, 110, 220}, {240, 240, 240}
        };
        static const f32 corners[4][2] = {
                {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}
        };
        s32 s;
        s32 c;

        if(mesh_init(mesh) < 0) {
                return -1;
        }

        for(s = 0; s < 6; s++) {
                const f32 *n = sides[s];
                IColor col = icolor(colors[s][0], colors[s][1], colors[s][2],
                                    255);
                s32 first = mesh_vertex_count(mesh);

                for(c = 0; c < 4; c++) {
                        f32 u = corners[c][0];
                        f32 v = corners[c][1];
                        FVector3 pos = fvec3(
                                n[0] * 0.5f + n[3] * u + n[6] * v,
                                n[1] * 0.5f + n[4] * u + n[7] * v,
                                n[2] * 0.5f + n[5] * u + n[8] * v);

                        mesh_add_vertex(mesh, vertex(pos,
                                fvec3(n[0], n[1], n[2]),
                                fvec2(u + 0.5f, v + 0.5f), col));
                }

                /* Counter-clockwise when looking at the side from outside */
                mesh_add_face(mesh, face(first, first + 1, first + 2));
                mesh_add_face(mesh, face(first, first + 2, first + 3));
        }

        return 0;
}

/*
 * Load the scene shader from the assets.
 */
static ShaderId load_scene_shader(AssetCache *assets, Renderer *ren)
{
        ShaderId none = { -1, 0 };
        AssetId vert = asset_load_text(assets, "shader/shader_model.vert");
        AssetId frag = asset_load_text(assets, "shader/shader_model.frag");

        if(vert < 0 || frag < 0) {
                return none;
        }

        return render_create_shader(ren, text_cstr(asset_text(assets, vert)),
                                    text_cstr(asset_text(assets, frag)));
}

s32 main(void)
{
        s32 running = 1;
        s32 ret = 1;
        AssetCache app_assets;
        Renderer app_renderer;
        Window window;

        char asset_dir[ASSET_BASE_DIR_LENGTH];
        WindowLinks links;

        Scene scene;
        Camera camera;
        ShaderId scene_shader;
        ModelId cube_model;
        Mesh cube;
        s32 cube_obj;
        Rectangle panel;

        /*
         * SETUP
         */

        if(!SDL_Init(SDL_INIT_VIDEO)) {
                LOG(LOG_ERROR, "Failed to initialize SDL");
                LOG_S(LOG_ERROR, "%s", (char *)SDL_GetError());
                return 1;
        }

        if(render_init(&app_renderer) < 0) {
                LOG(LOG_ERROR, "Failed to initialize the app renderer");
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
                goto err_render_deinit;
        }

        links.assets = &app_assets;
        links.renderer = &app_renderer;
        if(win_init(&window, links, "Vasall Studio", 800, 600) < 0) {
                LOG(LOG_ERROR, "Failed to create main window");
                goto err_asset_deinit;
        }

        /*
         * DEMO SCENE: a spinning cube with a panel drawn on the UI canvas
         */

        scene_shader = load_scene_shader(&app_assets, &app_renderer);
        if(scene_shader.index < 0) {
                LOG(LOG_ERROR, "Failed to load the scene shader");
                goto err_win_deinit;
        }

        if(make_cube(&cube) < 0) {
                LOG(LOG_ERROR, "Failed to build the cube mesh");
                goto err_win_deinit;
        }
        cube_model = render_create_model(&app_renderer, &cube);
        mesh_deinit(&cube);

        if(scene_init(&scene, scene_shader) < 0) {
                goto err_win_deinit;
        }
        cube_obj = scene_add(&scene, cube_model, vec3(0.0, 0.0, 0.0));

        camera_init(&camera);
        camera.position = vec3(2.0, 1.5, 3.0);
        camera_look_at(&camera, vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));

        win_set_scene(&window, &scene, &camera);

        /* Half transparent, to show that the scene shows through the UI */
        panel = mkrect(20, 20, 220, 120);
        img_fill_rect(&window.ui.canvas, panel, icolor(20, 20, 24, 200));
        img_draw_rect(&window.ui.canvas, panel, icolor(255, 255, 255, 255), 2);
        ui_screen_invalidate(&window.ui, mkrect(0, 0, 260, 160));

        /*
         * UPDATE
         */

        while(running) {
                SDL_Event event;
                SceneObject *obj;

                while(SDL_PollEvent(&event)) {
                        if(event.type == SDL_EVENT_QUIT) running = 0;
                        win_handle_event(&window, &event);
                }

                if((obj = scene_object(&scene, cube_obj))) {
                        f64 t = (f64)SDL_GetTicks() / 1000.0;
                        obj->rotation = quat_euler(t * 0.5, t * 0.8, 0.0);
                }

                win_render(&window);
        }

        ret = 0;

        /*
         * SHUTDOWN
         */

        scene_deinit(&scene);

err_win_deinit:
        win_deinit(&window);

err_asset_deinit:
        asset_deinit(&app_assets);

err_render_deinit:
        render_deinit(&app_renderer);

err_sdl_quit:
        SDL_Quit();

        return ret;
}
