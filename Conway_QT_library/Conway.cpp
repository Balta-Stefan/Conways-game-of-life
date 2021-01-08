#include "Conway.h"


Conway::Conway(int numOfHorizontalGroups, int numOfRows, unsigned char** GUIworld) //GUIWorld is address of the pointer to the array
{
    this->numOfHorizontalGroups = numOfHorizontalGroups;
    this->numOfRows = numOfRows;

    sizeOfTheWorld = numOfHorizontalGroups * numOfRows;
    world = createNewWorld();
    temporaryNewGeneration = createNewWorld();

    *GUIworld = createNewWorld();
    //perform GPU initialization
    GPUInit();
}


void Conway::littleToBigEndian(unsigned int* toConvert)
{
    *toConvert = ((*toConvert) >> 24) | (((*toConvert) << 8) & 0x00ff0000) | (((*toConvert) >> 8) & 0x0000ff00) | ((*toConvert) << 24); //it's very important that unsigned int is used.Othervise, shifting to the right might lead to new ones.
}

void Conway::bigToLittleEndian(unsigned int* toConvert)
{
    *toConvert = ((*toConvert) >> 24) | (((*toConvert) >> 8) & 0x0000ff00) | (((*toConvert) << 8) & 0x00ff0000) | ((*toConvert) << 24);
}
void Conway::startSimulation(unsigned char* world)
{
    cl_int err;
    std::memcpy(this->world, world, sizeOfTheWorld);


    // Write our data set into the arrays in device memory
    err = clEnqueueWriteBuffer(queue, GPUCurrentGeneration, CL_TRUE, 0, sizeOfTheWorld, world, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(queue, GPUNewGeneration, CL_TRUE, 0, sizeOfTheWorld, world, 0, NULL, NULL);
    clFinish(queue);  //not necessary because the calls are blocking (flag CL_TRUE)

    //waitCondition.notify_all();
}

void Conway::skipGenerations(int generation)
{
    jumpToGeneration = generation;
}

void Conway::GPUInit()
{
    cl_int err;

    // Bind to platform
    err = clGetPlatformIDs(1, &cpPlatform, NULL);

    // Get ID for the device
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

    // Create a context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

    // Create a command queue
    queue = clCreateCommandQueue(context, device_id, 0, &err);

    char* kernelSource = readKernelSource("singleByteConway.cl");

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);
    delete[] kernelSource;

    // Build the program executable
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);


    if (err != CL_SUCCESS)
    {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        // Allocate memory for the log
        char* log = new char[log_size];

        // Get the log
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        delete[] log;
        return; //notify the GUI using signal/slot mechanism.The passed value should be a string.
    }

    // Create the compute kernel in the program we wish to run
    kernel = clCreateKernel(program, "simulateLife", &err);

    // Number of work items in each local work group
    clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(localSize), &localSize, NULL);

    // Number of total work items - localSize must be devisor
    int iterationsPerRow = (numOfHorizontalGroups - 2);
    int numOfIterations = (numOfRows - 2) * iterationsPerRow; //total number of threads
    maxID = numOfIterations;

    globalSize = std::ceil((double)numOfIterations / localSize) * localSize;

    // Create the worlds in device memory for our calculation
    GPUCurrentGeneration = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeOfTheWorld, NULL, NULL);
    GPUNewGeneration = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeOfTheWorld, NULL, NULL);

    //set 2nd and 3rd argument.It's possible that this is wrong and that they have to be set for each iteration.
    err |= clSetKernelArg(kernel, 2, sizeof(int), &numOfHorizontalGroups);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &maxID);
    clFinish(queue);
}



