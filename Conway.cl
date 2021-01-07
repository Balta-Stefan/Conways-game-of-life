
/*
 *Kernel kao argumente prima:
 *      -blok memorije trenutne generacije
 *      -blok memorije naredne generacije
 *      -dimenzije svijeta
 *
 *
 * */
//#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

void littleToBigEndian(unsigned int* toConvert)
{
    *toConvert = ((*toConvert) >> 24) | (((*toConvert) << 8) & 0x00ff0000) | (((*toConvert) >> 8) & 0x0000ff00) | ((*toConvert) << 24); //it's very important that unsigned int is used.Othervise, shifting to the right might lead to new ones.
}

void bigToLittleEndian(unsigned int* toConvert)
{
    *toConvert = ((*toConvert) >> 24) | (((*toConvert) >> 8) & 0x0000ff00) | (((*toConvert) << 8) & 0x00ff0000) | ((*toConvert) << 24);
}


/*World is represented as array of unsigned chars.One char holds 8 contiguous cells.The order of the cells on the bit level corresponds to the order on the screen.
 *Every instance of the kernel reads a 4 byte integer.
 *Only inner 2 bytes are evaluated (they hold 16 cells).
 *Bytes must follow the 2D screen coordinate convention.Since computers are little endian, endianess of the read integer must be changed.
 *Now, leftmost byte's LSB is neighbour of the 2nd leftmost byte's MSB.
 *Also, rightmost byte's MSB is neighbour of the 2nd rightmost byte's LSB.
 *After evaluating the inner 16 cells, they have to be converted to a short and endianess has to be swapped again.
 *That 2 byte short is written back to the nextGeneration array.
 *
 *Number of (visible) horizontal cells has to be a multiple of 16.This is because 4 byte integers are read.Any different dimensions would lead to wrong calculations.
 * */
__kernel void simulateLife(__global unsigned char* currentGeneration, __global unsigned char* nextGeneration, int numOfHorizontalGroups, int maxID)
{
    //if world width is a multiple of 16, total number of 4 byte integer reads per row will be (numOfHorizontalGroups-2)/2

    int ID = get_global_id(0);

    if(ID >= maxID)
        return;

    int iterationsPerRow = (numOfHorizontalGroups-2)/2;

    int row = ID / iterationsPerRow + 1; //row coordinate is index of the ghost byte in a certain row
    int column = (ID % iterationsPerRow)*2; //index of the beginning of our integer in the row

    int upperRowIndex = (row-1)*numOfHorizontalGroups + column;
    int currentRowIndex = row*numOfHorizontalGroups + column;
    int lowerRowIndex = (row+1)*numOfHorizontalGroups + column;

    __global unsigned char* bytePointer = currentGeneration;

    //unsigned ints are used because only zeros fill out the void when shifting
    //each variable below holds 4 contiguous rows (total 4*8=32) of cells
    unsigned int upperRows = *((__global unsigned int*)(bytePointer + upperRowIndex));
    unsigned int currentRows = *((__global unsigned int*)(bytePointer + currentRowIndex));
    unsigned int lowerRows = *((__global unsigned int*)(bytePointer + lowerRowIndex));


    //each of the variables below contains 32 cells.Only the middle 16, LSB of leftmost byte and MSB of rightmost byte are of interest.
    //First byte's LSB is the neighbour of the 2nd byte's MSB
    //Last byte's MSB is the neighbour of the 3rd byte's LSB


    littleToBigEndian(&upperRows);
    littleToBigEndian(&currentRows);
    littleToBigEndian(&lowerRows);


    //all game of life is done below

    //this will be the new state
    unsigned int newCentralRow = 0;

    //lower 7 bits aren't needed (neither are the upper 7 bits).

    //evaluate inner 16 cells
    for(int i = 0; i < 16; i++) //16 iterations because that's how many bits (cells) are evaluated in a 4 byte integer (only the middle 2 bytes are evaluated)
    {
        unsigned int neighbours = 0;

        //might be better to store the results in different variables if it could harm pipelining (if it exists on GPUs)
        //evaluate column on the right
        neighbours += (upperRows >> (7+i)) & 0x01;
        neighbours += (currentRows >> (7+i)) & 0x01;
        neighbours += (lowerRows >> (7+i)) & 0x01;

        //evaluate column that contains the cell that is being evaluated
        neighbours += (upperRows >> (7+i+1)) & 0x01;
        unsigned int alive = (currentRows >> (7+i+1)) & 0x01;
        neighbours += (lowerRows >> (7+i+1)) & 0x01;

        //evaluate the left column
        neighbours += (upperRows >> (7+i+2)) & 0x01;
        neighbours += (currentRows >> (7+i+2)) & 0x01;
        neighbours += (lowerRows >> (7+i+2)) & 0x01;


        unsigned int newCellState;

        if(neighbours == 3)
            newCellState = 1; //alive
        else if((neighbours == 2) && (alive == 1))
            newCellState = 1; //alive
        else
            newCellState = 0; //dead

        newCentralRow |= newCellState << (8+i);
    }


    //put the new central row into the nextGeneration array

    //put inner 2 bytes of the integer into a short.Writing a 4 byte integer would overwrite other thread's result.
    unsigned short newRow = (unsigned short)((newCentralRow & 0x00ffff00) >> 8);
    //swap endianness of newRow
    newRow = ((newRow & 0x00ff) << 8) | ((newRow & 0xff00) >> 8);

    //bigToLittleEndian(&newCentralRow);
    bytePointer = nextGeneration; //now write results to the next generation
    *((__global unsigned short*)(bytePointer + currentRowIndex+1)) = newRow;

}


