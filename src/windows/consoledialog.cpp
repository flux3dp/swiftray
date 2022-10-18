#include "consoledialog.h"
#include "ui_consoledialog.h"

#include <QDebug>

ConsoleDialog::ConsoleDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ConsoleDialog)
{
  ui->setupUi(this);
  
  ui->consoleLog->document()->setMaximumBlockCount(100000);
}

ConsoleDialog::~ConsoleDialog()
{
  delete ui;
}


void ConsoleDialog::appendLogSent(QString msg) {
  if (msg.isEmpty()) {
    return;
  }

  // Special handling:
  if (msg.length() == 1 && msg[0] == '?' && ui->ignoreStatusCheckBox->isChecked()) {
    return;
  }
  if (msg.length() < 3 && !(msg[0].isPrint())) {
    // Handle special cmd (print in Hex value format)
    msg = QString::number(msg[0].unicode(), 16);
    msg.append('\n');
  }
  if (!msg.endsWith('\n')) {
    msg.append('\n');
  }

  ui->consoleLog->append("SND>" + msg);
}

void ConsoleDialog::appendLogRcvd(QString msg) {
  // Special handling:
  if (!msg.isEmpty() && msg.startsWith('<') && ui->ignoreStatusCheckBox->isChecked()) {
    return;
  }
  ui->consoleLog->insertPlainText(msg);

  qInfo() << "block cnt:" << ui->consoleLog->document()->blockCount();
}
