/*
    CorgiDS Copyright PSISP 2017
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef EMUWINDOW_HPP
#define EMUWINDOW_HPP

#include <QCloseEvent>
#include <QFocusEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QLabel>
#include <QPaintEvent>
#include "config.hpp"
#include "configwindow.hpp"
#include "emulator.hpp"
#include "emuthread.hpp"

class EmuWindow : public QMainWindow
{
    Q_OBJECT
    private:
        EmuThread emuthread;

        ConfigWindow* cfg;

        QString ROM_file_name;
        QMenu* file_menu;
        QAction* load_ROM_act;
        QAction* screenshot_act;

        QMenu* emulation_menu;
        QAction* config_act;

        QMenu* help_menu;
        QAction* about_act;
        QPixmap upper_pixmap, lower_pixmap;
    public:
        explicit EmuWindow(QWidget *parent = nullptr);
        int initialize();
        void closeEvent(QCloseEvent* event);
        /*void focusOutEvent(QFocusEvent* event);
        void focusInEvent(QFocusEvent* event);*/

        bool is_running();
        bool is_emulating();
        bool finished_frame();

        void mouseMoveEvent(QMouseEvent* event);
        void mousePressEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);
        void paintEvent(QPaintEvent *event);
        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
    signals:
        void shutdown();
        void press_key(DS_KEYS key);
        void release_key(DS_KEYS key);
        void touchscreen_event(int x, int y);
    public slots:
        void draw_frame(uint32_t* upper_buffer, uint32_t* lower_buffer);
        void update_FPS(int FPS);
    private slots:
        void about();
        void load_ROM();
        void preferences();
        void screenshot();
};

#endif // EMUWINDOW_HPP
