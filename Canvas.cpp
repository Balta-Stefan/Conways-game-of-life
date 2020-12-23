#include <Canvas.h>

Canvas::Canvas(QWidget *parent) : QWidget(parent)
{
    cellWidth = offsetX = CanvasWidth / numOfHorizontalCells;
    cellHeight = offsetY = CanvasHeight / numOfVerticalCells;

    for(int i = 0; i < numOfVerticalCells+2; i++)
        for(int j = 0; j < numOfHorizontalCells+2; j++)
            world[i][j] = false;
}

void Canvas::determineClickedCell(int x, int y, int &resultX, int &resultY)
{
    resultX = (x <= offsetX) ? 0 : ((x-offsetX)/cellWidth + 1);
    resultY = (y <= offsetY) ? 0 : ((y-offsetY)/cellHeight + 1);
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
    world[selectedUpperLeftCellY+clickedCellY][selectedUpperLeftCellX+clickedCellX] = true;
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
    world[selectedUpperLeftCellY+newCellY][selectedUpperLeftCellX+newCellX] = true;
    update();

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

    numOfScreenHorizontalCells = CanvasWidth / cellWidth;
    numOfScreenVerticalCells = CanvasHeight / cellHeight;

    if((numOfScreenHorizontalCells >= numOfHorizontalCells) || (numOfScreenVerticalCells >= numOfVerticalCells))
    {
        numOfScreenHorizontalCells = numOfHorizontalCells;
        numOfScreenVerticalCells = numOfVerticalCells;

        cellWidth = offsetX = CanvasWidth / numOfHorizontalCells;
        cellHeight = offsetY = CanvasHeight / numOfVerticalCells;

        selectedUpperLeftCellX = selectedUpperLeftCellY = 1;
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
    int losingHorizontalCells = screenOffsetX / cellWidth;
    int losingVerticalCells = screenOffsetY / cellHeight;



    offsetX = (screenOffsetX > 0) ? (cellWidth-std::abs(screenOffsetX)%cellWidth) : (std::abs(screenOffsetX)%cellWidth);
    offsetY = (screenOffsetY > 0) ? (cellHeight-std::abs(screenOffsetY)%cellHeight) : (std::abs(screenOffsetY)%cellHeight);

    //if offset is == to 0, then all cells fit on the screen and the first line is at a coordinate equal to the cell(width/height)
    offsetX += (offsetX == 0) ? cellWidth : 0;
    offsetY += (offsetY == 0) ? cellHeight : 0;


    //numOfScreenHorizontalCells += (offsetX != cellWidth) ? 1 : 0;
    //numOfScreenVerticalCells += (offsetY != cellHeight) ? 1 : 0;


    //shift the screen area
    qInfo() << "The new upper cells SHOULD be:" << selectedUpperLeftCellX+losingHorizontalCells << "," << selectedUpperLeftCellY+losingVerticalCells;

    selectedUpperLeftCellX = std::max(1, selectedUpperLeftCellX+losingHorizontalCells);
    selectedUpperLeftCellY = std::max(1, selectedUpperLeftCellY+losingVerticalCells);

    qInfo() << "Selected cell (world):" << selectedUpperLeftCellX+cellX-1 << "," << selectedUpperLeftCellY+cellY-1;

    //ceil(cellWidth/(canvasWidth % cellWidth))
    qInfo() << "PROBLEM SA NEISPRAVNIM RACUNANJEM BROJA VIDLJIVIH CELIJA JE ZBOG OSTATKA.AKO IMAMO OFFSET, TREBA VIDJETI KOLIKO CE SE GRESKE UVESTI ZA SVAKU LINIJU, TO MOZE BITI VISE OD JEDNE CELIJE, treba pogledati ostatak od canvasSize/cellSize, podijeliti dimenziju celije sa tim i uzeti ceil()";
    qInfo() << "canvasHeight % cellHeight=" << CanvasHeight % cellHeight;
    qInfo() << "Selected coordinates: " << event->x() << "," << event->y();
    qInfo() << "Selected cell (screen): " << cellX << "," << cellY;
    qInfo() << "Selected parts: " << partX << "," << partY;
    qInfo() << "New positions of the part: " << newPartXCoordinate << "," << newPartYCoordinate;
    qInfo() << "Difference: " << screenOffsetX << "," << screenOffsetY;
    qInfo() << "Cell gain: " << losingHorizontalCells << "," << losingVerticalCells;
    qInfo() << "new offsets:" << offsetX << "," << offsetY;
    qInfo() << "New dimensions of the cell: " << cellWidth << "," << cellHeight;
    qInfo() << "selected upper cells (starting from 1,1):" << selectedUpperLeftCellX << "," << selectedUpperLeftCellY;
    qInfo() << "number of cells: " << numOfScreenHorizontalCells << "," << numOfScreenVerticalCells;
    qInfo() << "===========================================================================";

    update();
}






void Canvas::paintEvent(QPaintEvent *)
{
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


    //draw cells.The loop indexes account for the buffer columns and rows (whose purpose is to avoid out of bound access when checking neighbours)
    //first and last rows and columns have to be handled manually because of the offsets
    //corner cells have to be handled separately.In total, that's 8 manually handled drawings

    int rightCellCoordinateX = offsetX + (numOfScreenHorizontalCells-2)*cellWidth;
    int downCellCoordinateY = offsetY + (numOfScreenVerticalCells-2)*cellHeight;
    int rightCellXIndex = selectedUpperLeftCellX + numOfScreenHorizontalCells-1;
    int rightCellYIndex = selectedUpperLeftCellY + numOfScreenVerticalCells - 1;




    //upper left cell
    //if(world[selectedUpperLeftCellY][selectedUpperLeftCellX] == true)
        paint.drawRect(QRect(0, 0, offsetX, offsetY));

    //upper right cell
    //if(world[selectedUpperLeftCellY][rightCellXIndex] == true)
        paint.drawRect(QRect(rightCellCoordinateX, 0, cellWidth, offsetY)); //cell width will go beyond screen area
    //down left cell
    //if(world[rightCellYIndex][selectedUpperLeftCellX] == true) //not working
        paint.drawRect(QRect(0, downCellCoordinateY, offsetX, cellHeight)); //cell height will go beyond screen area
    //down right cell
    //if(world[rightCellYIndex][rightCellXIndex] == true) //not working
        paint.drawRect(QRect(rightCellCoordinateX, downCellCoordinateY, cellWidth, cellHeight)); //cell dimensions will go beyond screen bounds

    //first row
    for(int i = 0; i < numOfScreenHorizontalCells-2; i++) //skipping 2 because they were handled as upper left and upper right corner cells
        if(world[selectedUpperLeftCellY][selectedUpperLeftCellX+i+1] == true)
            paint.drawRect(QRect(offsetX + cellWidth*i, 0, cellWidth, offsetY));
    //last row (also paints the last cell in the first row - upper right cell)
    for(int i = 0; i < numOfScreenHorizontalCells-2; i++) //NOT WORKING AT ALL, see by commenting out the if line
        if(world[rightCellYIndex][selectedUpperLeftCellX+i+1] == true)
            paint.drawRect(QRect(offsetX + cellWidth*i, downCellCoordinateY, cellWidth, cellHeight)); //cell height will go beyond screen bounds
    //first column
    for(int i = 0; i < numOfScreenVerticalCells-2; i++)
        if(world[selectedUpperLeftCellY+i+1][selectedUpperLeftCellX] == true)
            paint.drawRect(QRect(0, offsetY + cellHeight*i, offsetX, cellHeight));

    //last column
    for(int i = 0; i < numOfScreenVerticalCells-2; i++) //ALSO NOT WORKING AT ALL
        if(world[selectedUpperLeftCellY+i+1][rightCellXIndex] == true)
            paint.drawRect(QRect(rightCellCoordinateX, offsetY + cellHeight*i, cellWidth, cellHeight)); //cell width will go beyond screen bounds


    //everything else
    for(int j = 1; j < numOfScreenVerticalCells-1; j++)
        for(int i = 1; i < numOfScreenHorizontalCells-1; i++)
            if(world[selectedUpperLeftCellY+j][selectedUpperLeftCellX+i] == true)
                paint.drawRect(QRect(offsetX + cellWidth*(i-1), offsetY + cellHeight*(j-1), cellWidth, cellHeight));



    /*for(int j = 1; j <= numOfVerticalCells; j++)
        for(int i = 1; i <= numOfHorizontalCells; i++)
            if(world[j][i] == true)
                qInfo() << "cell:" << i-1 << "," << j-1 << "is activated";*/

}



/*
===========================================================================
The new upper cells SHOULD be: 4 , 5
Selected cell (world): 5 , 7
Selected coordinates:  177 , 139
Selected cell (screen):  3 , 3
Selected parts:  33 , 35
New positions of the part:  283 , 170
Difference:  106 , 31
Cell gain:  1 , 0
new offsets: 64 , 20
New dimensions of the cell:  85 , 51
selected upper cells (starting from 1,1): 4 , 5
number of cells:  16 , 15
===========================================================================
The new upper cells SHOULD be: 4 , 5
Selected cell (world): 5 , 7
Selected coordinates:  177 , 139
Selected cell (screen):  2 , 3
Selected parts:  32 , 33
New positions of the part:  148 , 123
Difference:  -29 , -16
Cell gain:  0 , 0
new offsets: 35 , 21
New dimensions of the cell:  64 , 37
selected upper cells (starting from 1,1): 4 , 5
number of cells:  21 , 20
===========================================================================

*/
