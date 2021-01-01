#include <World.h>

bool World::operator()(int row, int column)
{
    return world[width*row + column];
}

World::World(bool* world, int width, int height)
{
    this->world = world;
    this->width = width;
    this->height = height;
}
