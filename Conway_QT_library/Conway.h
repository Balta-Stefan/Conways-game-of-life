#ifndef CONWAY_H
#define CONWAY_H

#include <CL/cl.h>
#include <cmath>
#include <stdio.h>
#include <cstring>

class Conway
{
public:
    Conway(int numOfHorizontalGroups, int numOfRows, unsigned char** GUIworld);
    ~Conway();
    unsigned char* simulateLifeSerialCPU();
    unsigned char* simulateLifeGPU();
    unsigned char* experimentalSerialCPU();
    //void pause(bool pause); add to GUI
    void startSimulation(unsigned char* world);
    //void stop(); add to gui
    void skipGenerations(int generation);

private:
    unsigned char* world;
    unsigned char* temporaryNewGeneration;
    int numOfHorizontalGroups, numOfRows, sizeOfTheWorld;
    long long currentGeneration = 1;
    long long jumpToGeneration = 0; //check whether current generation is lower than jumpToGeneration.First generation will be 1.

    static const int maxNumberOfPrecomputedGenerations = 50;

    unsigned char* createNewWorld();
    void GPUInit();
    char* readKernelSource(const char* filename);
    //void deallocate();

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


    const char* TranslateOpenCLError(cl_int errorCode);
    void littleToBigEndian(unsigned int* toConvert);
    void bigToLittleEndian(unsigned int* toConvert);

    int maxID;

};



#endif // CONWAY_H
