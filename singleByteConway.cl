

/*Citati po 1 bajt.Sirina vise ne mora da bude djeljiva sa 16.
 *Svaki thread cita po 3 bajta: lijevi susjed (gleda se samo njegov LSB), bajt od interesa i desni bajt (gleda se samo njegov MSB)
 * 
 *Broj iteracija po redu je sada jednak broju vidljivih horizontalnih grupa (2x vece nego prije).
 *
 *
 *
 *Neka je akumulator unsigned int (4 bajta) jednak nuli.
 *Ucitati 1. bajt (lijevi, treba samo LSB), kastovati u unsigned int, shiftovati ga ulijevo za 16 mjesta i OR-ovati ga sa akumulatorom.
 *Ucitati 2. bajt (tu su celije), kastovati u unsigned int, shiftovati ga ulijevo za 1 mjesto i OR-ovati ga sa akumulatorom.
 *Ucitati 3. bajt, kastovati ga u unsigned int, shiftovati ga ulijevo za 9 mjesta i OR-ovati ga sa akumulatorom.
 *
 * */
 
__kernel void simulateLife(__global unsigned char* currentGeneration, __global unsigned char* nextGeneration, int numOfHorizontalGroups, int maxID)
{
    int ID = get_global_id(0);

    if(ID >= maxID)
        return;

    int iterationsPerRow = (numOfHorizontalGroups-2);

    int row = ID / iterationsPerRow + 1; //row coordinate is index of the ghost byte in a certain row
    int column = ID % iterationsPerRow; //index of the beginning of our integer in the row 
	
	//COLUMN MUST START FROM THE LEFT BYTE (whose LSB is left neighbour of the important central byte's MSB)!!!!

    int upperRowIndex = (row-1)*numOfHorizontalGroups;
    int currentRowIndex = row*numOfHorizontalGroups;
    int lowerRowIndex = (row+1)*numOfHorizontalGroups;

    __global unsigned char* bytePointer = currentGeneration;

    unsigned char upperRowLeftByte = *((__global unsigned char*)(bytePointer + upperRowIndex + column));
	unsigned char upperRowMiddleByte = *((__global unsigned char*)(bytePointer + upperRowIndex + column+1));
	unsigned char upperRowRightByte = *((__global unsigned char*)(bytePointer + upperRowIndex + column+2));
	
	unsigned char middleRowLeftByte = *((__global unsigned char*)(bytePointer + currentRowIndex + column));
	unsigned char middleRowMiddleByte = *((__global unsigned char*)(bytePointer + currentRowIndex + column+1));
	unsigned char middleRowRightByte = *((__global unsigned char*)(bytePointer + currentRowIndex + column+2));
	
	unsigned char lowerRowLeftByte = *((__global unsigned char*)(bytePointer + lowerRowIndex + column));
	unsigned char lowerRowMiddleByte = *((__global unsigned char*)(bytePointer + lowerRowIndex + column+1));
	unsigned char lowerRowRightByte = *((__global unsigned char*)(bytePointer + lowerRowIndex + column+2));
	
	
	unsigned int upperThreeRows = 0, middleThreeRows = 0, lowerThreeRows = 0;
	
	upperThreeRows |= (((unsigned int)upperRowLeftByte) << 9) | ((unsigned int)upperRowMiddleByte << 1) | ((unsigned int)upperRowRightByte >> 7);
	middleThreeRows |= (((unsigned int)middleRowLeftByte) << 9) | ((unsigned int)middleRowMiddleByte << 1) | ((unsigned int)middleRowRightByte >> 7);
	lowerThreeRows |= (((unsigned int)lowerRowLeftByte) << 9) | ((unsigned int)lowerRowMiddleByte << 1) | ((unsigned int)lowerRowRightByte >> 7);
	
	
	unsigned char newCentralRow = 0;
	
	
    //evaluate inner 8 cells
    for(int i = 0; i < 8; i++) //8 iterations because that's how many bits (cells) are evaluated in a 4 byte integer (only the middle 2 bytes are evaluated)
    {
        unsigned int neighbours = 0;

        //might be better to store the results in different variables if it could harm pipelining (if it exists on GPUs)
        //evaluate column on the right
        neighbours += (upperThreeRows >> i) & 0x01;
        neighbours += (middleThreeRows >> i) & 0x01;
        neighbours += (lowerThreeRows >> i) & 0x01;

        //evaluate column that contains the cell that is being evaluated
        neighbours += (upperThreeRows >> (i+1)) & 0x01;
        unsigned int alive = (middleThreeRows >> (i+1)) & 0x01;
        neighbours += (lowerThreeRows >> (i+1)) & 0x01;

        //evaluate the left column
        neighbours += (upperThreeRows >> (i+2)) & 0x01;
        neighbours += (middleThreeRows >> (i+2)) & 0x01;
        neighbours += (lowerThreeRows >> (i+2)) & 0x01;


        unsigned char newCellState;

        if(neighbours == 3)
            newCellState = 1; //alive
        else if((neighbours == 2) && (alive == 1))
            newCellState = 1; //alive
        else
            newCellState = 0; //dead

        newCentralRow |= newCellState << i;
    }
	

    bytePointer = nextGeneration; //now write results to the next generation
    *((__global unsigned char*)(bytePointer + currentRowIndex+column+1)) = newCentralRow;
}
