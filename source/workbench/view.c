#include <workbench/view.h>

#include <stdio.h>
#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 views_init(Views *views)
{
        views->next_id = 1;
        return pool_init(&views->pool, sizeof(View), 32);
}

PUB void views_deinit(Views *views)
{
        PoolHandle h;
        View *v;
        s32 i;

        for(i = 0; i < pool_slots(&views->pool); i++) {
                v = pool_at(&views->pool, i, &h);
                if(v) view_destroy(views, h);
        }
        pool_deinit(&views->pool);
}

PUB ViewHandle view_create(Views *views, const ViewClass *cls,
                const char *title, void *data)
{
        ViewHandle h;
        View *v;

        v = pool_add(&views->pool, NULL, &h);
        if(!v) return h;

        v->cls = cls;
        v->id = views->next_id++;
        v->data = data;
        snprintf(v->title, sizeof(v->title), "%s", title ? title : "");
        return h;
}

PUB View *view_get(Views *views, ViewHandle h)
{
        return pool_get(&views->pool, h);
}

PUB void view_destroy(Views *views, ViewHandle h)
{
        View *v = view_get(views, h);

        if(!v) return;
        if(v->cls && v->cls->destroy) v->cls->destroy(v);
        pool_remove(&views->pool, h);
}

PUB s32 view_handle_equal(ViewHandle a, ViewHandle b)
{
        return a.index == b.index && a.generation == b.generation;
}
