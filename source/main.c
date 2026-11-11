#include <devkit/log/log.h>

#include <platform/app.h>
#include <workbench/shell.h>

int main(void)
{
        App app;
        Workbench wb;
        AppWindow *win;

        if(app_init(&app) < 0) {
                LOG(LOG_ERROR, "can't start");
                return 1;
        }

        win = app_window_open(&app, "Vasall Studio", 1400, 860,
                        workbench_frame, &wb);
        if(!win) {
                app_deinit(&app);
                return 1;
        }

        if(workbench_init(&wb, win) < 0) {
                LOG(LOG_ERROR, "can't create the workbench");
                app_deinit(&app);
                return 1;
        }

        app_run(&app);

        workbench_deinit(&wb);
        app_deinit(&app);
        return 0;
}
