#ifndef GRAPHICS_PROVIDER_H_
#define GRAPHICS_PROVIDER_H_

#include "CommonModules.h"
#include "SPU.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <cstddef>

const size_t WINDOW_X_SIZE          = 1500;
const size_t WINDOW_Y_SIZE          = 1200;

const size_t CELLS_BY_LINE          = 100;
const float CELLS_DISTANCE          = 10;
const float CELL_SIZE               = 10;

const float LEFT_OFFSET             = (WINDOW_X_SIZE - CELLS_BY_LINE * CELLS_DISTANCE) / 2;
const float TOP_OFFSET              = (WINDOW_Y_SIZE - ((float) VRAM_SIZE / 3 / CELLS_BY_LINE * CELLS_DISTANCE)) / 2;

const float     OUTLINE_THICKNESS       = 0;
const sf::Color OUTLINE_COLOR           = sf::Color::Black;

ProcessorErrorCode UpdateGraphics (SPU *spu, size_t ramAddress);
ProcessorErrorCode RenderLoop (sf::RenderWindow* window, SPU *spu, sf::Mutex *workMutex);

ProcessorErrorCode InitCells ();

#endif
