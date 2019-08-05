#include "startupdialog.h"
#include "ui_startupdialog.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>

#include "crypto.h"
#include "mainwidget.h"

StartupDialog::StartupDialog(QWidget *parent) : QDialog(parent), ui(new Ui::StartupDialog), server(nullptr), socket(nullptr)
{
    ui->setupUi(this);

    connect(ui->serverRadioButton, &QRadioButton::clicked, this, &StartupDialog::serverRadioButtonClicked);
    connect(ui->clientRadioButton, &QRadioButton::clicked, this, &StartupDialog::clientRadioButtonClicked);
    connect(ui->selectSavePathToolButton, &QToolButton::clicked, this, &StartupDialog::selectSavePathToolButtonClicked);
    connect(ui->launchPushButton, &QPushButton::clicked, this, &StartupDialog::launchPushButtonClicked);

    const QDir downloadsDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    ui->savePathLineEdit->setText(downloadsDir.absoluteFilePath("snftp"));
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
    ui->addressLineEdit->setText("");
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
    (new MainWidget(ui->savePathLineEdit->text(), socket))->show();
    close();
}
