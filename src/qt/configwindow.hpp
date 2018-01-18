/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#ifndef CONFIGWINDOW_HPP
#define CONFIGWINDOW_HPP

#include <QSettings>
#include <QWidget>

namespace Ui {
    class ConfigWindow;
}

class ConfigWindow : public QWidget
{
    Q_OBJECT

    public:
        explicit ConfigWindow(QWidget *parent = 0);
        ~ConfigWindow();

    private slots:
        void on_find_arm7_BIOS_clicked();

        void on_find_arm9_BIOS_clicked();

        void on_toggle_direct_boot_clicked(bool checked);

        void on_find_firmware_clicked();

        void on_find_savelist_clicked();

        void on_find_gba_BIOS_clicked();

        void on_toggle_gba_boot_clicked(bool checked);

private:
        QSettings cfg;
        Ui::ConfigWindow *ui;

        void update_ui();
};

#endif // CONFIGWINDOW_HPP
