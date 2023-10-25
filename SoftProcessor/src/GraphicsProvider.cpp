#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/Event.hpp>

#include "GraphicsProvider.h"
#include "CommonModules.h"
#include "Logger.h"
#include "SPU.h"

// TODO graphics

static sf::RectangleShape memoryCells [VRAM_SIZE];

static sf::Mutex updateMutex = {};

void RenderingThread (sf::RenderWindow* window) {
    window->setActive (true);

    for (size_t cellIndex = 0; cellIndex < VRAM_SIZE; cellIndex++) {
        memoryCells [cellIndex].setPosition (LEFT_OFFSET + CELLS_DISTANCE * (float) (cellIndex / (size_t) CELLS_BY_LINE), TOP_OFFSET + CELLS_DISTANCE * (float) (cellIndex % CELLS_BY_LINE));

        sf::Color outlineColor (200, 200, 200);
        memoryCells [cellIndex].setFillColor    (sf::Color::Black);
        memoryCells [cellIndex].setOutlineColor (outlineColor);
        memoryCells [cellIndex].setOutlineThickness (OUTLINE_THICKNESS);

        memoryCells [cellIndex].setSize ({CELL_SIZE, CELL_SIZE});

        window->draw (memoryCells [cellIndex]);
    }

    window->display();

    while (window->isOpen()) {
        updateMutex.lock ();

        updateMutex.unlock ();
    }
}

ProcessorErrorCode UpdateGraphics (sf::RenderWindow *window) {
    PushLog (2);

    while (window->isOpen ()) {

    }

    RETURN NO_PROCESSOR_ERRORS;
}
