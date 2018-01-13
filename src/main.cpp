/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <QApplication>
#include "emuwindow.hpp"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    EmuWindow* window = new EmuWindow(); //Should this be defined on the heap? Who knows
    if (window->initialize())
        return 1;
    a.exec();
}
