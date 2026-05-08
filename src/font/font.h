#pragma once
#include "sol/types.h"

extern const char *font_path[SOL_FONT_COUNT];

SolFont *Sol_GetFont(SolFontKind kind);
void     Sol_Draw_Text(SolFontDesc desc);

int       Sol_Fonts_Init();
TextBounds ParseBounds(const char *p, const char *end);

ShaderPushTexts Prepare_Text(SolFontDesc desc);
