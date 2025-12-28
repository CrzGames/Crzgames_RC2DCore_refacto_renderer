#include <mygame/camera.h>
#include <SDL3/SDL_stdinc.h>

Camera::Camera(const GameScreen& gameScreen, const TileMap& tileMap, CameraConfig initialConfig)
    : gameScreen(&gameScreen), tileMap(&tileMap), config(initialConfig)
{
    if (this->config.zoom == 0.0f)
        this->config.zoom = CAMERA_DEFAULT_ZOOM;

    RecalculateVisibleBounds();
    ClampPositionWithinBounds();
}

Camera::~Camera() {}

void Camera::RecalculateVisibleBounds()
{
    if (!this->gameScreen || !this->tileMap)
        return;

    const float visibleWidth  = this->gameScreen->rect.w / this->config.zoom;
    const float visibleHeight = this->gameScreen->rect.h / this->config.zoom;

    this->config.minX = 0.0f;
    this->config.minY = 0.0f;
    this->config.maxX = (float)this->tileMap->width  - visibleWidth;
    this->config.maxY = (float)this->tileMap->height - visibleHeight;
}

void Camera::ClampPositionWithinBounds()
{
    if (!this->gameScreen || !this->tileMap)
        return;

    if (this->config.maxX < this->config.minX) this->config.maxX = this->config.minX;
    if (this->config.maxY < this->config.minY) this->config.maxY = this->config.minY;

    this->config.x = SDL_clamp(this->config.x, this->config.minX, this->config.maxX);
    this->config.y = SDL_clamp(this->config.y, this->config.minY, this->config.maxY);
}

void Camera::MoveCamera(float deltaX, float deltaY)
{
    this->config.x += deltaX;
    this->config.y += deltaY;
    ClampPositionWithinBounds();
}

void Camera::HandleKeyboardMovement(float deltaTime)
{
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (rc2d_keyboard_isDown(RC2D_LEFT))  moveX -= CAMERA_MOVE_SPEED * deltaTime;
    if (rc2d_keyboard_isDown(RC2D_RIGHT)) moveX += CAMERA_MOVE_SPEED * deltaTime;
    if (rc2d_keyboard_isDown(RC2D_UP))    moveY -= CAMERA_MOVE_SPEED * deltaTime;
    if (rc2d_keyboard_isDown(RC2D_DOWN))  moveY += CAMERA_MOVE_SPEED * deltaTime;

    if (moveX != 0.0f && moveY != 0.0f) {
        float magnitude = SDL_sqrtf(moveX * moveX + moveY * moveY);
        if (magnitude > 0.0f) {
            float normalizationFactor = (CAMERA_MOVE_SPEED * deltaTime) / magnitude;
            moveX *= normalizationFactor;
            moveY *= normalizationFactor;
        }
    }

    if (moveX != 0.0f || moveY != 0.0f) {
        RC2D_log(RC2D_LOG_INFO, "Camera move: dx=%.1f dy=%.1f pos=(%.1f, %.1f, zoom=%.2f)",
                 moveX, moveY, this->config.x, this->config.y, this->config.zoom);
        MoveCamera(moveX, moveY);
    }
}

void Camera::AdjustZoom(float zoomDelta)
{
    // Appliquer le zoom
    this->config.zoom += zoomDelta;

    // Clamp le zoom
    if (this->config.zoom < CAMERA_MIN_ZOOM) this->config.zoom = CAMERA_MIN_ZOOM;
    if (this->config.zoom > CAMERA_MAX_ZOOM) this->config.zoom = CAMERA_MAX_ZOOM;

    // Recalculer les bornes visibles et clamp la position
    RecalculateVisibleBounds();
    ClampPositionWithinBounds();
}