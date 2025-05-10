#ifndef _MAIN_WINDOW_HPP
#define _MAIN_WINDOW_HPP

#include <QMainWindow>

#include "video_speed_changer_widget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        VideoSpeedChangerWidget *mainWidget = new VideoSpeedChangerWidget(this);
        setCentralWidget(mainWidget);
    }
    ~MainWindow() {}

private:
};

#endif //_MAIN_WINDOW_HPP
