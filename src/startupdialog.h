#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

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
    enum {
        DEFAULT_PORT = 7638
    };
    QTcpServer *server;
    QTcpSocket *socket;
    QUdpSocket *broadcastSocket;
    QStringListModel hostStringListModel;
    QList<QHostAddress> hostAddressList;
    void setUiEnabled(bool enabled);
    void startMainWidget();
    void sendHostname(const QHostAddress &addr);
private slots:
    void serverRadioButtonClicked();
    void clientRadioButtonClicked();
    void selectSavePathToolButtonClicked();
    void launchPushButtonClicked();
    void serverNewConnection();
    void socketConnected();
    void socketErrored();
    void refreshPushButtonClicked();
    void broadcastSocketReadyRead();
    void hostListViewClicked(const QModelIndex &index);
};

#endif // STARTUPDIALOG_H
