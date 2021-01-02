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
 *
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
    void sendNewGeneration(bool* newWorld); //LifeCalculator class uses this slot to notify this (GUI) thread so it can update itself


private:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void determineClickedCell(int x, int y, int &resultX, int &resultY);


    //data
    static const int CanvasWidth = 1280, CanvasHeight = 720;
    int cellWidth = CanvasWidth / numOfHorizontalCells, cellHeight = CanvasHeight / numOfVerticalCells;
    int offsetX, offsetY; //represent offsets because of the zoom implementation (the part of the cell that was zoomed at will still be under the mouse cursor after zooming).When all cells have the same dimension, offsets will be equal to cell dimension
    static const int numOfHorizontalCells = 160, numOfVerticalCells = 240; //dimension of the world
    static const int numOfRealHorizontalCells = numOfHorizontalCells+2, numOfRealVerticalCells = numOfVerticalCells+2;
    int numOfScreenHorizontalCells = numOfHorizontalCells, numOfScreenVerticalCells = numOfVerticalCells; //number of cells visible on the screen
    int clickedCellX, clickedCellY; //to avoid redrawing the screen, when dragging the mouse, if the cursor is still in the same cell
    static const int zoomValue = 4;
    int selectedUpperLeftCellX=1, selectedUpperLeftCellY=1; //determines which cell is at the top left corner of the screen.Numbers start at 1 because the screen is surrounded by buffer zones (to avoid edge cases when checking neighbours).
    long long currentGeneration=0;
    bool *world;
    bool run = true, paused = true;
    LifeCalculator calculator;
    QMutex worldMutex; //synchronizes access to the world array.This is necessary because QWidget::update() can be called at any time
    QMutex queueMutex; //synchronizes access to worldQueue
    //QQueue<bool*> worldQueue;

    void updateCanvasElements();
    int rightCellCoordinateX = offsetX + (numOfScreenHorizontalCells-2)*cellWidth;
    int downCellCoordinateY = offsetY + (numOfScreenVerticalCells-2)*cellHeight;
    int rightCellXIndex = selectedUpperLeftCellX + numOfScreenHorizontalCells-1;
    int rightCellYIndex = selectedUpperLeftCellY + numOfScreenVerticalCells - 1;
};


#endif // CANVAS_H
