#include "render.h"

typedef struct
{
    u32           count;
    BillboardSSBO billboardSSBO[MAX_MODEL_INSTANCES];
    FlagsSSBO     flags[MAX_MODEL_INSTANCES];
} BillboardSubmission;

static BillboardSubmission    billboardQueue;

void Sol_Render_Push_Billboard(BillboardKind kind, BillboardSSBO *billboardSSBO, FlagsSSBO *flags)
{

}