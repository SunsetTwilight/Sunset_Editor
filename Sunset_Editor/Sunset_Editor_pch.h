//------------------------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------------------------

#ifndef _SUNSET_GAMEENGINE_PCH_H_
#define _SUNSET_GAMEENGINE_PCH_H_

//------------------------------------------------------------------------------------------------
//�w�b�_�[�t�@�C���@�ǂݍ���
//------------------------------------------------------------------------------------------------

#include <Windows.h>

#ifdef SUNSET_EDITOR_EXPORTS
#define SUNSET_EDITOR_API extern "C" __declspec(dllexport)
#else
#define SUNSET_EDITOR_API extern "C" __declspec(dllimport)
#endif

#endif // !_SUNSET_GAMEENGINE_PCH_H_

