
/*
 *Kernel kao argumente prima:
 *      -blok memorije trenutne generacije
 *      -blok memorije naredne generacije
 *      -dimenzije svijeta
 *
 *
 * */

__kernel simulateLife(__global bool* currentGeneration, __global bool* nextGeneration, int worldWidth, int worldHeight)
{
    //world dimensions will specify imaginary dimensions.They will account for 2 ghost rows and 2 ghost columns.
    //currentGeneration block will also contain ghost columns and rows.
    //total number of threads = realWorldWidth * realWorldHeight

    int location = get_global_id(0);
    //int row = (location / worldWidth + 1)*(worldWidth+2);
    //int column = location % worldWidth + 1;

    int row = location / (worldWidth-2) + 1;
    int column = location % (worldWidth-2) + 1;

    int numberOfNeighbours = 0;
    int rowCoordinate = (row-1)*worldWidth;

    if(currentGeneration[rowCoordinate + column-1] == true)
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
        numberOfNeighbours++;


    //next step should be first stored in shared memory (inside workgroup) to avoid global memory contention.
    //only when the entire group is finished, the result should be moved to global memory.

    __global bool* currentCell = &nextGeneration[row*worldWidth + column];

    if(neighbours == 3)
        *currentCell = true;
    else if((neighbours == 2) && (currentGeneration[row*worldWidth + column] == true))
        *currentCell = true;
    else
        *currentCell = true;
}
