#ifndef _MAIN_WINDOW_HPP
#define _MAIN_WINDOW_HPP

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr): QMainWindow(parent){
    }
    ~MainWindow(){}
private:
};

#endif //_MAIN_WINDOW_HPP
