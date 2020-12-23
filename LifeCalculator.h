#ifndef LIFECALCULATOR_H
#define LIFECALCULATOR_H
#include <CL/cl.h>
#include <iostream>
#include <math.h>

class LifeCalculator
{
public:
    void funkcija();
    char* readKernelSource(const char* filename);
};


#endif // LIFECALCULATOR_H
