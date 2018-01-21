/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include <fstream>

#include <QCoreApplication>
#include <QCursor>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <cstdio>
#include "../config.hpp"
#include "emuwindow.hpp"

using namespace std;

EmuWindow::EmuWindow(QWidget *parent) : QMainWindow(parent)
{

}

int EmuWindow::initialize()
{
    e = emuthread.get_emulator();
    if (emuthread.init())
        return 1;

    Config::test = false;

    ROM_file_name = "";

    load_ROM_act = new QAction(tr("&Load ROM..."), this);
    load_ROM_act->setShortcuts(QKeySequence::Open);
    connect(load_ROM_act, &QAction::triggered, this, &EmuWindow::load_ROM);

    load_GBA_ROM_act = new QAction(tr("&Load GBA ROM..."), this);
    connect(load_GBA_ROM_act, &QAction::triggered, this, &EmuWindow::load_GBA_ROM);

    screenshot_act = new QAction(tr("&Save Screenshot As..."), this);
    screenshot_act->setShortcut(Qt::Key_F2);
    connect(screenshot_act, &QAction::triggered, this, &EmuWindow::screenshot);

    file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->addAction(load_ROM_act);
    file_menu->addAction(load_GBA_ROM_act);
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
    if (menuBar()->isNativeMenuBar()) //Check for a global menubar, such as on OS X and Ubuntu
        menubar_height = 0;
    else
        menubar_height = menuBar()->height();
    resize(PIXELS_PER_LINE, SCANLINES * 2 + menubar_height);
    setMinimumSize(PIXELS_PER_LINE, SCANLINES * 2 + menubar_height);
    setWindowTitle("CorgiDS");
    show();

    spu_audio.set_SPU(e->get_SPU());
    spu_audio.open(QIODevice::ReadOnly);

    //Initialize audio
    //TODO: Is it better to place audio on the GUI thread or the emulation thread?
    QAudioFormat burp;
    burp.setSampleRate(32824);
    burp.setChannelCount(2); //stereo
    burp.setSampleSize(16);
    burp.setByteOrder(QAudioFormat::LittleEndian);
    burp.setSampleType(QAudioFormat::SignedInt);
    burp.setCodec("audio/pcm");

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(burp))
        printf("Audio initialization failed\n");
    else
        printf("Audio format supported\n");

    audio = new QAudioOutput(burp, nullptr);
    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handle_audio_state(QAudio::State)));
    //audio->start(&spu_audio);

    connect(this, SIGNAL(shutdown()), &emuthread, SLOT(shutdown()));
    connect(&emuthread, SIGNAL(finished_frame(uint32_t*,uint32_t*)), this, SLOT(draw_frame(uint32_t*,uint32_t*)));
    connect(&emuthread, SIGNAL(emulation_error(const char*)), this, SLOT(emulation_error(const char*)));
    connect(&emuthread, SIGNAL(update_FPS(int)), this, SLOT(update_FPS(int)));
    connect(this, SIGNAL(press_key(DS_KEYS)), &emuthread, SLOT(press_key(DS_KEYS)));
    connect(this, SIGNAL(release_key(DS_KEYS)), &emuthread, SLOT(release_key(DS_KEYS)));
    connect(this, SIGNAL(touchscreen_event(int,int)), &emuthread, SLOT(touchscreen_event(int,int)));
    emuthread.pause(PAUSE_EVENT::GAME_NOT_STARTED);
    emuthread.start();

    cfg = new ConfigWindow(0);

    return 0;
}

void EmuWindow::draw_frame(uint32_t* upper_buffer, uint32_t* lower_buffer)
{
    QImage upper((uint8_t*)upper_buffer, PIXELS_PER_LINE, SCANLINES, QImage::Format_RGB32);
    QImage lower((uint8_t*)lower_buffer, PIXELS_PER_LINE, SCANLINES, QImage::Format_RGB32);

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

    painter.drawPixmap(0, menubar_height, upper_pixmap);
    painter.drawPixmap(0, SCANLINES + menubar_height, lower_pixmap);
    event->accept();
}

