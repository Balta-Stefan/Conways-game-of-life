#ifndef LIFECALCULATOR_H
#define LIFECALCULATOR_H
#include <iostream>
#include <math.h>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <cstring>
#include "Conway.h"

//would it make sense to keep all 8 neighbours of a cell inside a byte?That would allow the usage of a lookup table.The table would have 2^8=256 entries.
    //even better, instead of making a lookup table for neighbours of ONE cell, make a lookup table for a block of arbitrary size.
        //the problem - exponential growth in size (2^block_size).
        //another problem - how to modify all the blocks for the new generation?

/*
 *Bit packing:
 *      -packingType is a data type used for packing (unsigned char in my case, might try larger types like short, int, long)
 *      -offset is used to skip the first ghost row and it is equal to: worldWidth/sizeof(packingType). worldWidth = realWorldWidth + 2
 *      -coordinate of our cell, in the array of packingType variables, is:
 *
 *-1 GPU thread works with an integer (4 bytes):
 *      -1st byte's LSB is neighbour of the 2nd byte's MSB
 *      -4th byte's MSB is neighbour of the 3rd byte's LSB
 *      -therefore, only 2 bytes are evaluated (that's 2*8=16 cells)
 *
 *      -total number of threads = realNumberOfRows * realRowWidth / 2 assuming that the real world width is a multiple of 8!Invisible rows aren't included here.
 *      -reading starts from the first ghost column
 *
 * */

class LifeCalculator : public QThread
{
    Q_OBJECT
public:
    //LifeCalculator(int numOfHorizontalCells, int numOfVerticalCells);
    void init(int numOfHorizontalGroups, int numOfRows, unsigned char** GUIWorld);
    void pause(bool pause);
    void startSimulation(unsigned char* world);
    void stop();
    void skipGenerations(int generation);
    ~LifeCalculator();

protected:
    void run() override;

private:
    Conway* simulator = nullptr;
    bool paused = true, runSimulation = true; //used to run the calculations

    QMutex startMutex;
    QWaitCondition waitCondition;

    static const int maxNumberOfPrecomputedGenerations=50;

signals:
    void sendNewWorld(unsigned char* newGeneration);
};


#endif // LIFECALCULATOR_H
