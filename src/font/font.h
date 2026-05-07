#pragma once
#include "sol/types.h"

extern const char *font_path[SOL_FONT_COUNT];

SolFont *Sol_GetFont(SolFontKind kind);
void     Sol_Draw_Text(SolFontDesc desc);

void       Sol_Load_Fonts();
void       Parse_Font(SolResource metrics, int id);
TextBounds ParseBounds(const char *p, const char *end);

ShaderPushTexts Prepare_Text(SolFontDesc desc);
