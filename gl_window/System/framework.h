#pragma once

#include <Windows.h>
#include "glew.h"
#include "wglew.h"
#include "log.h"

extern void osMessage(int ID, const char* fmt, ...);
extern void allegro_message(const char *title, const char *message);
extern HWND win_get_window();
extern int SCREEN_W;
extern int SCREEN_H;

