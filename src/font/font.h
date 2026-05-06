#pragma once
#include "sol/types.h"

void            Parse_Font(SolResource metrics, SolResource atlas, int id);
TextBounds      ParseBounds(const char *p, const char *end);
void            Sol_Draw_Text(SolFontDesc desc);
ShaderPushTexts Prepare_Text(SolFontDesc desc);
SolFont        *Sol_GetFont(SolFontKind kind);
