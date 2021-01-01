#include <LifeCalculator.h>
#include <QDebug>



/*GPGPU algorithm:
 *Ova klasa ce se vrtiti u zasebnom thread-u.
 *run() metoda ce biti zaduzena za poliranje GPU-a.Ona ce preuzimati rezultate obrade.
 *      -run ce uzimati u obzir jumpToGeneration kako bi jednostavno preskakao nepotrebne alokacije i kopiranja iz VRAM-a GPU-a na RAM kada zelimo skociti na neku generaciju.
 *      -ova metoda ce obavjestavati Canvas koristeci signal/slot mehanizam o novoj generaciji (poslace pokazivac bool**).Canvas klasa je zaduzena za konektovanje signala i slota.
 *
 *Promjene koje korisnik unosi za vrijeme izvrsavanja se ne uzimaju u obzir.Jednom pokrenuta simulacija se ne moze vise modifikovati.
 *Kako ograniciti broj unaprijed izracunatih generacija?Za to je zaduzena Canvas klasa.
 *
 * */


/*LifeCalculator::LifeCalculator(int numOfHorizontalCells, int numOfVerticalCells)
{
    qInfo() << "LifeCalculator ctor";
    this->numOfHorizontalCells = numOfHorizontalCells;
    this->numOfVerticalCells = numOfVerticalCells;

    world = createNewWorld();
}*/

void LifeCalculator::startSimulation(bool* world)
{
    cl_int err;

    // Write our data set into the arrays in device memory
    err = clEnqueueWriteBuffer(queue, GPUCurrentGeneration, CL_TRUE, 0, numOfHorizontalCells*numOfVerticalCells*sizeof(bool), world, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue, GPUNewGeneration, CL_TRUE, 0, numOfHorizontalCells*numOfVerticalCells*sizeof(bool), world, 0, NULL, NULL);
    qInfo() << "GPU allocation error: " << err;

    this->world = world;
    paused = false;
    waitCondition.notify_all();
}

void LifeCalculator::GPUInit()
{
    // Length of vectors
    unsigned int n = numOfHorizontalCells*numOfVerticalCells;

    // Host input vectors
    //bool** GPUWorld;
    // Host output vector
    //bool* hostWorld;



    size_t bytes = n * sizeof(bool); // Size, in bytes, of the world

    // Allocate memory for the world on host
    //hostWorld = (bool*)calloc(n, sizeof(bool));

    size_t globalSize, localSize;
    cl_int err;

    // Bind to platform
    err = clGetPlatformIDs(1, &cpPlatform, NULL);
    qInfo() << "err after binding to platform: " << TranslateOpenCLError(err);

    // Get ID for the device
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    qInfo() << "err after getting device ID: " << TranslateOpenCLError(err);

    // Create a context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

    // Create a command queue
    //cl_queue_properties prop[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT,  CL_QUEUE_SIZE, 128, 0 };
    queue = clCreateCommandQueue(context, device_id, 0, &err);

    char* kernelSource = readKernelSource("Conway.cl");
    qInfo() << "kernel source: " << kernelSource;

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);


    // Build the program executable
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    qInfo() << "err after building program: " << TranslateOpenCLError(err);


    if (err)
    {
        qInfo() << "ERROR!";
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        qInfo() << log_size;

        // Allocate memory for the log
        char* log = (char*)malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        qInfo() << "log= " << log;
        //printf("%s\n", log);

        free(log);
        return; //notify the GUI using signal/slot mechanism.The passed value should be a string.
    }

    // Create the compute kernel in the program we wish to run
    kernel = clCreateKernel(program, "doLife", &err);
    qInfo() << " clGetKernelWorkGroupInfo  return value: " <<  clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(localSize), &localSize, NULL);
    qInfo() << "localSize:" << localSize;

    // Number of work items in each local work group
     clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(localSize), &localSize, NULL);
    //localSize = 512;

    // Number of total work items - localSize must be devisor
    globalSize = (size_t)ceil(n / (float)localSize) * localSize;

    // Create the worlds in device memory for our calculation
    GPUCurrentGeneration = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, NULL);
    GPUNewGeneration = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, NULL);

    // Write our data set into the arrays in device memory
    //err = clEnqueueWriteBuffer(queue, GPUCurrentGeneration, CL_TRUE, 0, bytes, hostWorld, 0, NULL, NULL);
    //err |= clEnqueueWriteBuffer(queue, GPUNewGeneration, CL_TRUE, 0, bytes, hostWorld, 0, NULL, NULL);

    // Set the arguments to our compute kernel
    /*err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &n);*/

    clFinish(queue);

    // Execute the kernel over the entire range of the data set
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);

    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);

    // Read the results from the device
    //clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0, bytes, h_c, 0, NULL, NULL);

    clFinish(queue);


    free(kernelSource);
}

