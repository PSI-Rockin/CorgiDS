/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "debugwindow.hpp"
#include "ui_debugwindow.h"

DebugWindow::DebugWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DebugWindow)
{
    ui->setupUi(this);
}

DebugWindow::~DebugWindow()
{
    delete ui;
}
