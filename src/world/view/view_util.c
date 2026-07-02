#include "s_view.h"
#include "buff/s_buff.h"

const u32 buff_icon_map[BUFFKIND_COUNT] = {
    [BUFFKIND_FIRE]   = SOL_TEXTURE_FIREPARTICLE,
    [BUFFKIND_STUN]   = SOL_TEXTURE_SHOCKPARTICLE,
    [BUFFKIND_INVULN] = SOL_TEXTURE_SHIELD,
};