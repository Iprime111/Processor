#include <SFML/Config.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

#include "CustomAssert.h"
#include "GraphicsProvider.h"
#include "CommonModules.h"
#include "Logger.h"
#include "MessageHandler.h"
#include "SPU.h"

static sf::RectangleShape memoryCells [VRAM_SIZE / 3];

static sf::Mutex updateMutex = {};

static ProcessorErrorCode InitCells ();

ProcessorErrorCode RenderLoop (sf::RenderWindow* window, SPU *spu, sf::Mutex *workMutex) {
    PushLog (2);

    window->setActive (true);

    ProgramErrorCheck (InitCells (), "Error occuried while initializing ram graphics");


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
        for (size_t cellIndex = 0; cellIndex < VRAM_SIZE / 3; cellIndex++) {
            window->draw (memoryCells [cellIndex]);
        }
        updateMutex.unlock ();

        window->display ();
    }

    RETURN NO_PROCESSOR_ERRORS;
}

static ProcessorErrorCode InitCells () {
    PushLog (4);

    updateMutex.lock ();

    for (size_t cellIndex = 0; cellIndex < VRAM_SIZE / 3; cellIndex++) {
        memoryCells [cellIndex].setPosition (LEFT_OFFSET + CELLS_DISTANCE * (float) (cellIndex / (size_t) CELLS_BY_LINE), TOP_OFFSET + CELLS_DISTANCE * (float) (cellIndex % CELLS_BY_LINE));

        sf::Color outlineColor (200, 200, 200);
        memoryCells [cellIndex].setFillColor    (sf::Color::Black);

        memoryCells [cellIndex].setOutlineColor (outlineColor);
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

    size_t cellIndex = ramAddress / 3;

    sf::Color color ((sf::Uint8) spu->ram [cellIndex * 3], (sf::Uint8) spu->ram [cellIndex * 3 + 1], (sf::Uint8) spu->ram [cellIndex * 3 + 2]);

    updateMutex.lock ();
    memoryCells [cellIndex].setFillColor (color);
    updateMutex.unlock ();

    RETURN NO_PROCESSOR_ERRORS;
}
