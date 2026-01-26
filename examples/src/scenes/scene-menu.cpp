#include <RC2D/RC2D_steamworks.h>
#include <amoredtactics/scenes/scene-menu.h>

void MenuScene::unload(void) 
{
    RC2D_log(RC2D_LOG_INFO, "Menu Scene Unloaded\n");
}

void MenuScene::load(void) 
{
    RC2D_log(RC2D_LOG_INFO, "Menu Scene Loaded\n");
    rc2d_steamworks_unlockAchievement("TOTO");
}

void MenuScene::update(double dt) 
{

}

void MenuScene::draw(void) 
{

}

void MenuScene::keypressed(const char *key, SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod, bool isrepeat, SDL_KeyboardID keyboardID) 
{
    if (strcmp(key, "K") == 0 && !isrepeat) 
    {
        RC2D_log(RC2D_LOG_INFO, "Enter K Pressed change scene \n");
        sceneManager->changeScene("editormap");
    }
}

void MenuScene::mousepressed(float x, float y, RC2D_MouseButton button, int clicks, SDL_MouseID mouseID) 
{

}