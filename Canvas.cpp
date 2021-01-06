#include <Canvas.h>
#include <QThread>
#include <string>

Canvas::Canvas(QWidget *parent) : QWidget(parent)
{
    //connect(&renderingThread, &RenderThread::renderedImage, this, &MandelbrotWidget::updatePixmap);
    //connect(otherThreadAddress, otherThreadSignalAddress, receiverAddress, receiverSlotAddress)
    connect(&calculator, &LifeCalculator::sendNewWorld, this, &Canvas::sendNewGeneration); //which connection type should be used (slot and signal are in different threads)?

    offsetX = cellWidth;
    offsetY = cellHeight;
    calculator.init(rowLength, numOfRealVerticalCells);

    world = calculator.world;

    calculator.start();
    //QtConcurrent::run(this, &Canvas::play);
}

void Canvas::skipGenerations(int skipToGeneration)
{
    calculator.skipGenerations(skipToGeneration);
}

void Canvas::sendNewGeneration(unsigned char* newWorld)
{
    delete[] world;
    world = newWorld;

    //printState();
    update();
}

void Canvas::start()
{
    qInfo() << "i am started";
    calculator.startSimulation(world);
    //mutex.unlock();
}

void Canvas::stop()
{
    //mutex.lock();
    run = false;
    calculator.stop();
    qInfo() << "stopped!";
}


void Canvas::determineClickedCell(int x, int y, int &resultX, int &resultY)
{
    //Gives the screen coordinate of the clicked cell starting from 0.
    //Offset has to be subtracted because if it is included in the selected coordinate, calculation can be incorrect
    resultX = (x <= offsetX) ? 0 : ((x-offsetX)/cellWidth + 1);
    resultY = (y <= offsetY) ? 0 : ((y-offsetY)/cellHeight + 1);
}

void Canvas::modifyCell(unsigned char cellValue, int selectedCellX, int selectedCellY)
{
    //Method modifies the world too.
    //Arguments contain coordinates (starting from 0) of the clicked cell in the CUREENT VIEWPORT.They have to be translated into the global coordinates.
    //cellValue MUST be either 0 or 1

    int globalXCoordinate = selectedUpperLeftCellX + selectedCellX;
    int globalYCoordinate = selectedUpperLeftCellY + selectedCellY;

    int selectedGroupYCoordinate = (globalYCoordinate+1)*rowLength; //adding 1 because 1 row is invisible
    int selectedGroupXCoordinate = 1 + globalXCoordinate/packedGroupSize; //adding 1 because 1 byte is invisible (first column)
    unsigned char shiftPositions = (packedGroupSize - 1) - (globalXCoordinate % packedGroupSize); //specifies how many bit positions are between our bit (the cell) and the lowest significant bit within the group.

    unsigned char* selectedGroup = &world[selectedGroupYCoordinate+selectedGroupXCoordinate];
    //qInfo() << "shiftPositions=" << shiftPositions;
    //qInfo() << "cellValue before shifting: " << cellValue;
    cellValue <<= shiftPositions;
    //qInfo() << "cellValue after shifting: " << cellValue;
    (*selectedGroup) |= cellValue;
    //qInfo() << "selectedGroup now equals:" << (*selectedGroup);
    //qInfo() << "selecteduppercellbitposition=" << selectedUpperCellBitPosition;

    //THIS IS INCORRECT!THIS DOESNT MODIFY THE CORRECT WORLD COORDINATE, IT LOOKS ONLY AT CURRENT VIEWPORT COORDINATES, INSTEAD OF THE GLOBAL ONES.
    /*int selectedGroupY = (selectedCellY+1)*rowLength, selectedGroupX = (selectedCellX+1)%packedGroupSize;
    unsigned char* selectedGroup = &world[selectedGroupY+selectedGroupX];
    */
}

