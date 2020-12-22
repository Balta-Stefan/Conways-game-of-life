#ifndef CANVAS_H
#define CANVAS_H
#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QImage>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <vector>
#include <QThread>


class Worker;


unsigned int isMandelbrotNumber(double real, double imaginary, unsigned short numberOfIterations);

//when forming a selection rectangle, redraw the widget (using QImage converted to pixmap) and draw rectangle over it
//multithreading: subclass QObject, create QThread object and move that QObject to QThread (movetothread method)
class Canvas : public QWidget
{
    friend class Worker;
    Q_OBJECT
private:
    //unsigned short threadsDone = 0;
    unsigned short numberOfColors;
    unsigned short defaultNumOfWorkers = 12;
    unsigned short canvasWidth, canvasHeight;
    void paintEvent(QPaintEvent *event) override;
    unsigned short numberOfWorkers;
    unsigned short numberOfIterations;
    unsigned int isMandelbrotNumber(double real, double imaginary);
    double upperLeftX, upperLeftY, downRightX, downRightY;
    double aspectRatio;
    void calculateCoords();
    double incrementX, incrementY;
    //unsigned short screen[768][1024];
    //multithreading...
    std::vector<Worker*> threads;
    std::vector<QThread*> threadVector;
    unsigned short rowsPerThread;
    //end of multithreading part
    QColor **screenColors;
    QImage image;
    bool leftClickHold;
    unsigned int clickedX, clickedY, releasedMovedX, releasedMovedY;
    void draw();
    void selectionRectangle(int clickedX, int clickedY, int movedX, int movedY);
    int selectionRectangleX, selectionRectangleY, selectionRectangleWidth, selectionRectangleHeight;
    void mouseReleaseCalculation();
    QColor *mapping;
    void calculatorFunction(unsigned short screenColorRowIndex);
    void makeThreads(unsigned short numberOfThreads);
    void createColorPalette();





public:
    Canvas(QWidget *parent = nullptr);
private slots:
    void changeNumOfIterations(int n);
    void changeNumOfWorkers(int n);
    void reset();
    //void computationDoneNotifier(unsigned short numOfRows, unsigned short rowForThread);
    void computationDoneNotifier();


protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
signals:
    void wakeThreads();
    void changeIterationSpinBox(int numOfIterations);
    void changeWorkersSpinbox(int workers);


};

class Worker : public QObject
{
    Q_OBJECT

public slots:
    void doWork();
signals:
    void resultReady();

public:
    Worker(Canvas* object, unsigned short rows, unsigned short vectorHeightIndex) : otherClass(object), numOfRows(rows), yCoordinate(vectorHeightIndex){}
    Canvas* otherClass;
    unsigned short numOfRows;
    unsigned short yCoordinate;


};

#endif // CANVAS_H
