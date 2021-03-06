#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    startStateButtonStates();
    ui->horizontalSlider->setMinimum(0);
    ui->horizontalSlider->setMaximum(maxWidth-1);
    ui->horizontalSlider_2->setMinimum(0);
    ui->horizontalSlider_2->setMaximum(maxHeight-1);

    ui->horizontalSlider->setTickInterval(1);
    ui->horizontalSlider_2->setTickInterval(1);

    //    connect(&calculator, &LifeCalculator::sendNewWorld, this, &Canvas::sendNewGeneration); //which connection type should be used (slot and signal are in different threads)?

    connect(ui->canvas, &Canvas::changeSliders, this, &MainWindow::setSliderValues);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setSliderValues(int width, int height)
{
    for(int i = 0; i < maxWidth; i++)
    {
        if(widthValues[i] == width)
        {
            currentWidthSliderIndex = i;
            ui->horizontalSlider->setValue(i);
            //ui->horizontalSlider->setSliderPosition(i);
            break;
        }
    }
    for(int i = 0; i < maxHeight; i++)
    {
        if(heightValues[i] == height)
        {
            currentHeightSliderIndex = i;
            ui->horizontalSlider_2->setValue(i);
            //ui->horizontalSlider_2->setSliderPosition(i);
            break;
        }
    }
}

void MainWindow::on_jumpButton_clicked()
{
    int generation = ui->lineEdit->text().toInt();
    qInfo() << "ui->canvas generation: " << generation;
    ui->canvas->skipGenerations(generation);
}

std::string MainWindow::getPictureName()
{
    std::string strPicName = ui->pictureName->text().toStdString();

    return strPicName;
}

void MainWindow::on_getPicture_clicked()
{
    ui->canvas->writeImage(getPictureName());
}

void MainWindow::on_seedPicture_clicked()
{
    ui->canvas->readImage(getPictureName());
}


void MainWindow::on_startButton_clicked()
{
    ui->stopButton->setDisabled(false);
    ui->jumpButton->setDisabled(false);
    ui->resetButton->setDisabled(false);

    ui->startButton->setDisabled(true);
    ui->seedPicture->setDisabled(true);

    ui->horizontalSlider->setDisabled(true);
    ui->horizontalSlider_2->setDisabled(true);

    ui->canvas->start();
}

void MainWindow::on_resetButton_clicked()
{
    ui->startButton->setDisabled(false);
    startStateButtonStates();
    ui->canvas->reset();
}

void MainWindow::startStateButtonStates()
{
    ui->horizontalSlider->setDisabled(false);
    ui->horizontalSlider_2->setDisabled(false);
    ui->stopButton->setDisabled(true);
    ui->jumpButton->setDisabled(true);
    ui->resetButton->setDisabled(true);
    ui->seedPicture->setDisabled(false);
}


void MainWindow::on_horizontalSlider_sliderMoved(int position)
{
    currentWidthSliderIndex = position;
    ui->canvas->changeDimensions(widthValues[currentWidthSliderIndex], heightValues[currentHeightSliderIndex]);
}

void MainWindow::on_horizontalSlider_2_sliderMoved(int position)
{
    currentHeightSliderIndex = position;
    ui->canvas->changeDimensions(widthValues[currentWidthSliderIndex], heightValues[currentHeightSliderIndex]);
}


void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    currentWidthSliderIndex = value;
    ui->canvas->changeDimensions(widthValues[currentWidthSliderIndex], heightValues[currentHeightSliderIndex]);
}


void MainWindow::on_horizontalSlider_2_valueChanged(int value)
{
    currentHeightSliderIndex = value;
    ui->canvas->changeDimensions(widthValues[currentWidthSliderIndex], heightValues[currentHeightSliderIndex]);
}



/*release
 * win32: LIBS += -L$$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Release/release/ -lConway_QT_library

INCLUDEPATH += $$PWD/../Conway_QT_library
DEPENDPATH += $$PWD/../Conway_QT_library

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Release/release/Conway_QT_library.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Release/release/libConway_QT_library.a*/

/*
debug
win32: LIBS += -L$$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Debug/debug/ -lConway_QT_library

INCLUDEPATH += $$PWD/../Conway_QT_library
DEPENDPATH += $$PWD/../Conway_QT_library

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Debug/debug/Conway_QT_library.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../build-Conway_QT_library-Desktop_Qt_6_0_0_MSVC2019_64bit-Debug/debug/libConway_QT_library.a*/








