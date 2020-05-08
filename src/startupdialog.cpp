#include "startupdialog.h"
#include "ui_startupdialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHostInfo>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QStandardPaths>

#include "crypto.h"
#include "mainwidget.h"

StartupDialog::StartupDialog(QWidget *parent) : QDialog(parent), ui(new Ui::StartupDialog), server(nullptr), socket(nullptr), broadcastSocket(new QUdpSocket(this))
{
    ui->setupUi(this);

    connect(ui->serverRadioButton, &QRadioButton::clicked, this, &StartupDialog::serverRadioButtonClicked);
    connect(ui->clientRadioButton, &QRadioButton::clicked, this, &StartupDialog::clientRadioButtonClicked);
    connect(ui->selectSavePathToolButton, &QToolButton::clicked, this, &StartupDialog::selectSavePathToolButtonClicked);
    connect(ui->launchPushButton, &QPushButton::clicked, this, &StartupDialog::launchPushButtonClicked);
    connect(ui->refreshPushButton, &QPushButton::clicked, this, &StartupDialog::refreshPushButtonClicked);
    connect(ui->hostListView, &QListView::clicked, this, &StartupDialog::hostListViewClicked);
    connect(ui->hostListView, &QListView::doubleClicked, this, &StartupDialog::launchPushButtonClicked);
    connect(broadcastSocket, &QUdpSocket::readyRead, this, &StartupDialog::broadcastSocketReadyRead);

    const QDir downloadsDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    ui->savePathLineEdit->setText(downloadsDir.absoluteFilePath("snftp"));
    ui->hostListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->hostListView->setModel(&hostStringListModel);
    broadcastSocket->bind(DEFAULT_PORT);
    refreshPushButtonClicked();
}

StartupDialog::~StartupDialog()
{
    delete ui;
}

void StartupDialog::setUiEnabled(bool enabled)
{
    ui->serverRadioButton->setEnabled(enabled);
    ui->clientRadioButton->setEnabled(enabled);
    ui->addressLineEdit->setEnabled(enabled);
    ui->portLineEdit->setEnabled(enabled);
    ui->hostListView->setEnabled(enabled);
    ui->refreshPushButton->setEnabled(enabled);
    ui->passwordLineEdit->setEnabled(enabled);
    ui->savePathLineEdit->setEnabled(enabled);
    ui->selectSavePathToolButton->setEnabled(enabled);
    ui->launchPushButton->setEnabled(enabled);
}

void StartupDialog::serverRadioButtonClicked()
{
    ui->addressLineEdit->setText("0.0.0.0");
}

void StartupDialog::clientRadioButtonClicked()
{
    ui->addressLineEdit->selectAll();
    ui->addressLineEdit->setFocus();
}

void StartupDialog::selectSavePathToolButtonClicked()
{
    const QString path(QFileDialog::getExistingDirectory(this, "Select Save Path", ui->savePathLineEdit->text()));
    if (path.length() > 0)
        ui->savePathLineEdit->setText(path);
}

void StartupDialog::launchPushButtonClicked()
{
    const bool isServer = ui->serverRadioButton->isChecked();

    const QString address(ui->addressLineEdit->text());
    if (address.length() == 0) {
        QMessageBox::critical(this, "Error", "Please enter an address!");
        ui->addressLineEdit->setFocus();
        return;
    }

    bool ok;
    const quint16 port = ui->portLineEdit->text().toUShort(&ok);
    if (!ok || port == 0) {
        QMessageBox::critical(this, "Error", "Port must range from 1 to 65535!");
        ui->portLineEdit->setFocus();
        return;
    }

    const QString savePath(ui->savePathLineEdit->text());
    if (savePath.length() == 0) {
        QMessageBox::critical(this, "Error", "Please enter a save path!");
        ui->savePathLineEdit->setFocus();
        return;
    }

    if (!QDir().mkpath(savePath)) {
        QMessageBox::critical(this, "Error", QString("Unable to create path: %1").arg(savePath));
        ui->savePathLineEdit->setFocus();
        return;
    }

    if (!QFileInfo(savePath).isWritable()) {
        QMessageBox::critical(this, "Error", QString("Path is not writable: %1").arg(savePath));
        ui->savePathLineEdit->setFocus();
        return;
    }

    if (isServer) {
        server = new QTcpServer(this);
        connect(server, &QTcpServer::newConnection, this, &StartupDialog::serverNewConnection);
        if (!server->listen(QHostAddress(address), port)) {
            server->deleteLater();
            server = nullptr;
            QMessageBox::critical(this, "Error", QString("Unable to listen on %1:%2").arg(address).arg(port));
            ui->addressLineEdit->setFocus();
            return;
        }
        ui->launchPushButton->setText("Listening...");
    } else {
        socket = new QTcpSocket(this);
        connect(socket, &QTcpSocket::connected, this, &StartupDialog::socketConnected);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this, &StartupDialog::socketErrored);
        socket->connectToHost(address, port);
        if (socket == nullptr)
            // Already failed.
            return;
        ui->launchPushButton->setText("Connecting...");
    }

    setUiEnabled(false);
}

