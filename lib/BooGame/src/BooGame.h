#ifndef BOO_GAME_H
#define BOO_GAME_H

#include <stdint.h>
#include <math.h>

// Constants
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define GHOST_SIZE 32

class BooGame {
public:
    BooGame();

    void init();
    void update();
    
    // Getters for rendering/testing
    float getGhostX() const { return ghostX; }
    float getGhostY() const { return ghostY; }
    bool isBlinking() const { return blinking; }

    // Setters for testing
    void setPosition(float x, float y);
    void setVelocity(float vx, float vy);

private:
    float ghostX, ghostY;
    float velX, velY;
    
    // Blink state
    bool blinking;
    unsigned long lastBlinkTime;
    unsigned long blinkStartTime;
    
    // Helper to simulate millis() locally if needed, 
    // or we can pass delta time. For simplicity, we'll assume ~30fps 
    // or pass in a time value.
    unsigned long currentTime;
};

#endif
