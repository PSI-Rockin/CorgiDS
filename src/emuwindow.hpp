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
#include "config.hpp"
#include "configwindow.hpp"
#include "emulator.hpp"
#include "emuthread.hpp"

class EmuWindow : public QMainWindow
{
    Q_OBJECT
    private:
        EmuThread emuthread;
        QPixmap test2;

        ConfigWindow* cfg;

        QString ROM_file_name;
        QMenu* file_menu;
        QAction* load_ROM_act;
        QAction* screenshot_act;

        QMenu* emulation_menu;
        QAction* config_act;

        QMenu* help_menu;
        QAction* about_act;
        QLabel* upper_screen_label;
        QLabel* lower_screen_label;

        bool running;
        bool emulating;
        bool paused;
        bool out_of_focus;
        bool frame_finished;
        bool mouse_pressed;
    public:
        explicit EmuWindow(QWidget *parent = nullptr);
        int initialize();
        void closeEvent(QCloseEvent* event);
        /*void focusOutEvent(QFocusEvent* event);
        void focusInEvent(QFocusEvent* event);*/

        bool is_running();
        bool is_emulating();
        bool finished_frame();

        /*void mouseMoveEvent(QMouseEvent* event);
        void mousePressEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);*/
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

inline bool EmuWindow::is_running()
{
    return running;
}

inline bool EmuWindow::is_emulating()
{
    return emulating && !(Config::pause_when_unfocused && out_of_focus) && !paused;
}

inline bool EmuWindow::finished_frame()
{
    if (frame_finished)
    {
        frame_finished = false;
        return true;
    }
    return false;
}

#endif // EMUWINDOW_HPP
