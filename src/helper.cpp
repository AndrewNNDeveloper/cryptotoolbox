#include "helper.h"

#include "base58.h"
#include "libs/bitcoin/secp256k1/src/secp256k1.c"
#include "libs/bitcoin/crypto/ripemd160.h"
#include "libs/bitcoin/crypto/sha256.h"

std::string helper::convertQStringToStdString(const QString &str)
{
    return str.toLocal8Bit().constData();
}

QString helper::convertStdStringToQString(const std::string &str)
{
    return QTextCodec::codecForName("UTF8")->toUnicode(QByteArray(str.c_str()));
}

QString helper::encodeBase58(const QByteArray   &Data)
{
    return QT_STRING(EncodeBase58(
        reinterpret_cast<const unsigned char *>(Data.data()),
        reinterpret_cast<const unsigned char *>(Data.data() + Data.size())));
};

QString helper::encodeBase58(const QString &str)
{
    QByteArray hexBeforeBase58 = QByteArray::fromHex(str.toUtf8().data());

    return QT_STRING(EncodeBase58(
        reinterpret_cast<const unsigned char *>(hexBeforeBase58.data()),
        reinterpret_cast<const unsigned char *>(hexBeforeBase58.data() + hexBeforeBase58.size())));

    /*
    std::vector<unsigned char> beforeBase58Vector;
    for (int i = 0; i < hexBeforeBase58.size(); i++) {
        beforeBase58Vector.push_back(hexBeforeBase58.at(i));
    }
    std::string afterBase58Std = EncodeBase58(beforeBase58Vector);
    return QT_STRING(afterBase58Std);
    */
}

QString helper::decodeBase58(const QString &str)
{
    QByteArray WIFArray = str.toUtf8();
    char *buffer = WIFArray.data();

    std::vector<unsigned char> vchVector;
    if (!DecodeBase58(buffer, vchVector))
        return "";

    static const unsigned char  hexAlphas[] = "0123456789ABCDEF";

    QString Result = "";
    for (auto it : vchVector) {
        Result += hexAlphas[(it & 0xF0) >> 4];
        Result += hexAlphas[it & 0xF];
    }

    return Result;
}

QByteArray helper::getQtHexHashSha256(const QByteArray &ba)
{
    return QCryptographicHash::hash(ba, QCryptographicHash::Sha256).toHex();
}

QString helper::getQtHexHashSha256(const QString &str)
{
    return QString(getQtHexHashSha256(str.toUtf8()));
}

QString helper::getQtHexHashSha256FromHexString(const QString &str)
{
    return QString(helper::getQtHexHashSha256(QByteArray::fromHex(str.toUtf8().data())));
}

QString helper::getQtHashSha256(const QString &str)
{
    return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Sha256).toHex();
}

QByteArray helper::CalcHash(
        const   void                    *Data,
        const   size_t                  DataSize,
        const   CDigest::DIGEST_TYPE    DigestType)
{
    CDigest *Digest = CDigest::CreateDigest(DigestType);

    if (Digest == nullptr)
    {
        return QByteArray();
    }

    std::vector<unsigned char>   DigestBuffer;
    Digest->Update(Data, DataSize);
    Digest->Finish(DigestBuffer);

    return QByteArray(
                reinterpret_cast<const char *>(DigestBuffer.data()),
                DigestBuffer.size());
};

QByteArray helper::CalcHash(
        const   QByteArray              &Data,
        const   CDigest::DIGEST_TYPE    DigestType)
{
    return CalcHash(Data.data(), Data.size(), DigestType);
};

QByteArray helper::CalcHashN(
        const   void                                *Data,
        const   size_t                              DataSize,
        const   std::vector<CDigest::DIGEST_TYPE>   &DigestTypes)
{
    if ((Data == nullptr) ||
        (DataSize == 0) ||
        (DigestTypes.size() == 0))
    {
        return QByteArray();
    }

    QByteArray  CurrentHash;
    for (size_t k = 0; k < DigestTypes.size(); k++)
    {
        if (k == 0)
        {
            CurrentHash = CalcHash(Data, DataSize, DigestTypes[k]);
        }
        else
        {
            CurrentHash = CalcHash(CurrentHash, DigestTypes[k]);
        }
    };

    return CurrentHash;
};

QByteArray helper::CalcHashN(
        const   QByteArray                          &Data,
        const   std::vector<CDigest::DIGEST_TYPE>   &DigestTypes)
{
    return CalcHashN(Data.data(), Data.size(), DigestTypes);
};

QByteArray helper::encodeRipemd160(const QByteArray &ba)
{
    unsigned char hash[CRIPEMD160::OUTPUT_SIZE];
    CRIPEMD160().Write(reinterpret_cast<const unsigned char *>(ba.data()), ba.size()).Finalize(hash);
    return QByteArray(reinterpret_cast<const char*>(hash), CRIPEMD160::OUTPUT_SIZE);
}

QString helper::getHexHashRipemd160FromHexString(const QString &str)
{
    return encodeRipemd160(QByteArray::fromHex(str.toUtf8().data())).toHex();
}

QString helper::getHexHashRipemd160FromString(const QString &str)
{
    return encodeRipemd160(str.toUtf8().data()).toHex();
}

QByteArray helper::encodeSha256(const QByteArray &ba)
{
    unsigned char hash[CSHA256::OUTPUT_SIZE];
    CSHA256().Write(reinterpret_cast<const unsigned char *>(ba.data()), ba.size()).Finalize(hash);
    return QByteArray(reinterpret_cast<const char*>(hash), CSHA256::OUTPUT_SIZE);
}

QString helper::getHexHashSha256FromHexString(const QString &str)
{
    return encodeSha256(QByteArray::fromHex(str.toUtf8().data())).toHex();
}