void Canvas::printState()
{
    qInfo() << "====================new state====================";
    unsigned char temp = 1;
    for(int j = 0; j < numOfVerticalCells; j++)
    {
        std::string row = "";

        for(int i = 0; i < numOfHorizontalCells/8; i++)
        {
            unsigned char group = world[(j+1)*rowLength+1+i];
            for(int k = 0; k < 8; k++)
            {
                unsigned char tmp = ((temp << (7-k)) & (group)) >> (7-k);
                if(tmp == 0)
                    row += "0,";
                else
                    row += "1,";
            }
        }
        std::cout << row << std::endl;
    }
    qInfo() << "=================================================";
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    qInfo() << "mouse pressed!" << "(" << event->x() / cellWidth << "," << event->y()/cellHeight << ")";

    if(event->button() == Qt::LeftButton)
    {
        //ako je offset jednak dimenziji celije, nista posebno
        //ako nije: od koordinate oduzeti offset i vidjeti koliko je celija, onda dodati 1
        determineClickedCell(event->x(), event->y(), clickedCellX, clickedCellY);

        qInfo() << "clickedCells:" << clickedCellX << "," << clickedCellY;
        qInfo() << "Cells to be activated:" << selectedUpperLeftCellX + clickedCellX << "," << selectedUpperLeftCellY + clickedCellY;
    }
    else
        return;


    modifyCell(1, clickedCellX, clickedCellY);
    /*int selectedGroupY = (clickedCellY+1)*rowLength, selectedGroupX = (clickedCellX+1)%packedGroupSize;
    unsigned char selectedGroupIndex = selectedGroupX + selectedGroupY;
    unsigned char* selectedGroup = &world[selectedGroupIndex];
    unsigned char numOfShifts = packedGroupSize - 1 - (selectedGroupX % packedGroupSize);
    unsigned char temp = 1;
    temp <<= numOfShifts;
    *selectedGroup |= temp;*/

    /*int row = (selectedUpperLeftCellY+clickedCellY)*numOfRealHorizontalCells;
    int column = selectedUpperLeftCellX+clickedCellX;
    world[row + column] = 1;*/
    //world[selectedUpperLeftCellY+clickedCellY][selectedUpperLeftCellX+clickedCellX] = true;

    //printState();
    update();
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    //this is called only when mouse is pressed (any button)
    //qInfo() << "mouse is being moved!" << event->x() << ", " << event->y();


    int newCellX, newCellY;
    determineClickedCell(event->x(), event->y(), newCellX, newCellY);

    if((newCellX == clickedCellX) && (newCellY == clickedCellY))
        return;
    if((newCellX > numOfScreenHorizontalCells) || (newCellY > numOfScreenVerticalCells) || (newCellX < 0) || (newCellY < 0)) //to avoid out of bounds array access
        return;
    qInfo() << "updated cell: " << newCellX << "," << newCellY;

    modifyCell(1, newCellX, newCellY);
    /*int row = (selectedUpperLeftCellY+newCellY)*numOfRealHorizontalCells;
    int column = selectedUpperLeftCellX+newCellX;
    world[row + column] = 1; //MIGHT BE INCORRECT*/

    //world[selectedUpperLeftCellY+newCellY][selectedUpperLeftCellX+newCellX] = true;
    //printState();
    update();

}

