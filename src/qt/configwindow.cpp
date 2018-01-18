/*
    CorgiDS Copyright PSISP 2017-2018
    Licensed under the GPLv3
    See LICENSE.txt for details
*/

#include "configwindow.hpp"
#include "ui_configwindow.h"
#include <QFileDialog>
#include "../config.hpp"

ConfigWindow::ConfigWindow(QWidget *parent) :
    QWidget(parent),
    cfg("PSI", "CorgiDS"),
    ui(new Ui::ConfigWindow)
{
    ui->setupUi(this);
    ui->arm7_BIOS_name->setReadOnly(true);
    ui->arm9_BIOS_name->setReadOnly(true);
    ui->firmware_name->setReadOnly(true);
    ui->savelist_name->setReadOnly(true);
    ui->gba_BIOS_name->setReadOnly(true);

    setWindowTitle("Emulator Configuration");

    Config::arm7_bios_path = cfg.value("boot/bios7path").toString().toStdString();
    Config::arm9_bios_path = cfg.value("boot/bios9path").toString().toStdString();
    Config::firmware_path = cfg.value("boot/firmwarepath").toString().toStdString();
    Config::savelist_path = cfg.value("saves/savelistpath").toString().toStdString();
    Config::gba_bios_path = cfg.value("boot/gbapath").toString().toStdString();

    Config::direct_boot_enabled = cfg.value("boot/directboot").toBool();
    ui->toggle_direct_boot->setChecked(Config::direct_boot_enabled);

    Config::gba_direct_boot = cfg.value("boot/gbadirectboot").toBool();
    ui->toggle_gba_boot->setChecked(Config::gba_direct_boot);

    Config::pause_when_unfocused = false;

    update_ui();
}

ConfigWindow::~ConfigWindow()
{
    delete ui;
}

void ConfigWindow::on_find_arm7_BIOS_clicked()
{
    QString BIOS_path = QFileDialog::getOpenFileName(this, tr("Load ARM7 BIOS"), "", "BIOS image (*.bin *.rom)");
    Config::arm7_bios_path = BIOS_path.toStdString();
    cfg.setValue("boot/bios7path", BIOS_path);
    update_ui();
}

void ConfigWindow::on_find_arm9_BIOS_clicked()
{
    QString BIOS_path = QFileDialog::getOpenFileName(this, tr("Load ARM9 BIOS"), "", "BIOS image (*.bin *.rom)");
    Config::arm9_bios_path = BIOS_path.toStdString();
    cfg.setValue("boot/bios9path", BIOS_path);
    update_ui();
}

void ConfigWindow::on_find_firmware_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Load NDS firmware"), "", "Firmware image (*.bin *.rom)");
    Config::firmware_path = path.toStdString();
    cfg.setValue("boot/firmwarepath", path);
    update_ui();
}

void ConfigWindow::on_toggle_direct_boot_clicked(bool checked)
{
    Config::direct_boot_enabled = checked;
    cfg.setValue("boot/directboot", checked);
}

void ConfigWindow::update_ui()
{
    QString arm7_path(Config::arm7_bios_path.c_str());
    ui->arm7_BIOS_name->setText(QFileInfo(arm7_path).fileName());

    QString arm9_path(Config::arm9_bios_path.c_str());
    ui->arm9_BIOS_name->setText(QFileInfo(arm9_path).fileName());

    QString firm_path(Config::firmware_path.c_str());
    ui->firmware_name->setText(QFileInfo(firm_path).fileName());

    QString save_path(Config::savelist_path.c_str());
    ui->savelist_name->setText(QFileInfo(save_path).fileName());

    QString gba_path(Config::gba_bios_path.c_str());
    ui->gba_BIOS_name->setText(QFileInfo(gba_path).fileName());

    ui->toggle_direct_boot->setChecked(Config::direct_boot_enabled);
    ui->toggle_gba_boot->setChecked(Config::gba_direct_boot);
}

void ConfigWindow::on_find_savelist_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Load savelist database"), "", "Savelist (*.bin)");
    Config::savelist_path = path.toStdString();
    cfg.setValue("saves/savelistpath", path);
    update_ui();
}

void ConfigWindow::on_find_gba_BIOS_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Load GBA BIOS"), "", "GBA BIOS (*.bin *.rom)");
    Config::gba_bios_path = path.toStdString();
    cfg.setValue("boot/gbapath", path);
    update_ui();
}

void ConfigWindow::on_toggle_gba_boot_clicked(bool checked)
{
    Config::gba_direct_boot = checked;
    cfg.setValue("boot/gbadirectboot", checked);
}