void StartupDialog::serverNewConnection()
{
    socket = server->nextPendingConnection();
    socket->setParent(nullptr);

    server->close();
    server->deleteLater();
    server = nullptr;

    startMainWidget();
}

void StartupDialog::socketConnected()
{
    socket->setParent(nullptr);
    disconnect(socket, &QTcpSocket::connected, this, &StartupDialog::socketConnected);
    disconnect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
               this, &StartupDialog::socketErrored);

    startMainWidget();
}

void StartupDialog::socketErrored()
{
    const QString errorString(socket->errorString());

    socket->deleteLater();
    socket = nullptr;

    QMessageBox::critical(this, "Error", QString("Failed to connect: %1").arg(errorString));
    ui->launchPushButton->setText("Launch");
    setUiEnabled(true);
}

void StartupDialog::startMainWidget()
{
    Crypto::setPassword(ui->passwordLineEdit->text());
    MainWidget *mainWidget = new MainWidget(ui->savePathLineEdit->text(), socket);
    mainWidget->setAttribute(Qt::WA_DeleteOnClose);
    mainWidget->show();
    close();
}

void StartupDialog::refreshPushButtonClicked()
{
    broadcastSocket->writeDatagram(QByteArray(1, 0), QHostAddress::Broadcast, DEFAULT_PORT);
    hostStringListModel.setStringList(QStringList());
    hostAddressList.clear();
    sendHostname(QHostAddress::Broadcast);
}

void StartupDialog::broadcastSocketReadyRead()
{
    while (broadcastSocket->hasPendingDatagrams()) {
        QByteArray data;
        QHostAddress host;
        data.resize(broadcastSocket->pendingDatagramSize());
        broadcastSocket->readDatagram(data.data(), data.size(), &host);
        bool ok = true;
        foreach (QHostAddress other, QNetworkInterface::allAddresses()) {
            if (host.isEqual(other)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            if (data.size() == 1 && data.data()[0] == 0) {
                sendHostname(host);
            } else if (data.size() == 1 && data.data()[0] == 1) {
                int index = hostAddressList.indexOf(host);
                if (index != -1) {
                    QStringList hostStringList = hostStringListModel.stringList();
                    hostStringList.removeAt(index);
                    hostStringListModel.setStringList(hostStringList);
                    hostAddressList.removeAt(index);
                }
            } else if (hostAddressList.indexOf(host) == -1) {
                QString hostname(QString::fromUtf8(data));
                QStringList stringList(hostStringListModel.stringList());
                stringList.append(hostname + " (" + host.toString() + ')');
                hostStringListModel.setStringList(stringList);
                hostAddressList.append(host);
            }
        }
    }
}


void StartupDialog::hostListViewClicked(const QModelIndex &index)
{
    ui->clientRadioButton->click();
    ui->addressLineEdit->setText(hostAddressList.at(index.row()).toString());
}


void StartupDialog::sendHostname(const QHostAddress &addr)
{
    broadcastSocket->writeDatagram(QHostInfo::localHostName().toUtf8(), addr, DEFAULT_PORT);
}

void StartupDialog::sendOffline()
{
    broadcastSocket->writeDatagram(QByteArray(1, 1), QHostAddress::Broadcast, DEFAULT_PORT);
}

void StartupDialog::closeEvent(QCloseEvent *event)
{
    sendOffline();
    broadcastSocket->close();
    QDialog::closeEvent(event);
}