void EmuWindow::emulation_error(const char *message)
{
    QString error(message);
    audio->stop();
    QMessageBox::critical(this, "Emulation Error", error);
    upper_pixmap = QPixmap();
    update();
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

    if (event->y() > SCANLINES + menubar_height)
    {
        QPoint coords = QCursor::pos();
        coords = this->mapFromGlobal(coords);
        emit touchscreen_event(coords.x(), coords.y() - (SCANLINES + menubar_height));
    }
}

void EmuWindow::mousePressEvent(QMouseEvent *event)
{
    event->accept();

    if (event->y() > SCANLINES + menubar_height)
    {
        QPoint coords = QCursor::pos();
        coords = this->mapFromGlobal(coords);
        emit touchscreen_event(coords.x(), coords.y() - (SCANLINES + menubar_height));
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
            if (audio->state() == QAudio::ActiveState)
                audio->stop();
            else
                audio->start(&spu_audio);
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
    audio->stop();
    emuthread.pause(PAUSE_EVENT::LOADING_ROM);
    if (e->load_firmware())
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
        spu_audio.clear_buffer();
        Config::frameskip = 0;
        Config::hle_bios = false;
        Config::enable_framelimiter = true;
        emuthread.unpause(PAUSE_EVENT::GAME_NOT_STARTED);
        emuthread.unpause(PAUSE_EVENT::LOADING_ROM);
    }
    audio->start(&spu_audio);
}

void EmuWindow::load_GBA_ROM()
{
    audio->stop();
    emuthread.pause(PAUSE_EVENT::LOADING_ROM);
    //Load BIOS
    ifstream bios_file(Config::gba_bios_path, ios::binary);
    if (!bios_file.is_open())
    {
        QMessageBox::critical(this, "Error", "Please load the GBA BIOS image before "
                              "loading a GBA ROM.");
        emuthread.unpause(PAUSE_EVENT::LOADING_ROM);
        return;
    }
    uint8_t* BIOS = new uint8_t[BIOS_GBA_SIZE];
    bios_file.read((char*)BIOS, BIOS_GBA_SIZE);
    bios_file.close();
    e->load_bios_gba(BIOS);
    delete[] BIOS;
    QString name = QFileDialog::getOpenFileName(this, tr("Load GBA ROM"), "", "GBA ROMs (*.gba)");
    if (name.length())
    {
        ifstream file(name.toStdString(), ios::binary | ios::ate);
        if (file.is_open())
        {
            uint64_t ROM_size = file.tellg();
            file.seekg(0, ios::beg);
            uint8_t* data = new uint8_t[ROM_size];
            file.read((char*)data, ROM_size);
            file.close();
            spu_audio.clear_buffer();
            Config::enable_framelimiter = true;
            Config::hle_bios = false;
            Config::frameskip = 0;
            emuthread.load_slot2(data, ROM_size);
            delete[] data;
            printf("Slot 2 successfully loaded.\n");

            if (Config::gba_direct_boot)
            {
                e->power_on();
                e->start_gba_mode(false);
                emuthread.unpause(GAME_NOT_STARTED);
            }
        }
        else
            QMessageBox::critical(this, "Error", "Unable to load GBA ROM");
    }
    audio->start(&spu_audio);
    emuthread.unpause(PAUSE_EVENT::LOADING_ROM);
}

void EmuWindow::about()
{
    QMessageBox::about(this, "CorgiDS v0.2", "Created by PSISP.");
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

void EmuWindow::handle_audio_state(QAudio::State state)
{
    /*switch (state)
    {
        case QAudio::ActiveState:
            printf("\nAudio started");
            break;
        case QAudio::IdleState:
            printf("\nAudio idle");
            break;
        case QAudio::StoppedState:
            printf("\nAudio stopped");
            break;
        case QAudio::SuspendedState:
            printf("\nAudio suspended");
            break;
        default:
            printf("\nSomething weird happened to audio");
            break;
    }
    if (audio->error() != QAudio::NoError)
    {
        printf("\nAn audio error occurred!");
        switch (audio->error())
        {
            case QAudio::OpenError:
                printf("\nOpen error!");
                break;
            case QAudio::IOError:
                printf("\nIO error!");
                break;
            case QAudio::UnderrunError:
                printf("\nUnderrun error!");
                break;
            case QAudio::FatalError:
                printf("\nFatal error!");
                break;
            default:
                printf("\nSome other error!");
                break;
        }
    }*/
}
