#ifndef CRYPTO_H
#define CRYPTO_H

#include <QString>

#include <sodium.h>

class Crypto {
public:
    static void init();
    static void setPassword(const QString &password);
    static QByteArray encrypt(const QByteArray &plainText);
    static QByteArray decrypt(const QByteArray &cipherText);
private:
    static bool inited;
    const static QByteArray salt;
    static QByteArray key;
};

#endif // CRYPTO_H
