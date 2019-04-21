#pragma once

#include "arraybuffer.hpp"
#include "indexbuffer.hpp"
#include "position.hpp"
#include "texture2d.h"
#include "program.h"
#include "colour.h"

// I don't like writing function objects when lambdas exist but I ran into
// some issues.  To have a function that returns a lambda, you can use a
// return type of auto, however, if you specify the declaration in a header
// file and the definition in a translation unit then you have to specify
// the return type or the compiler can't resolve the symbols.  Using
// std::function as a return type yielded a strange compiler error where
// std::function is trying to invoke the copy constructors of things like
// the array buffer which are explicity deleted.  So this will have to do
// for now until I come up with something.
class BlockDrawer
{
public:
    BlockDrawer();

    void operator()(const Position<int>& blockTopLeft, const Colour& colour);

private:
    ArrayBuffer<float, 16> vertices_{ {
            // vertex       // texture
            0.0f, 0.0f,     0.0f, 0.0f,
            0.0f, 0.0f,     1.0f, 0.0f,
            0.0f, 0.0f,     1.0f, 1.0f,
            0.0f, 0.0f,     0.0f, 1.0f
        } };

    const Program program_;
    const Texture2d texture_{ ".\\Images\\block.jpg", Texture2d::ImageType::JPG };
    const IndexBuffer<unsigned, 6> indices_{ { 0, 1, 2, 2, 3, 0 } };
};