unsigned char* Conway::simulateLifeGPU()
{
    unsigned char* newGeneration = new unsigned char[sizeOfTheWorld];
    cl_int err;
    //swap current generation with next generation

    long long repeatCalculations = 1;

    if (currentGeneration < jumpToGeneration)
    {
        repeatCalculations = jumpToGeneration - currentGeneration;
        currentGeneration = jumpToGeneration;
    }
    else
        currentGeneration++;

    for (int i = 0; i < repeatCalculations; i++)
    {
        cl_mem temp = GPUNewGeneration;
        GPUNewGeneration = GPUCurrentGeneration;
        GPUCurrentGeneration = temp;


        //qInfo() << "MAX ID=" << maxID;
        //qInfo() << "CL_DEVICE_MAX_WORK_GROUP_SIZE INVOCATION ERROR:" << TranslateOpenCLError(clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &maxSize, NULL));
        //qInfo() << "MAX WORKGROUP SIZE=" << maxSize;
        //simulateLife(__global unsigned char* currentGeneration, __global unsigned char* nextGeneration, int worldWidth, int worldHeight)
        //set kernel arguments
        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &GPUCurrentGeneration); //should it be sizeof(cl_mem) or sizeof(cl_mem*)?
        err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &GPUNewGeneration);
        //It's possible that 3rd and 4th argument have to be set in every generation.

        //err |= clSetKernelArg(kernel, 2, sizeof(int), &numOfHorizontalGroups);
        //err |= clSetKernelArg(kernel, 3, sizeof(int), &maxID);


        //qInfo() << "Error after setting arguments: " << TranslateOpenCLError(err);

        // Execute the kernel over the entire range of the data set
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
        //qInfo() << "AFTER RUNNING KERNEL: GLOBAL SIZE=" << globalSize << ", local size=" << localSize;
        //qInfo() << "=====================Error after executing the kernel: " << TranslateOpenCLError(err);

        // Wait for the command queue to get serviced before reading back results
        clFinish(queue);
    }
    // Read the results from the device
    clEnqueueReadBuffer(queue, GPUNewGeneration, CL_TRUE, 0, sizeOfTheWorld, newGeneration, 0, NULL, NULL);

    clFinish(queue);
    return newGeneration;
}

unsigned char* Conway::experimentalSerialCPU()
{
    int iterationsPerRow = (numOfHorizontalGroups - 2);
    int numOfIterations = (numOfRows - 2) * iterationsPerRow;

    long long repeatCalculations = 1;

    if (currentGeneration < jumpToGeneration)
    {
        repeatCalculations = jumpToGeneration - currentGeneration;
        currentGeneration = jumpToGeneration;
    }
    else
        currentGeneration++;

    for (int i = 0; i < repeatCalculations; i++)
    {
        for (int ID = 0; ID < numOfIterations; ID++)
        {
            int row = ID / iterationsPerRow + 1; //row coordinate is index of the ghost byte in a certain row
            int column = ID % iterationsPerRow; //index of the beginning of our integer in the row

            //COLUMN MUST START FROM THE LEFT BYTE (whose LSB is left neighbour of the important central byte's MSB)!!!!

            int upperRowIndex = (row - 1) * numOfHorizontalGroups;// + column;
            int currentRowIndex = row * numOfHorizontalGroups;// + column;
            int lowerRowIndex = (row + 1) * numOfHorizontalGroups;// + column;

            //unsigned char* bytePointer = world;

            unsigned char upperRowLeftByte = world[upperRowIndex + column];
            unsigned char upperRowMiddleByte = world[upperRowIndex + column + 1];
            unsigned char upperRowRightByte = world[upperRowIndex + column + 2];

            unsigned char middleRowLeftByte = world[currentRowIndex + column];
            unsigned char middleRowMiddleByte = world[currentRowIndex + column + 1];
            unsigned char middleRowRightByte = world[currentRowIndex + column + 2];

            unsigned char lowerRowLeftByte = world[lowerRowIndex + column];
            unsigned char lowerRowMiddleByte = world[lowerRowIndex + column + 1];
            unsigned char lowerRowRightByte = world[lowerRowIndex + column + 2];


            unsigned int upperThreeRows = 0, middleThreeRows = 0, lowerThreeRows = 0;

            upperThreeRows |= (((unsigned int)upperRowLeftByte) << 9) | (((unsigned int)upperRowMiddleByte) << 1) | (((unsigned int)upperRowRightByte) >> 7);
            middleThreeRows |= (((unsigned int)middleRowLeftByte) << 9) | (((unsigned int)middleRowMiddleByte) << 1) | (((unsigned int)middleRowRightByte) >> 7);
            lowerThreeRows |= (((unsigned int)lowerRowLeftByte) << 9) | (((unsigned int)lowerRowMiddleByte) << 1) | (((unsigned int)lowerRowRightByte) >> 7);


            unsigned char newCentralRow = 0;


            //evaluate inner 8 cells
            for (int i = 0; i < 8; i++) //8 iterations because that's how many bits (cells) are evaluated in a 4 byte integer (only the middle 2 bytes are evaluated)
            {
                unsigned int neighbours = 0;

                //might be better to store the results in different variables if it could harm pipelining (if it exists on GPUs)
                //evaluate column on the right
                neighbours += (upperThreeRows >> i) & 0x01;
                neighbours += (middleThreeRows >> i) & 0x01;
                neighbours += (lowerThreeRows >> i) & 0x01;

                //evaluate column that contains the cell that is being evaluated
                neighbours += (upperThreeRows >> (i + 1)) & 0x01;
                unsigned int alive = (middleThreeRows >> (i + 1)) & 0x01;
                neighbours += (lowerThreeRows >> (i + 1)) & 0x01;

                //evaluate the left column
                neighbours += (upperThreeRows >> (i + 2)) & 0x01;
                neighbours += (middleThreeRows >> (i + 2)) & 0x01;
                neighbours += (lowerThreeRows >> (i + 2)) & 0x01;

                //if(alive == 1)
                    //qInfo() << "IS ALIVE";

                unsigned char newCellState;

                if (neighbours == 3)
                    newCellState = 1; //alive
                else if ((neighbours == 2) && (alive == 1))
                    newCellState = 1; //alive
                else
                    newCellState = 0; //dead

                newCentralRow |= newCellState << i;
            }


            //bytePointer = newGeneration; //now write results to the next generation
            temporaryNewGeneration[currentRowIndex + column + 1] = newCentralRow;

        }
        unsigned char* temp = world;
        world = temporaryNewGeneration;
        temporaryNewGeneration = temp;
    }
    unsigned char* worldForGUI = createNewWorld();
    std::memcpy(worldForGUI, world, sizeOfTheWorld);

    return worldForGUI;
}



