#include "sol_net.h"

UserSession user_session;

int Sol_Net_Init()
{
    user_session.user_cap = MAX_USERS;
    user_session.user_cnt = 0;
    user_session.users    = calloc(user_session.user_cap, sizeof(SessionUser));
    if (!user_session.users)
        return 1;

    return 0;
}

int Sol_Net_AddUser(SessionUser user)
{
    int idx                 = user_session.user_cnt++;
    user_session.users[idx] = user;
    return idx;
}