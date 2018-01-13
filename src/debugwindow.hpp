/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef DEBUGWINDOW_HPP
#define DEBUGWINDOW_HPP

#include <QWidget>

namespace Ui {
class DebugWindow;
}

class DebugWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DebugWindow(QWidget *parent = 0);
    ~DebugWindow();

private:
    Ui::DebugWindow *ui;
};

#endif // DEBUGWINDOW_HPP
