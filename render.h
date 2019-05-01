#pragma once

#include "tetris.h"

void renderScene(const Tetris::Tetrimino& tetrimino,
    const Tetris::Tetrimino& nextTetrimino,
    const Tetris::Grid& grid, int score, int rowsCleared);
