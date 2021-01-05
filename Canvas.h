#ifndef CANVAS_H
#define CANVAS_H
#include <QDebug>
#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <cmath>
#include <LifeCalculator.h>
#include <QtConcurrent/QtConcurrent>
#include <QMutex>
#include <QQueue>



/*LifeCalculator ce vrsiti kalkulacije i rezultate stavljati u svoj QQueue.
 *Koristice se promjenljiva koja ce osigurati da ne dodje do slucaja u kojem se rezultati proizvode brze nego sto se konzumiraju.
 *Canvas ce pozivati metodu u LifeCalculator kako bi preuzela stanje svijeta.
 *
 *Odredjivanje koordinata kod bitskog pakovanja:
 *      -bitovi se stavljaju u tip packedType (char, int, long, ...).U ovom slucaju ce se koristiti unsigned char.
 *      -koordinata grupe bitova koju trazimo:
 *          -rowLength = ceil(worldWidth/sizeof(packedType)) ; worldWidth ukljucuje nevidljive kolone
 *          -Y = (selectedScreenY+1)*rowLength
 *          -X = (selectedScreenX+1)%sizeof(packedType)
 *          -sada je koordinata unutar niza = X + Y
 *
 *      -koordinata celije unutar grupe:
 *          -packedType coord = sizeof(packedType)-1 - (X % sizeof(packedType))
 *          -ovo oznacava za koliko mjesta je potrebno izvrsiti shiftovanje udesno da bi se trazeni bit stavio na LSB poziciju
 *
 *      -kako evaluirati sve celije?Problem je sto se u jednom bajtu moze evaluirati samo 6 celija (prvi i zadnji bit se ne mogu evaluirati jer nemamo njihove susjede).
 *          -potrebna su 3 bajta (po svakom od 3 reda): lijevi bajt, sredisnji bajt (kojeg i posmatramo) i desni bajt - 2 bajta su "overhead"
 *          -Ucitati 4 bajta (int) i shiftovati ulijevo za 2 bajta.Ovo je zbog little endian reprezentacije.Sada imamo 2 bajta (na visoj polovini):
 *              -1. bajt (slijeva) je tu samo zbog jednog bita (njegov LSB je susjed MSB-a 2. bajta)
 *              -2. bajt (slijeva) je onaj kojeg i posmatramo.
 *
 * */

class Canvas : public QWidget
{
    /*For efficiency purposes, the 2D array should have 2 excess rows and columns just to avoid edge cases when checking neighbours of 1st row and column and last row and column
     *
     *
     *
     * */
    Q_OBJECT

public:
    Canvas(QWidget *parent = nullptr);
    void skipGenerations(int skipToGeneration);

private slots:
    void start();
    void stop();
    void sendNewGeneration(unsigned char* newWorld); //LifeCalculator class uses this slot to notify this (GUI) thread so it can update itself


private:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void determineClickedCell(int x, int y, int &resultX, int &resultY); //determines which cell was clicked on the screen
    void modifyCell(unsigned char cellValue, int selectedCellX, int selectedCellY); //value must be either 0 or 1.Coordinates represent screen coordinates.

    //data
    int selectedUpperLeftCellX=0, selectedUpperLeftCellY=0; //determines which cell is at the top left corner of the screen.Numbers start at 1 because the screen is surrounded by buffer zones (to avoid edge cases when checking neighbours).
    int selectedUpperGroupIndex = (selectedUpperLeftCellY+1)*rowLength+1 + selectedUpperLeftCellX/packedGroupSize; //index of the group that contains the upper left cell
    int selectedUpperCellBitPosition = (packedGroupSize - 1) - (selectedUpperLeftCellX%packedGroupSize); //Specifies the number of positions that have to be shifted to the right so we get the value of the upper left cell.

//selectedUpperCellBitPosition = (packedGroupSize - 1) - ((selectedUpperLeftCellY+1)*rowLength + (selectedUpperLeftCellX+1)/packedGroupSize + 1);
//(packedGroupSize-1) - ((selectedUpperLeftCellY+1)*rowLength)

    static const int CanvasWidth = 1280, CanvasHeight = 720;
    static const int numOfHorizontalCells = 160, numOfVerticalCells = 180; //dimension of the world
    static const int numOfRealHorizontalCells = numOfHorizontalCells+16, numOfRealVerticalCells = numOfVerticalCells+2; //there are 2 invisible columns and 2 invisible rows
    static const int zoomValue = 4;
    static const int packedGroupSize = sizeof(unsigned char)*8; //sizeof gives size in bytes, not bits...
    static const int rowLength = numOfRealHorizontalCells/packedGroupSize; //1 unsigned char should hold 8 cells.Ceil is not used because it is assumed that row is a multiple of 16.

    int cellWidth = CanvasWidth / numOfHorizontalCells, cellHeight = CanvasHeight / numOfVerticalCells;
    int offsetX, offsetY; //represent offsets because of the zoom implementation (the part of the cell that was zoomed at will still be under the mouse cursor after zooming).When all cells have the same dimension, offsets will be equal to cell dimension
    int numOfScreenHorizontalCells = numOfHorizontalCells, numOfScreenVerticalCells = numOfVerticalCells; //number of cells visible on the screen
    int clickedCellX, clickedCellY; //to avoid redrawing the screen, when dragging the mouse, if the cursor is still in the same cell
    long long currentGeneration=0;
    unsigned char* world;
    bool run = true, paused = true;
    LifeCalculator calculator;
    QMutex worldMutex; //synchronizes access to the world array.This is necessary because QWidget::update() can be called at any time
    QMutex queueMutex; //synchronizes access to worldQueue
    //QQueue<bool*> worldQueue;

    void updateCanvasElements();
    int rightCellCoordinateX = offsetX + (numOfScreenHorizontalCells-2)*cellWidth; //why is 2 subtracted?This might be incorrect for bit packing version.
    int downCellCoordinateY = offsetY + (numOfScreenVerticalCells-2)*cellHeight;
    int rightCellXIndex = selectedUpperLeftCellX + numOfScreenHorizontalCells-1;
    int rightCellYIndex = selectedUpperLeftCellY + numOfScreenVerticalCells - 1;


    void printState();
};


#endif // CANVAS_H
