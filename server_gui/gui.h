#ifndef GUI_H
#define GUI_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

class ServerControl : public QDialog
{

  Q_OBJECT

  public:
    ServerControl(QWidget *parent = 0);
  signals:
    void apply(const QString &str);
  private slots:
    void applyClick();
    void enableApplyButton(const QString &text);
  private:
    QLabel *port_label;
    QLabel *path_label;
    QLabel *rate_label;
    QLineEdit *portlineEdit;
    QLineEdit *pathlineEdit;
    QLineEdit *ratelineEdit;
    QPushButton *applyButton;
    QPushButton *clearButton;
    QPushButton *closeButton;
};

#endif
