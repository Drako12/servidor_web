#include <QtGui>
#include <QFile>
#include <QTextStream>
#include <signal.h>
#include "gui.h"


ServerControl::ServerControl(QWidget *parent)
  :QDialog(parent)
{
  port_label = new QLabel("Port");
  portlineEdit = new QLineEdit;
  port_label->setBuddy(portlineEdit);
  path_label = new QLabel("Path");
  pathlineEdit = new QLineEdit;
  path_label->setBuddy(pathlineEdit);
  rate_label = new QLabel("Rate");
  ratelineEdit = new QLineEdit;
  rate_label->setBuddy(ratelineEdit);

  applyButton = new QPushButton("Apply");
  applyButton->setDefault(true);
  applyButton->setEnabled(false);
  clearButton = new QPushButton("Clear");
  closeButton = new QPushButton("Close");

  connect(portlineEdit, SIGNAL(textChanged(const QString &)), this,
          SLOT(enableApplyButton(const QString &)));
  connect(pathlineEdit, SIGNAL(textChanged(const QString &)), this,
          SLOT(enableApplyButton(const QString &)));
  connect(ratelineEdit, SIGNAL(textChanged(const QString &)), this,
          SLOT(enableApplyButton(const QString &)));
  connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClick()));
  connect(clearButton, SIGNAL(clicked()), this, SLOT(clearClick()));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *portLayout = new QHBoxLayout;
  QHBoxLayout *pathLayout = new QHBoxLayout;
  QHBoxLayout *rateLayout = new QHBoxLayout;
  QVBoxLayout *leftside = new QVBoxLayout;
  QVBoxLayout *rightside = new QVBoxLayout;
  QHBoxLayout *mainLayout = new QHBoxLayout;

  portLayout->addWidget(port_label);
  portLayout->addWidget(portlineEdit);

  pathLayout->addWidget(path_label);
  pathLayout->addWidget(pathlineEdit);

  rateLayout->addWidget(rate_label);
  rateLayout->addWidget(ratelineEdit);

  rightside->addWidget(applyButton);
  rightside->addWidget(clearButton);
  rightside->addWidget(closeButton);

  leftside->addLayout(portLayout);
  leftside->addLayout(pathLayout);
  leftside->addLayout(rateLayout);

  mainLayout->addLayout(leftside);
  mainLayout->addLayout(rightside);
  setLayout(mainLayout);
  setWindowTitle("Server Control");
}

void ServerControl::applyClick()
{
  FILE *fp;
  char pid[100];

  QString porttext = portlineEdit->text();
  QString pathtext = pathlineEdit->text();
  QString ratetext = ratelineEdit->text();

  QString filename = "server.ini";
  QFile file(filename);
  if (file.open(QIODevice::ReadWrite | QIODevice::Truncate))
  {
    QTextStream stream(&file);
   // stream << "[SERVER_SETTINGS]" <<endl;
    stream << "PORT = " << porttext << endl;
    stream << "PATH = " << pathtext << endl;
    stream << "RATE = " << ratetext <<  endl;
  }

  file.close();

  if ((fp = fopen("server.pid", "r")) == NULL)
    QMessageBox::information(this, "Error", "Server pid file cant be found\n"
    "Maybe server has not been started?");
  else
  {
  kill(atoi(fgets(pid, 100, fp)), SIGHUP);
  fclose(fp);
  }
}

void ServerControl::enableApplyButton(const QString &text)
{
  applyButton->setEnabled(!text.isEmpty());
}

void ServerControl::clearClick()
{
  portlineEdit->clear();
  pathlineEdit->clear();
  ratelineEdit->clear();
}