void Canvas::updateCanvasElements()
{
    rightCellCoordinateX = offsetX + (numOfScreenHorizontalCells-2)*cellWidth;
    downCellCoordinateY = offsetY + (numOfScreenVerticalCells-2)*cellHeight;
    rightCellXIndex = selectedUpperLeftCellX + numOfScreenHorizontalCells-1;
    rightCellYIndex = selectedUpperLeftCellY + numOfScreenVerticalCells - 1;
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    /*Zooming algorithm:
     *      -determine which cell is under the cursor
     *      -determine which part of the cell is under the cursor (for both coordinates)
     *      -determine where the selected part will be on the screen after zooming
     *      -determine the offset required for translation.Also modify the upper left cell.
     * */
    //ako canvasSize / numOfCells ne daje ostatak 0, uzima se ona dimenzija celije za koju ce ostatak biti 0.
        //


    //modify offsets and the number of vertical and horizontal cells (numOfScreenHorizontalCells and numOfScreenVerticalCells)
    //qInfo() << "mouse wheeled" << "size of canvas: " << this->width() << ", " << this->height();

    int isZooming = event->angleDelta().y() > 0 ? 1 : -1; //angleDelta().y() will be positive when zooming and negative when unzooming
    //if(isZooming > 0 && (numOfScreenHorizontalCells < 10 || numOfScreenVerticalCells < 10))
        //return; //dont zoom out if all of the cells are already on the screen and dont zoom in if there are less than 10 horizontal cells on the screen


    //the zooming will change the number of cells by zoomValue
    //numOfScreenHorizontalCells -= isZooming*zoomValue;
    //numOfScreenVerticalCells -= isZooming*zoomValue;


    /*if((isZooming < 0) && (numOfScreenHorizontalCells >= numOfHorizontalCells || numOfScreenVerticalCells >= numOfVerticalCells))
    {
        qInfo() << "______________activated reset";
        numOfScreenHorizontalCells = numOfHorizontalCells;
        numOfScreenVerticalCells = numOfVerticalCells;
        selectedUpperLeftCellX = selectedUpperLeftCellY = 1;
        cellWidth = offsetX = CanvasWidth / numOfHorizontalCells;
        cellHeight = offsetY = CanvasHeight / numOfVerticalCells;
        update();
        return;
    }*/
    //the part of the cell that is under the cursor should still be under the cursor after zooming

    //cellX and cellY specify the part of the cell that was selected while zooming, expressed in percentage.
    int cellX, cellY;
    int partX = (double)((event->x()-offsetX) % cellWidth)/cellWidth * 100.0, partY = (double)((event->y()-offsetY) % cellHeight)/cellHeight * 100.0;

    determineClickedCell(event->x(), event->y(), cellX, cellY);

    cellWidth += isZooming*zoomValue;
    cellHeight += isZooming*zoomValue;

    //qInfo() << "CanvasHeight / cellHeight=" << (double)CanvasHeight / cellHeight;
    qInfo() << "Selected cell (world):" << selectedUpperLeftCellX-1+cellX << "," << selectedUpperLeftCellY-1+cellY;
    int tempNumOfScreenHorizontalCells = std::ceil((double)CanvasWidth / cellWidth);
    if(tempNumOfScreenHorizontalCells <= packedGroupSize)
    {
        //this is to avoid complexity with drawing the first row
        cellWidth -= isZooming*zoomValue;
        cellHeight -= isZooming*zoomValue;
        return;
    }
    numOfScreenHorizontalCells = tempNumOfScreenHorizontalCells;
    numOfScreenVerticalCells = std::ceil((double)CanvasHeight / cellHeight);

    if((numOfScreenHorizontalCells >= numOfHorizontalCells) || (numOfScreenVerticalCells >= numOfVerticalCells))
    {
        numOfScreenHorizontalCells = numOfHorizontalCells;
        numOfScreenVerticalCells = numOfVerticalCells;

        cellWidth = offsetX = CanvasWidth / numOfHorizontalCells;
        cellHeight = offsetY = CanvasHeight / numOfVerticalCells;

        selectedUpperLeftCellX = selectedUpperLeftCellY = 0;
        selectedUpperGroupIndex = (selectedUpperLeftCellY+1)*rowLength+1 + selectedUpperLeftCellX/packedGroupSize; //index of the group that contains the upper left cell
        selectedUpperCellBitPosition = (packedGroupSize - 1) - (selectedUpperLeftCellX%packedGroupSize);

        updateCanvasElements();
        update();
        return;
    }



    //cellWidth = CanvasWidth / numOfScreenHorizontalCells;
    //cellHeight = CanvasHeight / numOfScreenVerticalCells;

    //determine where on the screen the selected part will be after zooming.No offsets will be accounted for here (all the cells will have the same size in this temporary screen).
    int newPartXCoordinate = cellWidth*cellX + (cellWidth*partX)/100;
    int newPartYCoordinate = cellHeight*cellY + (cellHeight*partY)/100;

    int screenOffsetX = newPartXCoordinate - event->x();
    int screenOffsetY = newPartYCoordinate - event->y();

    //if offsetX is positive, the cells need to be moved to the left.If offsetY is positive, cells need to be moved up.

    //offset could eliminate some cells entirely, check for such situation (next 2 values can be positive or negative)

    //number of whole cells per each axis that will be removed due to the offsets.
    int horizontalCellGain, verticalCellGain;
    qInfo() << "----------gains are:" << (double)screenOffsetX/cellWidth << "," << (double)screenOffsetY/cellHeight;

    //if the user is zooming in, he won't gain any new cells on the left side.If he is zooming out, new cells can appear on the left side.
    if(isZooming > 0)
    {
        horizontalCellGain = screenOffsetX / cellWidth;
        verticalCellGain = screenOffsetY / cellHeight;
    }
    else
    {
        //floor and ceil work the same for negative numbers - they both round the number to the next greater number.That's why there are 2 multiplications with negative 1 - to convert the number to positive, and then to revert it back to negative.
        horizontalCellGain = -1*std::ceil(-1*(double)screenOffsetX / cellWidth);
        verticalCellGain = -1*std::ceil(-1*(double)screenOffsetY / cellHeight);
    }
    qInfo() << "----------final gains:" << horizontalCellGain << "," << verticalCellGain;


    //if we are losing the part of our left cell, new offset will be equal to cellSize - lostPart
    //if we are gaining a part of a new cell (on the left), new offset will be equal to the offset.
    offsetX = (screenOffsetX > 0) ? (cellWidth-std::abs(screenOffsetX)%cellWidth) : (std::abs(screenOffsetX)%cellWidth);
    offsetY = (screenOffsetY > 0) ? (cellHeight-std::abs(screenOffsetY)%cellHeight) : (std::abs(screenOffsetY)%cellHeight);

    //if offset is == to 0, then all cells fit on the screen and the first line is at a coordinate equal to the cell(width/height)
    if(offsetX != 0)
        numOfScreenHorizontalCells++;
    else
        offsetX = cellWidth;
    if(offsetY != 0)
        numOfScreenVerticalCells++;
    else
        offsetY = cellHeight;
    //offsetX += (offsetX == 0) ? cellWidth : 0;
    //offsetY += (offsetY == 0) ? cellHeight : 0;


    //numOfScreenHorizontalCells += (offsetX != cellWidth) ? 1 : 0;
    //numOfScreenVerticalCells += (offsetY != cellHeight) ? 1 : 0;


    //shift the screen area

    int newUpperLeftCellX = selectedUpperLeftCellX+horizontalCellGain;
    int newUpperLeftCellY = selectedUpperLeftCellY+verticalCellGain;

    //check if we're trying to display more cells than we can (which will lead to out of bounds array access)
    qInfo() << "offsets have to be modified if we are going out of bounds!"; //the last cell on the right/down side should be the last one.It shouldn't disappear!
    if((newUpperLeftCellX + numOfScreenHorizontalCells - 1) > numOfHorizontalCells)
        newUpperLeftCellX -= (newUpperLeftCellX + numOfScreenHorizontalCells - 1 - numOfHorizontalCells);
    if((newUpperLeftCellY + numOfScreenVerticalCells - 1) > numOfVerticalCells)
        newUpperLeftCellY -= (newUpperLeftCellY + numOfScreenVerticalCells - 1 - numOfVerticalCells);

    /*0
     *1
     *2
     *3
     *4
     *5
     *6
     * */

    selectedUpperLeftCellX = std::max(0, newUpperLeftCellX);
    selectedUpperLeftCellY = std::max(0, newUpperLeftCellY);

    selectedUpperGroupIndex = (selectedUpperLeftCellY+1)*rowLength+1 + selectedUpperLeftCellX/packedGroupSize; //index of the group that contains the upper left cell
    selectedUpperCellBitPosition = (packedGroupSize - 1) - (selectedUpperLeftCellX%packedGroupSize);

    //ceil(cellWidth/(canvasWidth % cellWidth))
    //qInfo() << "PROBLEM SA NEISPRAVNIM RACUNANJEM BROJA VIDLJIVIH CELIJA JE ZBOG OSTATKA.AKO IMAMO OFFSET, TREBA VIDJETI KOLIKO CE SE GRESKE UVESTI ZA SVAKU LINIJU, TO MOZE BITI VISE OD JEDNE CELIJE, treba pogledati ostatak od canvasSize/cellSize, pomnoziti to sa brojem cijelih celija (ne offsetovanih), podijeliti to sa dimenzijom celije i uzeti ceil()";
    qInfo() << "PROBLEM: MOZE SE DESITI DA ZBOG SELECTEDUPPERCELL I NUMOFSCREENCELLS POKUSAVAM PRIKAZATI VISE CELIJA NEGO STO POSTOJI PA DOLAZI DO OUT OF BOUNDS PRISTUPA";
    qInfo() << "canvasHeight % cellHeight=" << CanvasHeight % cellHeight;
    qInfo() << "Selected coordinates: " << event->x() << "," << event->y();
    qInfo() << "Selected cell (screen): " << cellX << "," << cellY;
    qInfo() << "Selected parts: " << partX << "," << partY;
    qInfo() << "New positions of the part: " << newPartXCoordinate << "," << newPartYCoordinate;
    qInfo() << "Difference: " << screenOffsetX << "," << screenOffsetY;
    qInfo() << "Cell gain: " << horizontalCellGain << "," << verticalCellGain;
    qInfo() << "new offsets:" << offsetX << "," << offsetY;
    qInfo() << "New dimensions of the cell: " << cellWidth << "," << cellHeight;
    qInfo() << "selected upper cells (starting from 0,0):" << selectedUpperLeftCellX << "," << selectedUpperLeftCellY;
    qInfo() << "number of cells: " << numOfScreenHorizontalCells << "," << numOfScreenVerticalCells;
    qInfo() << "===========================================================================";

    updateCanvasElements();
    update();
}





