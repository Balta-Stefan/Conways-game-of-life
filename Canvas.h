#ifndef CANVAS_H
#define CANVAS_H
#include <QDebug>
#include <QWidget>
#include <QMouseEvent>
#include <QPainter>

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

private:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void determineClickedCell(int x, int y, int &resultX, int &resultY);





    //data
    int CanvasWidth = 1280, CanvasHeight = 720;
    int offsetX, offsetY; //represent offsets because of the zoom implementation (the part of the cell that was zoomed at will still be under the mouse cursor after zooming).When all cells have the same dimension, offsets will be equal to cell dimension
    int cellWidth, cellHeight;
    static const int numOfHorizontalCells = 40, numOfVerticalCells = 40; //dimension of the world
    int numOfScreenHorizontalCells = numOfHorizontalCells, numOfScreenVerticalCells = numOfVerticalCells; //number of cells visible on the screen
    bool world[numOfVerticalCells+2][numOfHorizontalCells+2];
    int clickedCellX, clickedCellY; //to avoid redrawing the screen, when dragging the mouse, if the cursor is still in the same cell
    int zoomValue = 4; //determines how many cells will appear or disappear when zooming.
    int selectedUpperLeftCellX=1, selectedUpperLeftCellY=1; //determines which cell is at the top left corner of the screen.Numbers start at 1 because the screen is surrounded by buffer zones (to avoid edge cases when checking neighbours).
    long long currentGeneration=0;

};


#endif // CANVAS_H
