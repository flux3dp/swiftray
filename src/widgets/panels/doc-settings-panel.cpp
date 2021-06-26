#include "doc-settings-panel.h"
#include "ui_doc-settings-panel.h"

DocumentSettingsPanel::DocumentSettingsPanel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::DocumentSettingsPanel)
{
    ui->setupUi(this);
}

DocumentSettingsPanel::~DocumentSettingsPanel()
{
    delete ui;
}