//to do: rewrite the cell painting mechanism.It doesn't make sense to repeat all the calculations when only 1 read is required to get 8 cells (when char is used).

//selectedUpperGroupIndex = ((selectedUpperLeftY+1)*rowLength + (selectedUpperLeftX+1))/packedGroupSize
//selectedUpperCellBitPosition = ((selectedUpperLeftY+1)*rowLength + (selectedUpperLeftX+1)) % packedGroupSize

//get first group.Paint the upper left cell and use the rest for the rest of the row.When done with the first row, one bit will be left for the upper right cell.
//when every row is painted, first cell has to be handled specially because of its offset (first column).The rest shall be handled normally, only the last bit (cell)
    //is the bit of the last column which also has to be handled specially.

//last row has to be handled specially, just like the first row.


//first row:
/*
//upper left cell is drawn separately.That's why j is set to 1 for the loop.

//zooming method won't allow zooming so much that less than packedGroupSize cells can be displayed
//last column will be drawn just like every other.This might cause problems because rectangles are drawn outside the boundary.

unsigned char shifter = 1;
int j = 1; //used because upper left cell has to be skipped
for(int i = 0; i < numOfHorizontalScreenCells/8; i++) //iterate over groups within the first row.Rows are a multiple of 8.
{
      unsigned char currentGroup = world[selectedUpperGroupIndex+i];
      for(; j < packedGroupSize-1; j++) //iterate over each cell (bit) within the group from left to right (as usual).
      {
          if(((shifter << (packedGroupSize-1-j)) & currentGroup) == 1)
              paintCell; //first row

      }
      j=0;
}

//everything after the first row.This means that parts of the last row are drawn beyond the widget border.THIS MIGHT CAUSE PROBLEMS!!!

for(int j = 0; j < numOfScreenVerticalCells-1; j++) //Iterate over rows;
{
    int i = 1; //to skip the cell from the first column

    int selectedRow = (selectedUpperLeftCellY+1+j)*rowLength;
    unsigned char currentGroup = world[selectedRow+1]; //adding 1 because of the ghost column
    //draw cell from 1st column separately
    if(((shifter << (packedGroupSize-1)) & currentGroup) == 1)
        paintCell; //first column

    for(int k = 0; k < numOfHorizontalScreenCells/8; k++) //iterate over the row
    {
        currentGroup = world[selectedRow+1+k];
        for(; i < packedGroupSize-1; i++) //iterate over the cells within the selected group
        {
            if(((shifter << (packedGroupSize-1-i)) & currentGroup) == 1)
                paintCell; //normal cell
        }
        i=0;
    }


}




*/

