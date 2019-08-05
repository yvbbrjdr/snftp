#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QFileInfo>
#include <QMessageBox>

#include "crypto.h"

MainWidget::MainWidget(const QString &savePath, QTcpSocket *socket, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget),
    saveDir(savePath),
    socket(socket),
    curJobIndex(0),
    recvState(METADATA)
{
    ui->setupUi(this);
    socket->setParent(this);
    socket->setSocketOption(QTcpSocket::LowDelayOption, 1);

    ui->receiveListView->setModel(&recvStringListModel);
    ui->sendListView->setModel(&sendStringListModel);

    connect(socket, &QTcpSocket::readyRead, this, &MainWidget::socketReadyRead);
    connect(socket, &QTcpSocket::bytesWritten, this, &MainWidget::socketBytesWritten);
    connect(socket, &QTcpSocket::disconnected, this, &MainWidget::socketDisconnected);
}

MainWidget::~MainWidget()
{
    delete ui;
}

void MainWidget::socketWriteEncrypt(const QByteArray &data)
{
    QByteArray cipherText = Crypto::encrypt(data);
    const quint16 len = static_cast<quint16>(cipherText.length());
    QByteArray lenBytes(2, 0);
    lenBytes[0] = static_cast<char>(static_cast<unsigned char>(len >> 8));
    lenBytes[1] = static_cast<char>(static_cast<unsigned char>(len & 0xFF));
    cipherText = lenBytes + cipherText;
    socket->write(cipherText);
}

void MainWidget::updateSendListView()
{
    QStringList sendStringList;
    for (int i = 0; i < sendJobs.length(); ++i) {
        QString entry(sendJobs[i]->getFilename() + " - ");
        if (i < curJobIndex)
            entry += "Done";
        else if (i == curJobIndex)
            entry += "Sending";
        else
            entry += "Waiting";
        sendStringList.append(entry);
    }
    sendStringListModel.setStringList(sendStringList);
    ui->sendListView->scrollToBottom();
}

void MainWidget::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MainWidget::dropEvent(QDropEvent *e)
{
    const bool stopped = curJobIndex == sendJobs.length();

    foreach (const QUrl &url, e->mimeData()->urls()) {
        const QString filename(url.toLocalFile());
        const QFileInfo info(filename);
        if (!info.isFile()) {
            QMessageBox::warning(this, "Warning", QString("%1 is not a file!").arg(filename));
            continue;
        }
        if (!info.isReadable()) {
            QMessageBox::warning(this, "Warning", QString("%1 is not readable!").arg(filename));
            continue;
        }
        if (info.size() == 0) {
            QMessageBox::warning(this, "Warning", QString("%1 is an empty file!").arg(filename));
            continue;
        }
        sendJobs.append(new SendJob(filename));
    }

    updateSendListView();

    if (stopped)
        socketBytesWritten();
}

void MainWidget::socketReadyRead()
{
    recvBuffer += socket->readAll();

    for (;;) {
        const quint16 len = static_cast<quint16>((static_cast<unsigned char>(recvBuffer[0]) << 8) | static_cast<unsigned char>(recvBuffer[1]));
        if (recvBuffer.length() < len + 2)
            break;

        const QByteArray buf(Crypto::decrypt(recvBuffer.mid(2, len)));
        recvBuffer = recvBuffer.mid(len + 2);
        if (buf.length() == 0) {
            QMessageBox::critical(this, "Error", QString("The password seems to be incorrect!"));
            close();
        }

        if (recvState == METADATA) {
            const QByteArray len(buf.left(8));
            recvFileLen = 0;
            for (int i = 0; i < len.length(); ++i) {
                recvFileLen <<= 8;
                recvFileLen |= static_cast<unsigned char>(len[i]);
            }

            bytesRecved = 0;

            const QString filename(QString::fromUtf8(buf.mid(8)));
            const QString path(saveDir.absoluteFilePath(filename));
            recvFile.setFileName(path);
            if (!recvFile.open(QIODevice::WriteOnly)) {
                QMessageBox::critical(this, "Error", QString("Error opening file: %1").arg(path));
                close();
            }

            QStringList recvStringList(recvStringListModel.stringList());
            recvStringList.append(filename + " - Receiving");
            recvStringListModel.setStringList(recvStringList);
            ui->receiveListView->scrollToBottom();

            recvState = CONTENT;
        } else if (recvState == CONTENT) {
            recvFile.write(buf);

            bytesRecved += buf.length();
            ui->receiveProgressBar->setValue(static_cast<int>(bytesRecved * 100 / recvFileLen));
            if (recvFileLen == bytesRecved) {
                recvFile.close();

                QStringList recvStringList(recvStringListModel.stringList());
                QString back(recvStringList.back());
                back = back.left(back.length() - 9);
                back += "Done";
                recvStringList.pop_back();
                recvStringList.append(back);
                recvStringListModel.setStringList(recvStringList);
                ui->receiveListView->scrollToBottom();

                recvState = METADATA;
            }
        }
    }
}

void MainWidget::socketBytesWritten()
{
    if (socket->bytesToWrite() > 0 || curJobIndex == sendJobs.length())
        return;

    SendJob &curJob = *sendJobs[curJobIndex];
    if (!curJob.metadataSent) {
        qint64 fileSize = curJob.getFileSize();
        const QString filename(curJob.getFilename());
        QByteArray data(8, 0);
        for (int i = data.length() - 1; i >= 0; --i) {
            data[i] = static_cast<char>(static_cast<unsigned char>(fileSize & 0xFF));
            fileSize >>= 8;
        }
        data += filename.toUtf8();
        socketWriteEncrypt(data);
        curJob.metadataSent = true;

        return;
    }

    socketWriteEncrypt(curJob.read(64000));
    ui->sendProgressBar->setValue(static_cast<int>(curJob.getBytesRead() * 100 / curJob.getFileSize()));

    if (curJob.atEnd()) {
        ++curJobIndex;
        updateSendListView();
    }
}

void MainWidget::socketDisconnected()
{
    QMessageBox::information(this, "Error", QString("Socket disconnected!"));
    close();
}
