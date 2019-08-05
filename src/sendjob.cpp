#include "sendjob.h"

#include <QFileInfo>

using namespace std;

SendJob::SendJob(const QString &path) : metadataSent(false), bytesRead(0), file(path) {}

QString SendJob::getFilename()
{
    return QFileInfo(file).fileName();
}

qint64 SendJob::getFileSize()
{
    return file.size();
}

bool SendJob::atEnd()
{
    return bytesRead == file.size();
}

QByteArray SendJob::read(qint64 size)
{
    if (atEnd())
        return QByteArray();

    if (bytesRead == 0)
        if (!file.open(QIODevice::ReadOnly))
            throw runtime_error(QString("error opening file: %1").arg(file.fileName()).toLocal8Bit().data());
    QByteArray ret = file.read(size);
    bytesRead += ret.length();
    if (atEnd())
        file.close();
    return ret;
}
