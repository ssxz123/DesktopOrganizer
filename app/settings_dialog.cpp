#include "settings_dialog.h"

#include "ui_settings_dialog.h"

SettingsDialog::SettingsDialog(const doc::Settings &settings, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->autoOrganizeCheck->setChecked(settings.autoOrganize);
    ui->debounceSpin->setValue(settings.debounceMs);
    ui->whitelistList->addItems(settings.whitelist);

    connect(ui->addButton, &QPushButton::clicked, this, &SettingsDialog::onAdd);
    connect(ui->removeButton, &QPushButton::clicked, this, &SettingsDialog::onRemove);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

doc::Settings SettingsDialog::settings() const
{
    doc::Settings result;
    result.autoOrganize = ui->autoOrganizeCheck->isChecked();
    result.debounceMs = ui->debounceSpin->value();
    for (int i = 0; i < ui->whitelistList->count(); ++i)
        result.whitelist.append(ui->whitelistList->item(i)->text());
    return result;
}

void SettingsDialog::onAdd()
{
    const QString name = ui->whitelistEdit->text().trimmed();
    if (name.isEmpty())
        return;
    const QList<QListWidgetItem *> existing = ui->whitelistList->findItems(
        name, Qt::MatchFixedString | Qt::MatchCaseSensitive);
    if (existing.isEmpty())
        ui->whitelistList->addItem(name);
    ui->whitelistEdit->clear();
    ui->whitelistEdit->setFocus();
}

void SettingsDialog::onRemove()
{
    const int row = ui->whitelistList->currentRow();
    if (row >= 0)
        delete ui->whitelistList->takeItem(row);
}
