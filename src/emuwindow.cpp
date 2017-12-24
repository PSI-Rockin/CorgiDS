/*
    CorgiDS Copyright PSISP 2017
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <QCoreApplication>
#include <QCursor>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include "config.hpp"
#include "emuwindow.hpp"

EmuWindow::EmuWindow(QWidget *parent) : QMainWindow(parent)
{

}

int EmuWindow::initialize()
{
    if (emuthread.init())
        return 1;

    ROM_file_name = "";

    load_ROM_act = new QAction(tr("&Load ROM..."), this);
    load_ROM_act->setShortcuts(QKeySequence::Open);
    connect(load_ROM_act, &QAction::triggered, this, &EmuWindow::load_ROM);

    screenshot_act = new QAction(tr("&Save Screenshot As..."), this);
    screenshot_act->setShortcut(Qt::Key_F2);
    connect(screenshot_act, &QAction::triggered, this, &EmuWindow::screenshot);

    file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->addAction(load_ROM_act);
    file_menu->addAction(screenshot_act);

    config_act = new QAction(tr("&Config"), this);
    config_act->setShortcuts(QKeySequence::Preferences);
    connect(config_act, &QAction::triggered, this, &EmuWindow::preferences);

    emulation_menu = menuBar()->addMenu(tr("&Emulation"));
    emulation_menu->addAction(config_act);

    about_act = new QAction(tr("&About"), this);
    connect(about_act, &QAction::triggered, this, &EmuWindow::about);

    help_menu = menuBar()->addMenu(tr("&Help"));
    help_menu->addAction(about_act);

    menuBar()->show();
    resize(PIXELS_PER_LINE, SCANLINES * 2);
    setMinimumSize(PIXELS_PER_LINE, SCANLINES * 2);
    setWindowTitle("CorgiDS");
    show();

    connect(this, SIGNAL(shutdown()), &emuthread, SLOT(shutdown()));
    connect(&emuthread, SIGNAL(finished_frame(uint32_t*,uint32_t*)), this, SLOT(draw_frame(uint32_t*,uint32_t*)));
    connect(&emuthread, SIGNAL(update_FPS(int)), this, SLOT(update_FPS(int)));
    connect(this, SIGNAL(press_key(DS_KEYS)), &emuthread, SLOT(press_key(DS_KEYS)));
    connect(this, SIGNAL(release_key(DS_KEYS)), &emuthread, SLOT(release_key(DS_KEYS)));
    connect(this, SIGNAL(touchscreen_event(int,int)), &emuthread, SLOT(touchscreen_event(int,int)));
    emuthread.pause(PAUSE_EVENT::GAME_NOT_STARTED);
    emuthread.start();

    cfg = new ConfigWindow(0);
    /*upper_screen_label = new QLabel(this);
    upper_screen_label->show();
    upper_screen_label->resize(PIXELS_PER_LINE, SCANLINES);

    lower_screen_label = new QLabel(this);
    lower_screen_label->show();
    lower_screen_label->resize(PIXELS_PER_LINE, SCANLINES);
    lower_screen_label->move(0, SCANLINES);*/

    return 0;
}

void EmuWindow::draw_frame(uint32_t* upper_buffer, uint32_t* lower_buffer)
{
    /*for (int i = 0; i < 10; i++)
    {
        printf("\nColor data on (%d, 0): $%08X", i, upper_buffer[i]);
    }*/
    QImage upper((uint8_t*)upper_buffer, PIXELS_PER_LINE, SCANLINES, QImage::Format_RGB32);
    QImage lower((uint8_t*)lower_buffer, PIXELS_PER_LINE, SCANLINES, QImage::Format_RGB32);

    upper.setPixelColor(PIXELS_PER_LINE / 2, SCANLINES / 2, Qt::white);

    upper_pixmap = QPixmap::fromImage(upper);
    lower_pixmap = QPixmap::fromImage(lower);

    update();
}

void EmuWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (upper_pixmap.isNull() || lower_pixmap.isNull())
        return;

    painter.drawPixmap(0, 0, upper_pixmap);
    painter.drawPixmap(0, SCANLINES, lower_pixmap);
    event->accept();
}

void EmuWindow::update_FPS(int FPS)
{
    setWindowTitle(QString("CorgiDS - %1 FPS").arg(FPS));
}

void EmuWindow::closeEvent(QCloseEvent *event)
{
    emit shutdown();
    event->accept();
}

//TODO: add configurable option to pause emulation when out-of-focus
/*void EmuWindow::focusOutEvent(QFocusEvent *event)
{
    event->accept();
    out_of_focus = true;
}

void EmuWindow::focusInEvent(QFocusEvent *event)
{
    event->accept();
    out_of_focus = false;
}*/

void EmuWindow::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();

    if (event->y() > SCANLINES)
    {
        QPoint coords = QCursor::pos();
        coords = this->mapFromGlobal(coords);
        emit touchscreen_event(coords.x(), coords.y() - SCANLINES);
    }
}

