#pragma once
#include "sol/types.h"

typedef struct SolBank
{
    SolFont  fonts[SOL_FONT_COUNT];
    SolImage images[SOL_IMAGE_COUNT];
    SolModel models[SOL_MODEL_COUNT];
} SolBank;

const char *model_path[SOL_MODEL_COUNT];
const i32   model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT];

// FUNCS---------------------
SolModel Parse_Model(SolResource res);
void     Sol_Skeleton_Pose(SolSkeleton *skel, AnimBlend *blends);

void Sol_Draw_Model(SolModelId handle, vec3s pos, vec3s scale, versors quat, u32 flags);
void Sol_Draw_Model_Skinned(SolModelId handle, SolModelDraw *inst);

void       Parse_Font_Metrics(const char *json, float atlasW, float atlasH, SolGlyph *glyphs);
TextBounds ParseBounds(const char *p, const char *end);

void           Sol_Draw_Text(SolFontDesc desc);
ShaderPushTexts Prepare_Text(SolFontDesc desc);

SolBank       *Sol_Bank_Get();
SolModel      *Sol_GetModel(SolModelId id);