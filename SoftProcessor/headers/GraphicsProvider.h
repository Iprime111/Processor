#ifndef GRAPHICS_PROVIDER_H_
#define GRAPHICS_PROVIDER_H_

#include "CommonModules.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <cstddef>

const size_t WINDOW_X_SIZE          = 800;
const size_t WINDOW_Y_SIZE          = 600;

const size_t CELLS_BY_LINE          = 10;
const float CELLS_DISTANCE          = 50;
const float CELL_SIZE               = 40;

const float LEFT_OFFSET             = 140;
const float TOP_OFFSET              = 40;

const float OUTLINE_THICKNESS       = 1.5;

const sf::Time GRAPHICS_UPDATE_TIME = sf::milliseconds (10);

ProcessorErrorCode UpdateGraphics (sf::RenderWindow *window);

void RenderingThread (sf::RenderWindow* window);

#endif
