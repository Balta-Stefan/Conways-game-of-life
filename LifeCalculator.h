#ifndef LIFECALCULATOR_H
#define LIFECALCULATOR_H
#include <CL/cl.h>
#include <iostream>
#include <math.h>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

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
    void funkcija();
    unsigned char* simulateLifeSerialCPU();
    unsigned char* simulateLifeGPU();
    unsigned char* world;
    void init(int numOfHorizontalGroups, int numOfRows);
    void pause(bool pause);
    unsigned char* getGeneration();
    void startSimulation(unsigned char* world);
    void stop();
    void skipGenerations(int generation);

protected:
    void run() override;

private:
    unsigned char* createNewWorld();
    bool paused = true, runSimulation = true; //used to run the calculations
    int numOfNeighbours(int x, int y);

    int numOfHorizontalGroups, numOfRows;
    long long jumpToGeneration=0; //check whether current generation is lower than jumpToGeneration.First generation will be 1.
    long long currentGeneration=1;
    QMutex startMutex;
    QWaitCondition waitCondition;

    void GPUInit();
    char* readKernelSource(const char* filename);

    void deallocate();

    // Device input buffers
    cl_mem GPUCurrentGeneration;
    // Device output buffer
    cl_mem GPUNewGeneration;

    cl_platform_id cpPlatform;        // OpenCL platform
    cl_device_id device_id;           // device ID
    cl_context context;               // context
    cl_command_queue queue;           // command queue
    cl_program program;               // program
    cl_kernel kernel;                 // kernel
    size_t globalSize, localSize;//, sizeOfTheRealWorld;
    int sizeOfTheWorld;

    static const int maxNumberOfPrecomputedGenerations=50;
    const char* TranslateOpenCLError(cl_int errorCode);


    void littleToBigEndian(unsigned int* toConvert);
    void bigToLittleEndian(unsigned int* toConvert);

signals:
    void sendNewWorld(unsigned char* newGeneration);
};


#endif // LIFECALCULATOR_H
