#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <QDialog>
#include <QTcpServer>
#include <QTcpSocket>

namespace Ui {
    class StartupDialog;
}

class StartupDialog : public QDialog {
    Q_OBJECT
public:
    explicit StartupDialog(QWidget *parent = nullptr);
    ~StartupDialog();
private:
    Ui::StartupDialog *ui;
    QTcpServer *server;
    QTcpSocket *socket;
    void setUiEnabled(bool enabled);
    void startMainWidget();
private slots:
    void serverRadioButtonClicked();
    void clientRadioButtonClicked();
    void selectSavePathToolButtonClicked();
    void launchPushButtonClicked();
    void serverNewConnection();
    void socketConnected();
    void socketErrored();
};

#endif // STARTUPDIALOG_H
