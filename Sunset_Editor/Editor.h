#ifndef _GAME_ENGINE_H_
#define _GAME_ENGINE_H_

#include "Sunset_Editor_pch.h"

struct HINSTANCE__;
typedef int BOOL;

SUNSET_EDITOR_API int __stdcall PROJECT_MAIN_FUNC(HINSTANCE__* hInstance, int nShowCmd);

BOOL GameEngine_Init(void);
void GameEngine_Update(void);
void GameEngine_Render(void);
BOOL GameEngine_CleanUp(void);

#endif // !_GAME_ENGINE_H_
