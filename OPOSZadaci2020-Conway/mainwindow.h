#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_jumpButton_clicked();

    void on_getPicture_clicked();

    void on_seedPicture_clicked();

    void on_startButton_clicked();

    void on_resetButton_clicked();

    void on_horizontalSlider_sliderMoved(int position);


    void on_horizontalSlider_2_sliderMoved(int position);

private:
    Ui::MainWindow *ui;
    std::string getPictureName();
    void startStateButtonStates();
    static const int maxWidth = 9, maxHeight = 14;
    int currentWidthSliderIndex = 0, currentHeightSliderIndex = 0;
    int widthValues[maxWidth] = {32, 40, 64, 80, 128, 160, 256, 320, 640};
    int heightValues[maxHeight] = {30, 36, 40, 45, 48, 60, 72, 80, 90, 120, 144, 180, 240, 360};
};
#endif // MAINWINDOW_H
