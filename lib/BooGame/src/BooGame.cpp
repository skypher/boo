#include "BooGame.h"
#include <stdlib.h> // for random if needed

// We need a way to get time. For now, let's assume update() is called ~30 times a second
// or we can add a 'deltaTime' parameter.
// For the blink logic which relies on millis(), we might need to inject time.

BooGame::BooGame() {
    ghostX = 0;
    ghostY = 0;
    velX = 0;
    velY = 0;
    blinking = false;
    lastBlinkTime = 0;
    blinkStartTime = 0;
    currentTime = 0;
}

void BooGame::init() {
    ghostX = SCREEN_WIDTH / 2 - GHOST_SIZE / 2;
    ghostY = SCREEN_HEIGHT / 2 - GHOST_SIZE / 2;
    velX = 1.2f;
    velY = 0.8f;
    
    // Reset state
    blinking = false;
    lastBlinkTime = 0;
    blinkStartTime = 0;
    currentTime = 0;
}

void BooGame::setPosition(float x, float y) {
    ghostX = x;
    ghostY = y;
}

void BooGame::setVelocity(float vx, float vy) {
    velX = vx;
    velY = vy;
}

void BooGame::update() {
    // Advance time (simulated 33ms per frame for now)
    currentTime += 33; 

    // Update position
    ghostX += velX;
    ghostY += velY;

    // Bounce logic
    if (ghostX <= 0) {
        ghostX = 0;
        velX = -velX;
    } else if (ghostX >= SCREEN_WIDTH - GHOST_SIZE) {
        ghostX = SCREEN_WIDTH - GHOST_SIZE;
        velX = -velX;
    }

    if (ghostY <= 0) {
        ghostY = 0;
        velY = -velY;
    } else if (ghostY >= SCREEN_HEIGHT - GHOST_SIZE - 18) { // -18 from original code logic
        ghostY = SCREEN_HEIGHT - GHOST_SIZE - 18;
        velY = -velY;
    }

    // Blink logic
    if (currentTime - lastBlinkTime > 3000) {
        blinking = true;
        blinkStartTime = currentTime;
        lastBlinkTime = currentTime;
    }
    if (blinking && currentTime - blinkStartTime > 200) {
        blinking = false;
    }
}
