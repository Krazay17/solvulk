#pragma once
#include "types.h"
#include "components.h"

#define MAX_USER_ITEMS 512

typedef struct SolUser
{
    int view_world;
    int view_ent;
    int menu_world, game_world, hud_world;
    float yaw, pitch;
    SolActions actions;
    bool mouse_locked;
    vec2s mouse_pos, mouse_pos_ui, focus_start;
    bool interact;
    bool interact_last;
    bool grab, grab_last;

    u32 window_greyitem;

    int target, target_w;
    int focus, focus_w;
    int drag, drag_w;
} SolUser;

typedef struct UserData
{
    float look_sens;
    float volume;
    SolActions key_binds[SOL_KEY_COUNT];
    SolActions mouse_binds[SOL_MOUSE_COUNT];

    ScInventory inventory;
    SolItem loose_items[MAX_USER_ITEMS];
    int loose_item_count;
    bool unlocked_items[ABILITYKIND_COUNT];
} UserData;

extern UserData user_data;
extern SolUser sol_user;

int Sol_User_Init(void);

void Sol_User_Tick(double dt);
void Sol_User_PostTick(double dt);
void Sol_User_Draw(double dt);

void Sol_User_SaveUserSettings();
void Sol_User_LoadUserSettings();
void Sol_User_ClearUserSettings();
World *Sol_User_GetGameWorld();
void Sol_User_EnterGameWorld(u32 idx, bool sim_last, vec3s pos);
void Sol_User_AddItem(SolItem *item);
void Sol_User_AddItemLoose(SolItem *item);