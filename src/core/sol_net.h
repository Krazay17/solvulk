#pragma once
#include "sol/types.h"
#include "sol_user.h"

#define MAX_USERS 8

typedef struct
{
    u32 id;
    UserData data;
} SessionUser;
typedef struct
{
    SessionUser *users;
    int user_cnt;
    int user_cap;
} UserSession;

extern UserSession user_session;

int Sol_Net_Init();
int Sol_Net_AddUser(SessionUser user);