#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>
#include <cstdio>
#include <cstdlib>

#include "CustomAssert.h"
#include "GraphicsProvider.h"
#include "CommonModules.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SPU.h"

static sf::RectangleShape *memoryCells = NULL;

static sf::Mutex updateMutex = {};

ProcessorErrorCode RenderLoop (sf::RenderWindow* window, SPU *spu, sf::Mutex *workMutex) {
    PushLog (2);

    custom_assert (window,      pointer_is_null, NO_BUFFER);
    custom_assert (workMutex,   pointer_is_null, NO_BUFFER);
    custom_assert (spu,         pointer_is_null, NO_PROCESSOR);

    window->setActive (true);

    while (window->isOpen()) {
        window->clear ();

        sf::Event event = {};

        workMutex->lock ();
        if (!spu->isWorking) {
            window->close ();
        }
        workMutex->unlock ();

        while (window->pollEvent (event))
        {
            if (event.type == sf::Event::Closed)
                window->close();
        }

        updateMutex.lock ();
        for (size_t cellIndex = 0; cellIndex < PIXEL_COUNT; cellIndex++) {
            window->draw (memoryCells [cellIndex]);
        }
        updateMutex.unlock ();

        window->display ();
    }

    delete [] memoryCells;

    RETURN NO_PROCESSOR_ERRORS;
}

ProcessorErrorCode InitCells () {
    PushLog (4);

    updateMutex.lock ();

    memoryCells = new sf::RectangleShape [PIXEL_COUNT];

    if (!memoryCells) {
        updateMutex.unlock ();
        RETURN NO_BUFFER;
    }

    for (size_t cellIndex = 0; cellIndex < PIXEL_COUNT; cellIndex++) {

        memoryCells [cellIndex].setPosition (LEFT_OFFSET + CELLS_DISTANCE * (float) (cellIndex / (size_t) CELLS_BY_LINE),
                                                TOP_OFFSET + CELLS_DISTANCE * (float) (cellIndex % CELLS_BY_LINE));

        memoryCells [cellIndex].setFillColor    (sf::Color::Black);

        memoryCells [cellIndex].setOutlineColor     (OUTLINE_COLOR);
        memoryCells [cellIndex].setOutlineThickness (OUTLINE_THICKNESS);

        memoryCells [cellIndex].setSize ({CELL_SIZE, CELL_SIZE});
    }

    updateMutex.unlock ();

    RETURN NO_PROCESSOR_ERRORS;
}

ProcessorErrorCode UpdateGraphics (SPU *spu, size_t ramAddress) {
    PushLog (2);

    custom_assert (spu,      pointer_is_null, NO_PROCESSOR);
    custom_assert (spu->ram, pointer_is_null, NO_BUFFER);

    size_t cellIndex = ramAddress / COLOR_CHANNELS;

    sf::Color color ((sf::Uint8) spu->ram [cellIndex * COLOR_CHANNELS], (sf::Uint8) spu->ram [cellIndex * COLOR_CHANNELS + 1],
                         (sf::Uint8) spu->ram [cellIndex * COLOR_CHANNELS + 2]);

    //updateMutex.lock ();
    memoryCells [cellIndex].setFillColor (color);
    //updateMutex.unlock ();

    RETURN NO_PROCESSOR_ERRORS;
}
