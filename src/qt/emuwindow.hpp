/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef EMUWINDOW_HPP
#define EMUWINDOW_HPP

#include <QAudioFormat>
#include <QAudioOutput>
#include <QCloseEvent>
#include <QFocusEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QLabel>
#include <QPaintEvent>
#include "../config.hpp"
#include "audiodevice.hpp"
#include "configwindow.hpp"
#include "emuthread.hpp"

class EmuWindow : public QMainWindow
{
    Q_OBJECT
    private:
        Emulator* e;
        EmuThread emuthread;

        AudioDevice spu_audio;
        QAudioFormat format;
        QAudioOutput* audio;

        ConfigWindow* cfg;

        QString ROM_file_name;
        QMenu* file_menu;
        QAction* load_ROM_act;
        QAction* load_GBA_ROM_act;
        QAction* screenshot_act;

        QMenu* emulation_menu;
        QAction* config_act;

        QMenu* help_menu;
        QAction* about_act;
        QPixmap upper_pixmap, lower_pixmap;

        int menubar_height;
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
        void emulation_error(const char* message);
        void update_FPS(int FPS);
    private slots:
        void about();
        void load_ROM();
        void load_GBA_ROM();
        void preferences();
        void screenshot();
        void handle_audio_state(QAudio::State);
};

#endif // EMUWINDOW_HPP
