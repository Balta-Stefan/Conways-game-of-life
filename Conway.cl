
/*
 *Kernel kao argumente prima:
 *      -blok memorije trenutne generacije
 *      -blok memorije naredne generacije
 *      -dimenzije svijeta
 *
 *
 * */

__kernel void simulateLife(__global unsigned char* currentGeneration, __global unsigned char* nextGeneration, int worldWidth, int worldHeight)
{
    //world dimensions will specify imaginary dimensions.They will account for 2 ghost rows and 2 ghost columns.
    //currentGeneration block will also contain ghost columns and rows.
    //total number of threads = realWorldWidth * realWorldHeight


    //in order to use local memory, the size has to be known at compile time or specified when creating the kernel.

    int location = get_global_id(0);
    int realWorldWidth = worldWidth-2, realWorldHeight = worldHeight-2;

    if(location > (realWorldWidth*realWorldHeight - 1))
        return;

    //int row = (location / worldWidth + 1)*(worldWidth+2);
    //int column = location % worldWidth + 1;

    int row = location / realWorldWidth + 1;
    int column = location % realWorldWidth + 1;

    int numberOfNeighbours = 0;
    int rowCoordinate0 = (row-1)*worldWidth; //row above
    int rowCoordinate1 = rowCoordinate0+worldWidth; //cell's row
    int rowCoordinate2 = rowCoordinate1+worldWidth; //row below

    int leftColumn = column-1, rightColumn = column+1;

    numberOfNeighbours += currentGeneration[rowCoordinate0 + leftColumn] + currentGeneration[rowCoordinate0 + column] + currentGeneration[rowCoordinate0 + rightColumn]
                          + currentGeneration[rowCoordinate1 + leftColumn] + currentGeneration[rowCoordinate1 + rightColumn]
                          + currentGeneration[rowCoordinate2 + leftColumn] + currentGeneration[rowCoordinate2 + column] + currentGeneration[rowCoordinate2 + rightColumn];



    /*if(currentGeneration[rowCoordinate + column-1] == true)
        numberOfNeighbours++;
    if(currentGeneration[rowCoordinate + column] == true)
        numberOfNeighbours++;
    if(currentGeneration[rowCoordinate + column+1] == true)
        numberOfNeighbours++;

    rowCoordinate += worldWidth;

    if(currentGeneration[rowCoordinate + column-1] == true)
        numberOfNeighbours++;
    if(currentGeneration[rowCoordinate + column+1] == true)
        numberOfNeighbours++;

    rowCoordinate += worldWidth;

    if(currentGeneration[rowCoordinate + column-1] == true)
        numberOfNeighbours++;
    if(currentGeneration[rowCoordinate + column] == true)
        numberOfNeighbours++;
    if(currentGeneration[rowCoordinate + column+1] == true)
        numberOfNeighbours++;*/


    //next step should be first stored in shared memory (inside workgroup) to avoid global memory contention.
    //only when the entire group is finished, the result should be moved to global memory.

    __global unsigned char* currentCell = &nextGeneration[row*worldWidth + column];

    if(numberOfNeighbours == 3)
        *currentCell = 1;
    else if((numberOfNeighbours == 2) && (currentGeneration[row*worldWidth + column] == 1))
        *currentCell = 1;
    else
        *currentCell = 0;
}
