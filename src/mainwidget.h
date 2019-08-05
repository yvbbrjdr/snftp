#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QDir>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QStringListModel>
#include <QTcpSocket>
#include <QVector>
#include <QWidget>

#include "sendjob.h"

namespace Ui {
    class MainWidget;
}

class MainWidget : public QWidget {
    Q_OBJECT
public:
    explicit MainWidget(const QString &savePath, QTcpSocket *socket, QWidget *parent = nullptr);
    ~MainWidget();
private:
    Ui::MainWidget *ui;
    QDir saveDir;
    QTcpSocket *socket;
    QVector<SendJob *> sendJobs;
    int curJobIndex;
    enum {
        METADATA,
        CONTENT
    } recvState;
    QByteArray recvBuffer;
    qint64 recvFileLen;
    qint64 bytesRecved;
    QFile recvFile;
    QStringListModel recvStringListModel;
    QStringListModel sendStringListModel;
    void socketWriteEncrypt(const QByteArray &data);
    void updateSendListView();
protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void socketReadyRead();
    void socketBytesWritten();
    void socketDisconnected();
};

#endif // MAINWIDGET_H
