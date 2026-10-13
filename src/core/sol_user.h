#pragma once
#include "types.h"
#include "components.h"

#define MAX_USER_ITEMS 512

typedef struct UserCore
{
    int view_ent;
    int view_world;
    int menu_world, game_world, hud_world;
    float yaw, pitch;
    u32 inventory_id;
    SolActions actions;
    UserInput user_input;
    UserHit user_hit;
} UserCore;

typedef struct UserConfig
{
    float look_sens;
    float volume;
    SolActions key_binds[SOL_KEY_COUNT];
    SolActions mouse_binds[SOL_MOUSE_COUNT];
} UserConfig;

typedef struct UserData
{
    UserConfig user_config;
    ScInventory inventory;
    SolItem loose_items[MAX_USER_ITEMS];
    int loose_item_count;
    bool unlocked_items[ITEMKIND_COUNT];
} UserData;

extern UserData user_data;
extern UserCore sol_user;

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