void LifeCalculator::init(int numOfHorizontalCells, int numOfVerticalCells)
{
    this->numOfHorizontalCells = numOfHorizontalCells;
    this->numOfVerticalCells = numOfVerticalCells;
    world = createNewWorld();

    //perform GPU initialization
    GPUInit();
}

void LifeCalculator::stop()
{
    runSimulation = false;
}

void LifeCalculator::skipGenerations(int generation)
{
    jumpToGeneration = generation;
}

void LifeCalculator::run()
{
    //first do initialization for GPU

    startMutex.lock(); //in order to wait on this mutex, this thread has to hold the mutex

    while(runSimulation)
    {
        while(paused)
        {
            waitCondition.wait(&startMutex);
        }
        bool* newWorld = doLife();
        if(currentGeneration > jumpToGeneration)
        {
            emit sendNewWorld(newWorld);
            msleep(100);
            //emit sendNewWorld(ptr);
        }
        else if(currentGeneration == jumpToGeneration)
        {
            jumpToGeneration = 0;
            emit sendNewWorld(newWorld);
        }
        currentGeneration++;
        qInfo() << "Current generation: " << currentGeneration;
    }
}

void LifeCalculator::deallocate()
{
    //deallocate all the memory from the host and GPU
    // release OpenCL resources
    clReleaseMemObject(GPUCurrentGeneration);
    clReleaseMemObject(GPUNewGeneration);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}


void LifeCalculator::pause(bool pause)
{
    if((paused == true) && (pause == true))
        return;

    if((paused == true) && (pause == false)) //continue
    {
        paused = false;
        waitCondition.notify_all();
        return;
    }
    paused = pause;
}

bool* LifeCalculator::createNewWorld()
{
    bool *array = new bool[numOfVerticalCells*numOfHorizontalCells]();
    return array;
}



char* LifeCalculator::readKernelSource(const char* filename)
{
    char* kernelSource = nullptr;
    long length;
    FILE* f = fopen(filename, "r");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        kernelSource = (char*)calloc(length, sizeof(char));
        if (kernelSource)
            fread(kernelSource, 1, length, f);
        fclose(f);
    }
    return kernelSource;
}

int LifeCalculator::numOfNeighbours(int x, int y)
{
    int neighbours = 0;
    if(world[(y-1)*numOfHorizontalCells+x-1] == true)
        neighbours++;
    if(world[(y-1)*numOfHorizontalCells+x] == true)
        neighbours++;
    if(world[(y-1)*numOfHorizontalCells+x+1] == true)
        neighbours++;

    if(world[y*numOfHorizontalCells+x-1] == true)
        neighbours++;
    if(world[y*numOfHorizontalCells+x+1] == true)
        neighbours++;

    if(world[(y+1)*numOfHorizontalCells+x-1] == true)
        neighbours++;
    if(world[(y+1)*numOfHorizontalCells+x] == true)
        neighbours++;
    if(world[(y+1)*numOfHorizontalCells+x+1] == true)
        neighbours++;

    /*if(world[y-1][x-1] == true)
        neighbours++;
    if(world[y-1][x] == true)
        neighbours++;
    if(world[y-1][x+1] == true)
        neighbours++;

    if(world[y][x-1] == true)
        neighbours++;
    if(world[y][x+1] == true)
        neighbours++;

    if(world[y+1][x-1] == true)
        neighbours++;
    if(world[y+1][x] == true)
        neighbours++;
    if(world[y+1][x+1] == true)
        neighbours++;*/


    return neighbours;
}
bool* LifeCalculator::doLife()
{
    //bool newWorld[numOfVerticalCells+2][numOfHorizontalCells+2];

    bool *newWorld = createNewWorld();

    for(int j = 1; j < numOfVerticalCells-1; j++)
        for(int i = 1; i < numOfHorizontalCells-1; i++)
        {
            int neighbours = numOfNeighbours(i, j);

            if(neighbours == 3)
                newWorld[j*numOfHorizontalCells+i] = true;
                //newWorld[j][i] = true;
            else if((neighbours == 2) && (world[j*numOfHorizontalCells+i] == true))
                newWorld[j*numOfHorizontalCells+i] = true;
            else
                newWorld[j*numOfHorizontalCells+i] = false;
        }
    //deleteArray(world);
    //notify the GUI thread


    world = newWorld;
    return newWorld;
}