unsigned char* Conway::simulateLifeSerialCPU()
{
    //bool newWorld[numOfVerticalCells+2][numOfHorizontalCells+2];

    int iterationsPerRow = (numOfHorizontalGroups - 2) / 2;

    long long repeatCalculations = 1;

    if (currentGeneration < jumpToGeneration)
    {
        repeatCalculations = jumpToGeneration - currentGeneration;
        currentGeneration = jumpToGeneration;
    }
    else
        currentGeneration++;

    for (int i = 0; i < repeatCalculations; i++)
    {
        //Because 4 bytes at a time are read, reading in the next iteration will lead to the loss of the 3rd byte from previous iteration.It has to be preserved.

        //number of iterations per row is equal to half of the number of visible groups (subtracting by 2 above because invisible bytes aren't counted)
        for (int ID = 0; ID < ((numOfRows - 2) * iterationsPerRow); ID++) //numOfRows also contains ghost rows.These shouldn't be counted.
        //for(int ID = 0; ID < 1; ID++)
        {
            int row = ID / iterationsPerRow + 1; //row coordinate is index of the ghost byte in a certain row
            int column = (ID % iterationsPerRow) * 2; //index of the beginning of our integer in the row

            int upperRowIndex = (row - 1) * numOfHorizontalGroups + column;
            int currentRowIndex = row * numOfHorizontalGroups + column;
            int lowerRowIndex = (row + 1) * numOfHorizontalGroups + column;

            unsigned char* bytePointer = world;

            //unsigned ints are used because only zeros fill out the void when shifting
            unsigned int upperRows = *((unsigned int*)(bytePointer + upperRowIndex));
            unsigned int currentRows = *((unsigned int*)(bytePointer + currentRowIndex));
            unsigned int lowerRows = *((unsigned int*)(bytePointer + lowerRowIndex));


            //each of the variables below contains 32 cells.Only the middle 16 are of interest.
            //First byte's LSB is the neighbour of the 2nd byte's MSB
            //Last byte's MSB is the neighbour of the 3rd byte's LSB


            littleToBigEndian(&upperRows);
            littleToBigEndian(&currentRows);
            littleToBigEndian(&lowerRows);


            //all game of life is done below

            unsigned int newCentralRow = 0;

            //lower 7 bits aren't needed (neither are the upper 7 bits).
            for (int i = 0; i < 16; i++) //16 iterations because that's how many bits (cells) are evaluated in a 4 byte integer (only the middle 2 bytes are evaluated)
            {
                unsigned int neighbours = 0;

                //might be better to store the results in different variables if it could harm pipelining (if it exists on GPUs)
                neighbours += (upperRows >> (7 + i)) & 0x01;
                neighbours += (currentRows >> (7 + i)) & 0x01;
                neighbours += (lowerRows >> (7 + i)) & 0x01;

                neighbours += (upperRows >> (7 + i + 1)) & 0x01;
                unsigned int alive = (currentRows >> (7 + i + 1)) & 0x01;
                neighbours += (lowerRows >> (7 + i + 1)) & 0x01;

                neighbours += (upperRows >> (7 + i + 2)) & 0x01;
                neighbours += (currentRows >> (7 + i + 2)) & 0x01;
                neighbours += (lowerRows >> (7 + i + 2)) & 0x01;


                unsigned int temp;

                if (neighbours == 3)
                    temp = 1; //alive
                else if ((neighbours == 2) && (alive == 1))
                    temp = 1; //alive
                else
                    temp = 0; //dead

                newCentralRow |= temp << (8 + i);
            }

            //central row now has to become newCentralRow.What about endianess?It probably has to be converted to a different endianness....

            unsigned short newRow = (unsigned short)((newCentralRow & 0x00ffff00) >> 8);
            //swap endianness of newRow
            newRow = ((newRow & 0x00ff) << 8) | ((newRow & 0xff00) >> 8);

            bytePointer = temporaryNewGeneration; //now write results to the next generation
            *((unsigned short*)(bytePointer + currentRowIndex + 1)) = newRow;
        }
        unsigned char* temp = world;
        world = temporaryNewGeneration;
        temporaryNewGeneration = temp;
    }
    unsigned char* worldForGUI = createNewWorld();
    std::memcpy(worldForGUI, world, sizeOfTheWorld);

    return worldForGUI;
}

