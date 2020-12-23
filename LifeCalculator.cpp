#include <LifeCalculator.h>
#include <QDebug>

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