void EmuWindow::mousePressEvent(QMouseEvent *event)
{
    event->accept();

    if (event->y() > SCANLINES)
    {
        QPoint coords = QCursor::pos();
        coords = this->mapFromGlobal(coords);
        emit touchscreen_event(coords.x(), coords.y() - SCANLINES);
    }
}

void EmuWindow::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    emit touchscreen_event(0, 0xFFF);
}

void EmuWindow::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    switch (event->key())
    {
        case Qt::Key_Tab:
            Config::enable_framelimiter = !Config::enable_framelimiter;
            break;
        case Qt::Key_O:
            Config::frameskip = !Config::frameskip;
            break;
        case Qt::Key_P:
            emuthread.manual_pause();
            break;
        case Qt::Key_Up:
            emit press_key(BUTTON_UP);
            break;
        case Qt::Key_Down:
            emit press_key(BUTTON_DOWN);
            break;
        case Qt::Key_Left:
            emit press_key(BUTTON_LEFT);
            break;
        case Qt::Key_Right:
            emit press_key(BUTTON_RIGHT);
            break;
        case Qt::Key_Q:
            emit press_key(BUTTON_L);
            break;
        case Qt::Key_W:
            emit press_key(BUTTON_R);
            break;
        case Qt::Key_A:
            emit press_key(BUTTON_Y);
            break;
        case Qt::Key_S:
            emit press_key(BUTTON_X);
            break;
        case Qt::Key_X:
            emit press_key(BUTTON_A);
            break;
        case Qt::Key_Z:
            emit press_key(BUTTON_B);
            break;
        case Qt::Key_Return:
            emit press_key(BUTTON_START);
            break;
        case Qt::Key_Space:
            emit press_key(BUTTON_SELECT);
            break;
        case Qt::Key_0:
            emit press_key(DEBUGGING);
            break;
    }
}

void EmuWindow::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
    switch (event->key())
    {
        case Qt::Key_Up:
            emit release_key(BUTTON_UP);
            break;
        case Qt::Key_Down:
            emit release_key(BUTTON_DOWN);
            break;
        case Qt::Key_Left:
            emit release_key(BUTTON_LEFT);
            break;
        case Qt::Key_Right:
            emit release_key(BUTTON_RIGHT);
            break;
        case Qt::Key_Q:
            emit release_key(BUTTON_L);
            break;
        case Qt::Key_W:
            emit release_key(BUTTON_R);
            break;
        case Qt::Key_A:
            emit release_key(BUTTON_Y);
            break;
        case Qt::Key_S:
            emit release_key(BUTTON_X);
            break;
        case Qt::Key_X:
            emit release_key(BUTTON_A);
            break;
        case Qt::Key_Z:
            emit release_key(BUTTON_B);
            break;
        case Qt::Key_Return:
            emit release_key(BUTTON_START);
            break;
        case Qt::Key_Space:
            emit release_key(BUTTON_SELECT);
            break;
    }
}

void EmuWindow::load_ROM()
{
    emuthread.pause(PAUSE_EVENT::LOADING_ROM);
    if (emuthread.load_firmware())
    {
        QMessageBox::critical(this, "Error", "Please load the BIOS and firmware images before "
                              "loading a game ROM.");
        emuthread.unpause(PAUSE_EVENT::LOADING_ROM);
        return;
    }
    emuthread.load_save_database();
    ROM_file_name = QFileDialog::getOpenFileName(this, tr("Load NDS ROM"), ROM_file_name, "NDS ROMs (*.nds)");
    if (ROM_file_name.length())
    {
        if (emuthread.load_game(ROM_file_name))
        {
            QMessageBox::critical(this, "Error", "Unable to load ROM");
            emuthread.unpause(PAUSE_EVENT::LOADING_ROM);
            return;
        }
        Config::frameskip = 0;
        Config::hle_bios = false;
        Config::enable_framelimiter = true;
        emuthread.unpause(PAUSE_EVENT::GAME_NOT_STARTED);
        emuthread.unpause(PAUSE_EVENT::LOADING_ROM);
        //e.debug();
    }
}

void EmuWindow::about()
{
    QMessageBox::about(this, "CorgiDS v0.1", "Created by PSISP.");
}

void EmuWindow::preferences()
{
    cfg->show();
}

void EmuWindow::screenshot()
{
    QString screenshot_path = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), "", "Images (*.png)");
    if (!screenshot_path.isEmpty())
    {
        QRect widget;
        //todo: modify based upon screen size
        widget.setCoords(0, 0, (PIXELS_PER_LINE) - 1, (SCANLINES * 2) - 1);
        QPixmap image(widget.size());
        render(&image, QPoint(), QRegion(widget));
        image.save(screenshot_path);
    }
}
