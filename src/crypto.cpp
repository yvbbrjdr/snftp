#include "crypto.h"

using namespace std;

bool Crypto::inited = false;
const QByteArray Crypto::salt("snftpsnftpsnftps");
QByteArray Crypto::key(crypto_aead_chacha20poly1305_IETF_KEYBYTES, 0);

void Crypto::init()
{
    if (inited)
        return;
    if (sodium_init() < 0)
        throw runtime_error("failed to initialize libsodium");
    inited = true;
}

void Crypto::setPassword(const QString &password)
{
    QByteArray passwordBytes(password.toUtf8());
    if (crypto_pwhash(reinterpret_cast<unsigned char *>(key.data()), static_cast<unsigned long long>(key.length()),
                      passwordBytes.data(), static_cast<unsigned long long>(passwordBytes.length()),
                      reinterpret_cast<const unsigned char *>(salt.data()),
                      crypto_pwhash_OPSLIMIT_INTERACTIVE,
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0)
        throw runtime_error("failed to generate password hash");
}

QByteArray Crypto::encrypt(const QByteArray &plainText)
{
    QByteArray nonce(crypto_aead_chacha20poly1305_IETF_NPUBBYTES, 0);
    randombytes_buf(nonce.data(), static_cast<size_t>(nonce.length()));
    QByteArray ret(plainText.length() + static_cast<int>(crypto_aead_chacha20poly1305_IETF_ABYTES), 0);
    unsigned long long cipherTextLen;
    crypto_aead_chacha20poly1305_ietf_encrypt(reinterpret_cast<unsigned char *>(ret.data()), &cipherTextLen,
                                              reinterpret_cast<const unsigned char *>(plainText.data()), static_cast<unsigned long long>(plainText.length()),
                                              nullptr, 0, nullptr, reinterpret_cast<unsigned char *>(nonce.data()),
                                              reinterpret_cast<unsigned char *>(key.data()));
    return nonce + ret.left(static_cast<int>(cipherTextLen));
}

QByteArray Crypto::decrypt(const QByteArray &cipherText)
{
    if (static_cast<unsigned int>(cipherText.length()) < crypto_aead_chacha20poly1305_IETF_NPUBBYTES)
        return QByteArray();
    QByteArray nonce(cipherText.left(crypto_aead_chacha20poly1305_IETF_NPUBBYTES));
    QByteArray cipher = cipherText.mid(crypto_aead_chacha20poly1305_IETF_NPUBBYTES);
    QByteArray ret(cipher.length(), 0);
    unsigned long long plainTextLen;
    if (crypto_aead_chacha20poly1305_ietf_decrypt(reinterpret_cast<unsigned char *>(ret.data()), &plainTextLen, nullptr,
                                                  reinterpret_cast<unsigned char *>(cipher.data()), static_cast<unsigned long long>(cipher.length()),
                                                  nullptr, 0, reinterpret_cast<unsigned char *>(nonce.data()),
                                                  reinterpret_cast<unsigned char *>(key.data())) != 0)
        return QByteArray();
    return ret.left(static_cast<int>(plainTextLen));
}
