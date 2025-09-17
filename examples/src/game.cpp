#include <mygame/map.h>
#include <mygame/game.h>
#include <RC2D/RC2D.h>

static Map map;

void rc2d_unload(void) 
{
    map.Unload();
}

void rc2d_load(void) 
{
    rc2d_window_setSize(1280, 720);

    map.Load();
}

void rc2d_update(double dt) 
{
    map.Update(dt);
}

void rc2d_draw(void) 
{
    map.Draw();
}

void rc2d_keypressed(const char* key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID) 
{
    map.KeyPressed(key, scancode, keycode, mod, isrepeat, keyboardID);
}

void rc2d_mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{
    map.MousePressed(x, y, button, clicks, mouseID);
}