#ifndef GRAPHICS_PROVIDER_H_
#define GRAPHICS_PROVIDER_H_

#include "CommonModules.h"
#include "SPU.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <cstddef>

const size_t WINDOW_X_SIZE          = 800;
const size_t WINDOW_Y_SIZE          = 600;

const size_t CELLS_BY_LINE          = 20;
const float CELLS_DISTANCE          = 25;
const float CELL_SIZE               = 20;

const float LEFT_OFFSET             = (WINDOW_X_SIZE - CELLS_BY_LINE * CELLS_DISTANCE) / 2;
const float TOP_OFFSET              = (WINDOW_Y_SIZE - ((float) VRAM_SIZE / 3 / CELLS_BY_LINE * CELLS_DISTANCE)) / 2;

const float OUTLINE_THICKNESS       = 1.5;

ProcessorErrorCode UpdateGraphics (SPU *spu, size_t ramAddress);

void RenderingThread (sf::RenderWindow* window);

#endif
