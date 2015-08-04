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
  portlineEdit->setValidator(new QIntValidator(1024, 65535, this));
  portlineEdit->setToolTip("Port Number has to be between 1024 and 65535");
  port_label->setBuddy(portlineEdit);
  path_label = new QLabel("Path");
  pathlineEdit = new QLineEdit;
  path_label->setBuddy(pathlineEdit);
  pathlineEdit->setToolTip("Path with new directories will be created");
  rate_label = new QLabel("Rate");
  ratelineEdit = new QLineEdit;
  ratelineEdit->setMaxLength(9);
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

  QDir dir(pathtext);

  if (!dir.exists())
  {
    if (dir.mkpath(".") == false)
      QMessageBox::information(this, "Error", "Folder cant be created\n"
                               "Changes will not be applied");
  }
  else
  {
    QFileInfo finfo(pathtext);

    if (finfo.permission(QFile::WriteUser) == false &&
          !pathtext.isEmpty())
        QMessageBox::information(this, "Error", "Cant use this folder\n"
                                 "Changes will not be applied");
    else
    {
      QString filename = "server.ini";
      QString path;
      QFile file(filename);

      if (!path.isEmpty())
        path = dir.absolutePath();

      if (file.open(QIODevice::ReadWrite | QIODevice::Truncate))
      {
        QTextStream stream(&file);
        stream << "PORT = " << porttext << endl;
        stream << "PATH = " << path << endl;
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
  }
}

void ServerControl::enableApplyButton(const QString &text)
{
  if (portlineEdit->hasAcceptableInput() == TRUE || portlineEdit->text().isEmpty())
    applyButton->setEnabled(!text.isEmpty());
  else
    applyButton->setEnabled(false);

}

void ServerControl::clearClick()
{
  portlineEdit->clear();
  pathlineEdit->clear();
  ratelineEdit->clear();
}

