#include "s_ai.h"

const StateFunc aistate_func[AISTATE_COUNT] = {
    [AISTATE_IDLE] =
        {
            Idle_State_Update,
            Idle_State_Enter,
            Idle_State_Exit,
            Idle_State_CanExit,
            Idle_State_CanEnter,
        },
    [AISTATE_PATROL] =
        {
            Patrol_State_Update,
            Patrol_State_Enter,
            Patrol_State_Exit,
            Patrol_State_CanExit,
            Patrol_State_CanEnter,
        },
    [AISTATE_AGGRO] =
        {
            Aggro_State_Update,
            Aggro_State_Enter,
            Aggro_State_Exit,
            Aggro_State_CanExit,
            Aggro_State_CanEnter,
        },
};