Conway::~Conway()
{
    //deallocate all the memory from the host and GPU`
// release OpenCL resources
    clReleaseMemObject(GPUCurrentGeneration);
    clReleaseMemObject(GPUNewGeneration);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);


    delete[] world;
    delete[] temporaryNewGeneration;
}

/*void Conway::deallocate()
{
    //deallocate all the memory from the host and GPU
    // release OpenCL resources
    clReleaseMemObject(GPUCurrentGeneration);
    clReleaseMemObject(GPUNewGeneration);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    delete[] world;
    delete[] temporaryNewGeneration;
}*/


unsigned char* Conway::createNewWorld()
{
    unsigned char* array = new unsigned char[sizeOfTheWorld]();
    return array;
}


char* Conway::readKernelSource(const char* filename)
{
    char* kernelSource = nullptr;
    long length;
    FILE* f = fopen(filename, "r");

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        kernelSource = new char[length];
        //kernelSource = (char*)calloc(length, sizeof(char));
        if (kernelSource)
            fread(kernelSource, 1, length, f);
        fclose(f);
    }
    return kernelSource;
}


const char* Conway::TranslateOpenCLError(cl_int errorCode)
{
    switch (errorCode)
    {
    case CL_SUCCESS:                            return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:                   return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:               return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:             return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:                   return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:                 return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:       return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:                   return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:              return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:              return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:                        return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "CL_MISALIGNED_SUB_BUFFER_OFFSET";                          //-13
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:    return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";   //-14
    case CL_COMPILE_PROGRAM_FAILURE:            return "CL_COMPILE_PROGRAM_FAILURE";                               //-15
    case CL_LINKER_NOT_AVAILABLE:               return "CL_LINKER_NOT_AVAILABLE";                                  //-16
    case CL_LINK_PROGRAM_FAILURE:               return "CL_LINK_PROGRAM_FAILURE";                                  //-17
    case CL_DEVICE_PARTITION_FAILED:            return "CL_DEVICE_PARTITION_FAILED";                               //-18
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:      return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";                         //-19
    case CL_INVALID_VALUE:                      return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:                return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:                   return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:                     return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:                    return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:           return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:              return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:                   return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:                 return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:                 return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:                    return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:                     return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:              return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:                    return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:         return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:                return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:          return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL:                     return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:                  return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:                  return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:                   return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:                return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:             return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:            return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:             return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:              return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:            return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:                      return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:                  return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:                  return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE:                return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:                  return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:           return "CL_INVALID_GLOBAL_WORK_SIZE";                           //-63
    case CL_INVALID_PROPERTY:                   return "CL_INVALID_PROPERTY";                                   //-64
    case CL_INVALID_IMAGE_DESCRIPTOR:           return "CL_INVALID_IMAGE_DESCRIPTOR";                           //-65
    case CL_INVALID_COMPILER_OPTIONS:           return "CL_INVALID_COMPILER_OPTIONS";                           //-66
    case CL_INVALID_LINKER_OPTIONS:             return "CL_INVALID_LINKER_OPTIONS";                             //-67
    case CL_INVALID_DEVICE_PARTITION_COUNT:     return "CL_INVALID_DEVICE_PARTITION_COUNT";                     //-68
                                                                                                                //    case CL_INVALID_PIPE_SIZE:                  return "CL_INVALID_PIPE_SIZE";                                  //-69
                                                                                                                //    case CL_INVALID_DEVICE_QUEUE:               return "CL_INVALID_DEVICE_QUEUE";                               //-70

    default:
        return "UNKNOWN ERROR CODE";
    }
}
