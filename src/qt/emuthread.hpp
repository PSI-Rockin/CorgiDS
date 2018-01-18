/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef EMUTHREAD_HPP
#define EMUTHREAD_HPP
#include <QMutex>
#include <QThread>
#include "../emulator.hpp"

enum DS_KEYS
{
    BUTTON_A,
    BUTTON_B,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_L,
    BUTTON_R,
    BUTTON_START,
    BUTTON_SELECT,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,
    DEBUGGING
};

enum PAUSE_EVENT
{
    GAME_NOT_STARTED,
    LOADING_ROM,
    OUT_OF_FOCUS,
    MANUAL
};

class EmuThread : public QThread
{
    Q_OBJECT
    private:
        Emulator e;
        QMutex load_mutex, pause_mutex, key_mutex, screen_mutex;
        int pause_status;
        bool abort;
        uint32_t upper_buffer[PIXELS_PER_LINE * SCANLINES], lower_buffer[PIXELS_PER_LINE * SCANLINES];
    public:
        explicit EmuThread(QObject* parent = 0);
        int init();
        int load_firmware();
        void load_save_database();
        void load_slot2(uint8_t* data, uint64_t size);
        int load_game(QString ROM_file);

        Emulator* get_emulator();
    protected:
        void run() override;
    signals:
        void finished_frame(uint32_t* upper_buffer, uint32_t* lower_buffer);
        void emulation_error(const char* message);
        void update_FPS(int FPS);
    public slots:
        void shutdown();
        void manual_pause();
        void pause(PAUSE_EVENT event);
        void unpause(PAUSE_EVENT event);
        void press_key(DS_KEYS key);
        void release_key(DS_KEYS key);
        void touchscreen_event(int x, int y);
};

#endif // EMUTHREAD_HPP