void Canvas::paintEvent(QPaintEvent *)
{
    //qInfo() << "AM PAINTING NOW!";
    QPainter paint(this);
    QPen pen;
    QBrush brush;
    paint.setBrush(Qt::black);


    pen.setWidth(1);
    pen.setColor(Qt::black);
    paint.setPen(pen);


    //paint.fillRect(rectangle,Qt::red);

    //draw the grid.
    //first draw the outer grid lines
    paint.drawRect(QRect(0, 0, CanvasWidth, 0));
    paint.drawRect(QRect(0, 0, 0, CanvasHeight));
    paint.drawRect(QRect(0, CanvasHeight-1, CanvasWidth, 0));
    paint.drawRect(QRect(CanvasWidth-1, 0, 0, CanvasHeight));

    //debug, remove later
    //paint.drawRect(QRect(cellWidth*5 + cellWidth*0.45, 0, 1, CanvasHeight));
    //paint.drawRect(QRect(0, 3*cellHeight + 0.75*cellHeight, CanvasWidth, 1));
    for(int i = offsetX; i < CanvasWidth; i+=cellWidth) //draw vertical lines
        paint.drawRect(QRect(i, 0, 0, CanvasHeight));

    for(int i = offsetY; i < CanvasHeight; i+=cellHeight) //draw horizontal lines
        paint.drawRect(QRect(0, i, CanvasWidth, 0));

    //first row:


    //when right shifting unsigned numbers, zeros should be filled in
    unsigned char shifter = 1;
    unsigned char currentGroup = world[selectedUpperGroupIndex];
    //qInfo() << "====SELECTED UPPER GROUP INDEX=" << selectedUpperGroupIndex;
    //draw upper left cell
    if(((shifter << (selectedUpperCellBitPosition)) & currentGroup) != 0)
        paint.drawRect(QRect(0, 0, offsetX, offsetY));
    //draw the rest of the row, not including the upper right cell


    //upper left cell is drawn separately.That's why j is set to 1 for the loop.

    //zooming method won't allow zooming so much that less than packedGroupSize cells can be displayed
    //last column will be drawn just like every other.This might cause problems because rectangles are drawn outside the boundary.


    //try 2
    int counter = 0, indexCounter=0;
    int start = selectedUpperCellBitPosition-1, end = 0; //go from bit closest to MSB to LSB
    //1st row
    while(counter < (numOfScreenHorizontalCells-1))
    {
        for(int i = start; i >= end; i--)
        {
            if(((shifter << i) & (currentGroup)) != 0)
                paint.drawRect(QRect(offsetX + cellWidth*counter, 0, cellWidth, offsetY)); //first row
            counter++;
        }
        start = 7; //assuming 8 bit unsigned char is used
        end = std::max(0, 8-(numOfScreenHorizontalCells-counter));
        currentGroup = world[selectedUpperGroupIndex+(++indexCounter)];
    }

    //all other rows:
    for(int row = 1; row < numOfScreenVerticalCells; row++) //first row is skipped as it was drawn above
    {
        indexCounter = (selectedUpperLeftCellY+1+row)*rowLength+1 + selectedUpperLeftCellX/packedGroupSize;
        currentGroup = world[indexCounter];
        counter = 0;

        start = selectedUpperCellBitPosition-1;
        end = 0; //go from bit closest to MSB to LSB

        //first column has to be handled separately
        if(((shifter << selectedUpperCellBitPosition) & currentGroup) != 0)
            paint.drawRect(QRect(0, offsetY + cellHeight*(row-1), offsetX, cellHeight));

        while(counter < (numOfScreenHorizontalCells-1))
        {
            for(int i = start; i >= end; i--)
            {
                if(((shifter << i) & (currentGroup)) != 0)
                    paint.drawRect(QRect(offsetX + cellWidth*counter, offsetY + cellHeight*(row-1), cellWidth, cellHeight)); //first row
                counter++;
            }
            start = 7; //assuming 8 bit unsigned char is used
            end = std::max(0, 8-(numOfScreenHorizontalCells-counter));
            currentGroup = world[++indexCounter];
        }
    }


    //qInfo() << "AM DONE PAINTING!";

    //=========================================================================================
    //FATAL ERROR: I'M NOT CONSIDERING selectedUpperCellBitPosition ANYWHERE!!!THIS IS WRONG!!!
    //=========================================================================================

    //the number of iterations per row will be equal to numOfScreenHorizontalCells
    //first load the group that contains first element of the first row.



    //below is incorrect implementation that doesn't consider selectedUpperCellBitPosition
    /*int j = 1; //used because upper left cell has to be skipped
    for(int i = 0; i < numOfScreenHorizontalCells/8; i++) //iterate over groups within the first row.Rows are a multiple of 8.
    {
          unsigned char currentGroup = world[selectedUpperGroupIndex+i];
          for(; j < packedGroupSize; j++) //iterate over each cell (bit) within the group from left to right (as usual).
          {
              if(((shifter << (packedGroupSize-1-j)) & currentGroup) != 0)
                  paint.drawRect(QRect(offsetX + cellWidth*(packedGroupSize*i + (j-1)), 0, cellWidth, offsetY)); //first row

          }
          j=0;
    }


    qInfo() << "Offsets are: " << offsetX << "," << offsetY;
    //everything after the first row.This means that parts of the last row are drawn beyond the widget border.THIS MIGHT CAUSE PROBLEMS!!!
    for(int i = 1; i < numOfScreenVerticalCells; i++) //Iterate over rows;
        {
            j = 1; //to skip the cell from the first column

            int selectedRow = (selectedUpperLeftCellY+1+i)*rowLength;
            unsigned char currentGroup = world[selectedRow+1]; //adding 1 because of the ghost column
            //draw cell from 1st column separately
            if(((shifter << (packedGroupSize-1)) & currentGroup) != 0)
                paint.drawRect(QRect(0, offsetY + cellHeight*(i-1), offsetX, cellHeight)); //first column

            for(int k = 0; k < numOfScreenHorizontalCells/8; k++) //iterate over the row
            {
                currentGroup = world[selectedRow+1+k];
                for(; j < packedGroupSize; j++) //iterate over the cells within the selected group
                {
                    if(((shifter << (packedGroupSize-1-j)) & currentGroup) != 0)
                        paint.drawRect(QRect(offsetX + cellWidth*(k*packedGroupSize+j-1), offsetY + cellHeight*(i-1), cellWidth, cellHeight)); //normal cell
                }
                j=0; //all other MSBs don't have to be skipped because they aren't in the first column.
            }


        }*/












    //draw cells.The loop indexes account for the buffer columns and rows (whose purpose is to avoid out of bound access when checking neighbours)
    //first and last rows and columns have to be handled manually because of the offsets
    //corner cells have to be handled separately.In total, that's 8 manually handled drawings

    /*int rightCellCoordinateX = offsetX + (numOfScreenHorizontalCells-2)*cellWidth;
    int downCellCoordinateY = offsetY + (numOfScreenVerticalCells-2)*cellHeight;
    int rightCellXIndex = selectedUpperLeftCellX + numOfScreenHorizontalCells-1;
    int rightCellYIndex = selectedUpperLeftCellY + numOfScreenVerticalCells - 1;*/




    //============================EVERYTHING BELOW IS PRE BIT PACKING VERSION==============================

    /*//upper left cell
    //if(world[selectedUpperLeftCellY][selectedUpperLeftCellX] == true)
    if(world[selectedUpperLeftCellY*numOfRealHorizontalCells + selectedUpperLeftCellX] == 1)
        paint.drawRect(QRect(0, 0, offsetX, offsetY));

    //upper right cell
    //if(world[selectedUpperLeftCellY][rightCellXIndex] == true)
    if(world[selectedUpperLeftCellY*numOfRealHorizontalCells + rightCellXIndex] == 1)
        paint.drawRect(QRect(rightCellCoordinateX, 0, cellWidth, offsetY)); //cell width will go beyond screen area
    //down left cell
    //if(world[rightCellYIndex][selectedUpperLeftCellX] == true)
    if(world[rightCellYIndex*numOfRealHorizontalCells + selectedUpperLeftCellX] == 1)
        paint.drawRect(QRect(0, downCellCoordinateY, offsetX, cellHeight)); //cell height will go beyond screen area
    //down right cell
    //if(world[rightCellYIndex][rightCellXIndex] == true)
    if(world[rightCellYIndex*numOfRealHorizontalCells + rightCellXIndex] == 1)
        paint.drawRect(QRect(rightCellCoordinateX, downCellCoordinateY, cellWidth, cellHeight)); //cell dimensions will go beyond screen bounds

    //first row
    for(int i = 0; i < numOfScreenHorizontalCells-2; i++) //skipping 2 because they were handled as upper left and upper right corner cells
        //if(world[selectedUpperLeftCellY][selectedUpperLeftCellX+i+1] == true)
        if(world[selectedUpperLeftCellY*numOfRealHorizontalCells + selectedUpperLeftCellX+i+1] == 1)
            paint.drawRect(QRect(offsetX + cellWidth*i, 0, cellWidth, offsetY));
    //last row (also paints the last cell in the first row - upper right cell)
    for(int i = 0; i < numOfScreenHorizontalCells-2; i++)
        //if(world[rightCellYIndex][selectedUpperLeftCellX+i+1] == true)
        if(world[rightCellYIndex*numOfRealHorizontalCells + selectedUpperLeftCellX+i+1] == 1)
            paint.drawRect(QRect(offsetX + cellWidth*i, downCellCoordinateY, cellWidth, cellHeight)); //cell height will go beyond screen bounds
    //first column
    for(int i = 0; i < numOfScreenVerticalCells-2; i++)
        //if(world[selectedUpperLeftCellY+i+1][selectedUpperLeftCellX] == true)
        if(world[(selectedUpperLeftCellY+i+1)*numOfRealHorizontalCells + selectedUpperLeftCellX] == 1)
            paint.drawRect(QRect(0, offsetY + cellHeight*i, offsetX, cellHeight));

    //last column
    for(int i = 0; i < numOfScreenVerticalCells-2; i++)
        //if(world[selectedUpperLeftCellY+i+1][rightCellXIndex] == true)
        if(world[(selectedUpperLeftCellY+i+1)*numOfRealHorizontalCells + rightCellXIndex] == 1)
            paint.drawRect(QRect(rightCellCoordinateX, offsetY + cellHeight*i, cellWidth, cellHeight)); //cell width will go beyond screen bounds


    //everything else
    for(int j = 1; j < numOfScreenVerticalCells-1; j++)
        for(int i = 1; i < numOfScreenHorizontalCells-1; i++)
            //if(world[selectedUpperLeftCellY+j][selectedUpperLeftCellX+i] == true)
            if(world[(selectedUpperLeftCellY+j)*numOfRealHorizontalCells + selectedUpperLeftCellX+i] == 1)
                paint.drawRect(QRect(offsetX + cellWidth*(i-1), offsetY + cellHeight*(j-1), cellWidth, cellHeight));*/


    /*for(int j = 1; j <= numOfVerticalCells; j++)
        for(int i = 1; i <= numOfHorizontalCells; i++)
            if(world[j][i] == true)
                qInfo() << "cell:" << i-1 << "," << j-1 << "is activated";*/

}



/*
===========================================================================
Selected cell (world): 12 , 17
canvasHeight % cellHeight= 0
Selected coordinates:  397 , 315
Selected cell (screen):  10 , 12
Selected parts:  37 , 50
New positions of the part:  456 , 375
Difference:  59 , 60
Cell gain:  1 , 2
new offsets: 29 , 30
New dimensions of the cell:  44 , 30
selected upper cells (starting from 1,1): 4 , 8
number of cells:  31 , 25
===========================================================================
Selected cell (world): 12 , 17
canvasHeight % cellHeight= 18
Selected coordinates:  397 , 315
Selected cell (screen):  9 , 10
Selected parts:  36 , 50
New positions of the part:  374 , 273
Difference:  -23 , -42
Cell gain:  0 , -1
new offsets: 23 , 16
New dimensions of the cell:  40 , 26
selected upper cells (starting from 1,1): 4 , 7 -> PROBLEM, x se mora umanjiti za 1, zasto je ostalo isto?Zato se celija pomjerila ulijevo.
number of cells:  33 , 29
===========================================================================

-new cell should be at: x=374
*/
