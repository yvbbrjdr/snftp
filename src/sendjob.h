#ifndef SENDJOB_H
#define SENDJOB_H

#include <QFile>

class SendJob {
public:
    SendJob(const QString &path);
    ~SendJob();
    QString getFilename();
    qint64 getFileSize();
    bool atEnd();
    QByteArray read(qint64 size);
    bool metadataSent;
private:
    qint64 bytesRead;
    QFile file;
};

#endif // SENDJOB_H
