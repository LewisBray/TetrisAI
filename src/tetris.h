#ifndef TETRIS_H
#define TETRIS_H

#include "colour.h"
#include "maths.h"
#include "types.h"

namespace Tetris {
	struct Tetrimino {
        struct Blocks {
            static constexpr i32 COUNT = 4;
            Coordinates top_left_coordinates[COUNT];
        };

        enum Type { T = 0, L = 1, RL = 2, S = 3, Z = 4, SQUARE = 5, LONG = 6, COUNT = 7 };
        
        Blocks blocks;
		Vec2 centre;
        Type type;
    };

    enum Rotation { CLOCKWISE = 0, ANTI_CLOCKWISE = 1 };
    Colour piece_colour(Tetrimino::Type type);
    Tetrimino construct_tetrimino(Tetrimino::Type type, const Coordinates& top_left);
    Tetrimino shift(Tetrimino tetrimino, const Coordinates& shift);
    Tetrimino rotate(Tetrimino tetrimino, Rotation rotation);

    struct Grid {
        using Cell = Colour;
        static constexpr i32 ROW_COUNT = 18;
        static constexpr i32 COLUMN_COUNT = 10;
        Cell cells[ROW_COUNT][COLUMN_COUNT];
    };

    bool is_empty_cell(const Grid::Cell& grid_cell);
    i32 remove_completed_rows(Grid& grid);  // TODO: don't like mutable ref
    bool collision(const Tetrimino& tetrimino, const Grid& grid);
    void merge(const Tetrimino& tetrimino, Grid& grid); // TODO: ditto
    bool resolve_rotation_collision(Tetrimino& tetrimino, const Grid& grid);
}

#endif
