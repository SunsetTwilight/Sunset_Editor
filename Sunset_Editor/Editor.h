#ifndef _GAME_ENGINE_H_
#define _GAME_ENGINE_H_

#include "Sunset_Editor_pch.h"

struct HINSTANCE__;
typedef int BOOL;

SUNSET_EDITOR_API int __stdcall PROJECT_MAIN_FUNC(HINSTANCE__* hInstance, int nShowCmd);

BOOL EditorEngine_Init(void);
void GameEngine_Update(void);
void GameEngine_Render(void);
BOOL EditorEngine_CleanUp(void);

#endif // !_GAME_ENGINE_H_