void LifeCalculator::funkcija()
{
    // Length of vectors
    unsigned int n = 10000000;

    // Host input vectors
    float* h_a;
    float* h_b;
    // Host output vector
    float* h_c;

    // Device input buffers
    cl_mem d_a;
    cl_mem d_b;
    // Device output buffer
    cl_mem d_c;

    cl_platform_id cpPlatform;        // OpenCL platform
    cl_device_id device_id;           // device ID
    cl_context context;               // context
    cl_command_queue queue;           // command queue
    cl_program program;               // program
    cl_kernel kernel;                 // kernel

    size_t bytes = n * sizeof(float); // Size, in bytes, of each vector

    // Allocate memory for each vector on host
    h_a = (float*)malloc(bytes);
    h_b = (float*)malloc(bytes);
    h_c = (float*)malloc(bytes);

    // Initialize vectors on host
    for (unsigned i = 0; i < n; i++)
    {
        h_a[i] = sinf((float)i) * sinf((float)i);
        h_b[i] = cosf((float)i) * cosf((float)i);
    }

    size_t globalSize, localSize;
    cl_int err;


    // Bind to platform
    err = clGetPlatformIDs(1, &cpPlatform, NULL);

    // Get ID for the device
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

    // Create a context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

    // Create a command queue
    //cl_queue_properties prop[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT,  CL_QUEUE_SIZE, 128, 0 };
    queue = clCreateCommandQueue(context, device_id, 0, &err);

    char* kernelSource = readKernelSource("VectorAdd.cl");
    qInfo() << "kernel source: " << kernelSource;

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char**)&kernelSource, NULL, &err);

    // Build the program executable
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);


    if (err)
    {
        qInfo() << "ERROR!";
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        qInfo() << log_size;

        // Allocate memory for the log
        char* log = (char*)malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        qInfo() << log;
        //printf("%s\n", log);

        free(log);
        return;
    }

    // Create the compute kernel in the program we wish to run
    kernel = clCreateKernel(program, "vecAdd", &err);
    qInfo() << " clGetKernelWorkGroupInfo  return value: " <<  clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(localSize), &localSize, NULL);
    qInfo() << "localSize:" << localSize;

    // Number of work items in each local work group
    localSize = 512;

    // Number of total work items - localSize must be devisor
    globalSize = (size_t)ceil(n / (float)localSize) * localSize;

    // Create the input and output arrays in device memory for our calculation
    d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, bytes, NULL, NULL);
    d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, bytes, NULL, NULL);
    d_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, bytes, NULL, NULL);

    // Write our data set into the input array in device memory
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, bytes, h_a, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0, bytes, h_b, 0, NULL, NULL);

    // Set the arguments to our compute kernel
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &n);

    clFinish(queue);

    // Execute the kernel over the entire range of the data set
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);

    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);

    // Read the results from the device
    clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0, bytes, h_c, 0, NULL, NULL);

    clFinish(queue);

    //Sum up vector c and print result divided by n, this should equal 1 within error
    float sum = 0;
    for (unsigned i = 0; i < n; i++)
    {
        if (i < 10)
            qInfo() << h_c[i];
            //printf("%f ", h_c[i]);
        sum += h_c[i];
    }
    //printf("\nfinal result: %f\n", sum / n);
    qInfo() << "final result: " << sum/n;

    // release OpenCL resources
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_c);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    //release host memory
    free(h_a);
    free(h_b);
    free(h_c);
    free(kernelSource);
}

const char* LifeCalculator::TranslateOpenCLError(cl_int errorCode)
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
