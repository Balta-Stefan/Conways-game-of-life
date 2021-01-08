#include <LifeCalculator.h>
#include <QDebug>
#include <QTime>




void LifeCalculator::startSimulation(unsigned char* world)
{
    simulator->startSimulation(world);

    paused = false;
    waitCondition.notify_all();
}


void LifeCalculator::init(int numOfHorizontalGroups, int numOfRows, unsigned char** GUIWorld)
{
    if(simulator != nullptr)
        delete simulator;

    simulator = new Conway(numOfHorizontalGroups, numOfRows, GUIWorld);

}

void LifeCalculator::stop()
{
    runSimulation = false;
}

void LifeCalculator::skipGenerations(int generation)
{
    simulator->skipGenerations(generation);
}


void LifeCalculator::run()
{
    startMutex.lock(); //in order to wait on this mutex, this thread has to hold the mutex
    //QTime time;
    //int max = 55000;

    while(runSimulation)
    //while(currentGeneration < max)
    {
        while(paused)
        {
            waitCondition.wait(&startMutex);
            //time.start();
        }
        //unsigned char* newWorld = simulator->simulateLifeSerialCPU();
        unsigned char* newWorld = simulator->simulateLifeGPU();
        //unsigned char* newWorld = simulator->experimentalSerialCPU();
        //delete[] newWorld;
        /*if(currentGeneration > jumpToGeneration)
        {
            emit sendNewWorld(newWorld);
            msleep(45);
            //emit sendNewWorld(ptr);
        }
        else if(currentGeneration == jumpToGeneration)
        {
            jumpToGeneration = 0;
            emit sendNewWorld(newWorld);
        }
        currentGeneration++;
        qInfo() << "Current generation: " << currentGeneration;*/
        emit sendNewWorld(newWorld);
        qInfo() << "I have received new world";
        msleep(45);
        //break;
    }
    //int elapsed = time.elapsed();
    //qInfo() << "elapsed: " << elapsed << " generations/elapsed[g/ms]=" << (double)currentGeneration/elapsed;

    //GPGPU: elapsed=7286,  generations/elapsed[g/ms]= 6.86248, uchar: elapsed=5037,  generations/elapsed[g/ms]= 9.92654, packed: elapsed=491,  generations/elapsed[g/ms]= 10.1833
    //CPU serial: elapsed=26816,  generations/elapsed[g/ms]= 1.86456, uchar: elapsed=26797,  generations/elapsed[g/ms]= 1.86588, packed: elapsed=2131,  generations/elapsed[g/ms]= 2.34632 (WTF???)
    startMutex.unlock();
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




