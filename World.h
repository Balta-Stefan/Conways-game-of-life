#ifndef WORLD_H
#define WORLD_H

class World
{
public:
    bool operator()(int row, int column);
    World(bool* world, int width, int height);

private:
    bool* world;
    int width, height;
};


#endif // WORLD_H
