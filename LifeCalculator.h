#ifndef LIFECALCULATOR_H
#define LIFECALCULATOR_H
#include <CL/cl.h>
#include <iostream>
#include <math.h>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class LifeCalculator : public QThread
{
    Q_OBJECT
public:
    //LifeCalculator(int numOfHorizontalCells, int numOfVerticalCells);
    void funkcija();
    bool* doLife();
    bool* world;
    void init(int numOfHorizontalCells, int numOfVerticalCells);
    void pause(bool pause);
    bool* getGeneration();
    void startSimulation(bool* world);
    void stop();
    void skipGenerations(int generation);

protected:
    void run() override;

private:
    bool* createNewWorld();
    bool paused = true, runSimulation = true; //used to run the calculations
    int numOfNeighbours(int x, int y);

    int numOfHorizontalCells, numOfVerticalCells;
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

    static const int maxNumberOfPrecomputedGenerations=50;
    const char* TranslateOpenCLError(cl_int errorCode);

signals:
    void sendNewWorld(bool* newGeneration);
};


#endif // LIFECALCULATOR_H
