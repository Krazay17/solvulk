#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "playercontroller.h"
#include "singles/input.h"

static PlayerState playerState = {0};

PlayerState *Sol_GetPlayerState()
{
    SolMouse mouse = SolInput_GetMouse();

    playerState.yaw += mouse.dx;
    playerState.pitch += mouse.dy;

    if (SolInput_KeyDown(SOL_KEY_W))
        playerState.actionState |= ACTION_FWD;
    else
        playerState.actionState &= ~ACTION_FWD;

    if (SolInput_KeyDown(SOL_KEY_S))
        playerState.actionState |= ACTION_BWD;
    else
        playerState.actionState &= ~ACTION_BWD;

    if (SolInput_KeyDown(SOL_KEY_A))
        playerState.actionState |= ACTION_LEFT;
    else
        playerState.actionState &= ~ACTION_LEFT;

    if (SolInput_KeyDown(SOL_KEY_D))
        playerState.actionState |= ACTION_RIGHT;
    else
        playerState.actionState &= ~ACTION_RIGHT;

    return &playerState;
}