QString helper::getHexHashSha256FromString(const QString &str)
{
    return encodeSha256(str.toUtf8().data()).toHex();
}

QString helper::getPublicECDSAKey(
    const   QString &privKeyQString,
            bool    compressedFlag)
{
    QByteArray                  ba = QByteArray::fromHex(privKeyQString.toUtf8().data());
    const unsigned char         *seckey = reinterpret_cast<const unsigned char *>(ba.data());
    const secp256k1_context     *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_pubkey            pubkey;
    int                         ret = 0;
    size_t                      clen = compressedFlag ? 33 : 65;
    std::vector<unsigned char>  ResultBuffer;

    ret = secp256k1_ec_pubkey_create(ctx, &pubkey, seckey);
    assert(ret);

    ResultBuffer.resize(clen, 0);

    ret = secp256k1_ec_pubkey_serialize(
                ctx,
                &ResultBuffer[0],
                &clen,
                &pubkey,
                compressedFlag ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);

    assert(ret);

    return QString(QByteArray(reinterpret_cast<const char *>(&ResultBuffer[0]), clen).toHex());
};

QString helper::getPrivateKeysSum(const QString &key1, const QString &key2)
{
    const secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    QByteArray ba1 = QByteArray::fromHex(key1.toUtf8().data());
    unsigned char *result = reinterpret_cast<unsigned char *>(ba1.data());

    QByteArray ba2 = QByteArray::fromHex(key2.toUtf8().data());
    const unsigned char *tweak = reinterpret_cast<const unsigned char *>(ba2.data());

    bool ret = secp256k1_ec_privkey_tweak_add(ctx, result, tweak);
    assert(ret);

    return QString(QByteArray(reinterpret_cast<const char*>(result), strlen((char*)result)).toHex());
}

int helper::qt_secp256k1_ec_privkey_tweak_mul(const secp256k1_context* ctx, unsigned char *seckey, const unsigned char *tweak) {
    secp256k1_scalar factor;
    secp256k1_scalar sec;
    int ret = 0;
    int overflow = 0;
    VERIFY_CHECK(ctx != NULL);
    ARG_CHECK(seckey != NULL);
    ARG_CHECK(tweak != NULL);

    secp256k1_scalar_set_b32(&factor, tweak, &overflow);
    secp256k1_scalar_set_b32(&sec, seckey, NULL);
    ret = !overflow && secp256k1_eckey_privkey_tweak_mul(&sec, &factor);
    //memset(seckey, 0, 32);
    //seckey = new unsigned char[32]();
    //seckey = calloc(32, sizeof(unsigned char));
    if (ret) {
        secp256k1_scalar_get_b32(seckey, &sec);
    }

    secp256k1_scalar_clear(&sec);
    secp256k1_scalar_clear(&factor);
    return ret;
}

QString helper::getPrivateKeysMultiplication(const QString &key1, const QString &key2)
{
    const secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN); // | SECP256K1_CONTEXT_VERIFY

    QByteArray ba1 = QByteArray::fromHex(key1.toUtf8().data());
    unsigned char *result = reinterpret_cast<unsigned char *>(ba1.data());

    QByteArray ba2 = QByteArray::fromHex(key2.toUtf8().data());
    const unsigned char *tweak = reinterpret_cast<const unsigned char *>(ba2.data());

    int ret = secp256k1_ec_privkey_tweak_mul(ctx, result, tweak);
    assert(ret);

    return QString(QByteArray(reinterpret_cast<const char*>(result), strlen((char*)result)).toHex());
}

QString helper::getPublicKeysSum(const QString &key1, const QString &key2, bool compressedFlag)
{
    secp256k1_context           *Context = nullptr;
    size_t                      clen = compressedFlag ? 33 : 65;
    QByteArray                  ba1;
    QByteArray                  ba2;
    secp256k1_pubkey            pubkey;
    int                         ret;
    std::vector<unsigned char>  ResultBuffer;

    Context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

    ba1 = QByteArray::fromHex(key1.toUtf8().data());

    ret = secp256k1_ec_pubkey_parse(
                Context,
                &pubkey,
                reinterpret_cast<const unsigned char *>(ba1.data()),
                clen);
    assert(ret);

    ba2 = QByteArray::fromHex(key2.toUtf8().data());

    ret = secp256k1_ec_pubkey_tweak_add(
                Context,
                &pubkey,
                reinterpret_cast<const unsigned char *>(ba2.data()));
    assert(ret);

    ResultBuffer.resize(clen, 0);

    ret = secp256k1_ec_pubkey_serialize(
            Context,
            &ResultBuffer[0],
            &clen,
            &pubkey,
            compressedFlag ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);

    assert(ret);

    return "";
}

QString helper::GetRandomString()
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 255; // assuming you want random strings of 12 characters

   QString randomString;
   for(int i=0; i<randomStringLength; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}

QString helper::generateWIF()
{
    QString phrase = GetRandomString();
    QString privECDSAKey = helper::getHexHashSha256FromString(phrase).toUpper();
    QString prependVersion = QString("80" + privECDSAKey);
    QString stingSHA256HashOf2 = helper::getHexHashSha256FromHexString(prependVersion).toUpper();
    QString stingSHA256HashOf3 = helper::getHexHashSha256FromHexString(stingSHA256HashOf2).toUpper();
    QByteArray first4BitesOf4;
    for (int i = 0; i < 8; i++) {
        first4BitesOf4.append(stingSHA256HashOf3.at(i));
    }
    QString stringFirst4BitesOf4 = QString(first4BitesOf4);
    QString beforeBase58 = prependVersion + first4BitesOf4;
    return helper::encodeBase58(beforeBase58);
}
