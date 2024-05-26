#ifndef _ENGINE_H
#define _ENGINE_H

#include "wad.h"

void engine_init(wad_t *wad, const char *mapname);
void engine_update(float dt);
void engine_render();

#endif // !_ENGINE_H
