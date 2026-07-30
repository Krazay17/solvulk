#include "s_view.h"
#include "sol_core.h"
#include "sol_engine.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "combat/s_combat.h"
#include "physx/s_body.h"
#include "render/render.h"
#include "buff/s_buff.h"
#include "vital/s_vital.h"

#define HEALTHBAR_HIDE_DURATION 5.0f

void Sol_Crosshair_Draw(World *world, double dt, double time)
{
    float     x      = (float)WINDOW_WIDTH / 2.0f;
    float     y      = (float)WINDOW_HEIGHT / 2.0f;
    float     width  = 11.0f;
    float     height = 11.0f;
    RectSSBO *ssbo   = Sol_Render_GetNext_Rect();
    ssbo->pos        = (vec4s){UISCALE(x - width * 0.5f), UISCALE(y - height * 0.5f), 1, 1.0f};
    ssbo->dims       = (vec4s){UISCALE(width), UISCALE(height), 0, 1.0f};
    ssbo->color      = (vec4s){1, 1, 1, 1};
    ssbo->textureID  = SOL_TEXTURE_CROSSHAIR;
    ssbo->uv         = (vec4s){0.0f, 0.0f, 1.0f, 1.0f};
    ssbo->type       = 0;
    ssbo->flags      = 0;
}

static void Weapon_Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & BITC(HAS_COMBAT)))
            continue;
        CompModel *model = &world->models[id];

        SolXform leftXform = Sol_Model_GetBoneXform(world, id, "hand.L.Weapon");
        Sol_Xform_SetXform(world, model->leftWeaponEnt, leftXform);

        SolXform rightXform = Sol_Model_GetBoneXform(world, id, "hand.R.Weapon");
        Sol_Xform_SetXform(world, model->rightWeaponEnt, rightXform);
    }
}

static void Draw_Player_Buffs(World *world, double dt, double time)
{
    CompBuff *buffs = &solEngine.gameWorld3d->buffs[1];
    for (int i = 0; i < buffs->count; i++)
    {
        Buff buff = buffs->buffs[i];
        
        RectSSBO *ssbo  = Sol_Render_GetNext_Rect();
     //   ssbo->pos
    }
}

void Sol_View_Init(World *world)
{
    Sol_Nameplate_Init(world);
    WAdd3d(world) = Weapon_Draw;
}
