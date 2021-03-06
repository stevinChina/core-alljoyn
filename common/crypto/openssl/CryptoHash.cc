/**
 * @file Crypto.cc
 *
 * Implementation for methods from Crypto.h
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <ctype.h>

#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Crypto.h>
#include <qcc/Util.h>

#include <Status.h>
#include "OpenSsl.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "CRYPTO"

namespace qcc {
#ifndef QCC_LINUX_OPENSSL_GT_1_1_X
class Crypto_Hash::Context {
  public:

    Context(bool MAC) : MAC(MAC) { }

    /// Union of context storage for HMAC or MD.
    union {
        HMAC_CTX hmac;    ///< Storage for the HMAC context.
        EVP_MD_CTX md;    ///< Storage for the MD context.
        uint8_t pad[512]; ///< Ensure we have enough storage for openssl 1.0
    };

    bool MAC;

  private:
    Context() { }
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
};

QStatus Crypto_Hash::Init(Algorithm alg, const uint8_t* hmacKey, size_t keyLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;

    if (nullptr != ctx) {
        delete ctx;
        ctx = nullptr;
        initialized = false;
    }

    MAC = (hmacKey != nullptr);

    if (MAC && (0 == keyLen)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("HMAC key length cannot be zero"));
        delete ctx;
        ctx = nullptr;
        return status;
    }

    const EVP_MD* mdAlgorithm = nullptr;

    /*
     * Select the hash algorithm.
     * No memory allocation/initialization is done; OK to return.
     */
    switch (alg) {
    case qcc::Crypto_Hash::SHA1:
        mdAlgorithm = EVP_sha1();
        break;

    case qcc::Crypto_Hash::SHA256:
        mdAlgorithm = EVP_sha256();
        break;

    default:
        status = ER_BAD_ARG_1;
        QCC_LogError(status, ("Unsupported hash algorithm %d", alg));
        return status;
    }

    ctx = new Crypto_Hash::Context(MAC);

    if (MAC) {
        HMAC_CTX_init(&ctx->hmac);
        HMAC_Init_ex(&ctx->hmac, hmacKey, keyLen, mdAlgorithm, nullptr);
    } else if (EVP_DigestInit(&ctx->md, mdAlgorithm) == 0) {
        status = ER_CRYPTO_ERROR;
        EVP_MD_CTX_cleanup(&ctx->md);
        QCC_LogError(status, ("Failed to initialize hash digest"));
    }
    if (ER_OK == status) {
        initialized = true;
    } else {
        delete ctx;
        ctx = nullptr;
    }
    return status;
}

Crypto_Hash::~Crypto_Hash(void)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (nullptr != ctx) {
        if (initialized) {
            if (MAC) {
                HMAC_CTX_cleanup(&ctx->hmac);
            } else {
                EVP_MD_CTX_cleanup(&ctx->md);
            }
        }
        delete ctx;
    }
}

QStatus Crypto_Hash::Update(const uint8_t* buf, size_t bufSize)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;

    if (nullptr == buf) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (MAC) {
            HMAC_Update(&ctx->hmac, buf, bufSize);
        } else if (EVP_DigestUpdate(&ctx->md, buf, bufSize) == 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to update hash digest"));
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

QStatus Crypto_Hash::Update(const qcc::String& str)
{
    return Update((const uint8_t*)str.data(), str.size());
}

QStatus Crypto_Hash::Update(const vector<uint8_t, SecureAllocator<uint8_t> >& d)
{
    return Update(d.data(), d.size());
}

QStatus Crypto_Hash::GetDigest(uint8_t* digest, bool keepAlive)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;

    if (nullptr == digest) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (MAC) {
            /* keep alive is not allowed for HMAC */
            if (keepAlive) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Keep alive is not allowed for HMAC"));
                keepAlive = false;
            }
            HMAC_Final(&ctx->hmac, digest, nullptr);
            HMAC_CTX_cleanup(&ctx->hmac);
            initialized = false;
        } else {
            Context* keep = nullptr;
            /* To keep the hash alive we need to copy the context before calling EVP_DigestFinal */
            if (keepAlive) {
                keep = new Context(false);
                EVP_MD_CTX_copy(&keep->md, &ctx->md);
            }
            if (EVP_DigestFinal(&ctx->md, digest, nullptr) == 0) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Failed to finalize hash digest"));
            }
            EVP_MD_CTX_cleanup(&ctx->md);
            if (nullptr != keep) {
                delete ctx;
                ctx = keep;
            } else {
                initialized = false;
            }
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}
#else
class Crypto_Hash::Context {
  public:

    Context(bool MAC) : MAC(MAC) { }

    /// Union of context storage for HMAC or MD.
    union {
        HMAC_CTX* hmac;    ///< Storage for the HMAC context.
        EVP_MD_CTX* md;    ///< Storage for the MD context.
    };

    bool MAC;

  private:
    Context() { }
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

};

QStatus Crypto_Hash::Init(Algorithm alg, const uint8_t* hmacKey, size_t keyLen)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;

    if (nullptr != ctx) {
        delete ctx;
        ctx = nullptr;
        initialized = false;
    }

    MAC = (hmacKey != nullptr);

    if (MAC && (0 == keyLen)) {
        status = ER_CRYPTO_ERROR;
        QCC_LogError(status, ("HMAC key length cannot be zero"));
        delete ctx;
        ctx = nullptr;
        return status;
    }

    const EVP_MD* mdAlgorithm = nullptr;

    /*
     * Select the hash algorithm.
     * No memory allocation/initialization is done; OK to return.
     */
    switch (alg) {
    case qcc::Crypto_Hash::SHA1:
        mdAlgorithm = EVP_sha1();
        break;

    case qcc::Crypto_Hash::SHA256:
        mdAlgorithm = EVP_sha256();
        break;

    default:
        status = ER_BAD_ARG_1;
        QCC_LogError(status, ("Unsupported hash algorithm %d", alg));
        return status;
    }

    ctx = new Crypto_Hash::Context(MAC);
    if (MAC) {
        ctx->hmac = HMAC_CTX_new();
        HMAC_Init_ex(ctx->hmac, hmacKey, keyLen, mdAlgorithm, nullptr);
    } else {
        ctx->md = EVP_MD_CTX_create();
        if (EVP_DigestInit_ex(ctx->md, mdAlgorithm, nullptr) == 0) {
            status = ER_CRYPTO_ERROR;
            EVP_MD_CTX_free(ctx->md);
            QCC_LogError(status, ("Failed to initialize hash digest"));
        }
    }
    if (ER_OK == status) {
        initialized = true;
    } else {
        delete ctx;
        ctx = nullptr;
    }
    return status;
}

Crypto_Hash::~Crypto_Hash(void)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    if (nullptr != ctx) {
        if (initialized) {
            if (MAC) {
                HMAC_CTX_free(ctx->hmac);

            } else {
                EVP_MD_CTX_free(ctx->md);
            }
        }
        delete ctx;
    }
}

QStatus Crypto_Hash::Update(const uint8_t* buf, size_t bufSize)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;

    if (nullptr == buf) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (MAC) {
            HMAC_Update(ctx->hmac, buf, bufSize);
        } else if (EVP_DigestUpdate(ctx->md, buf, bufSize) == 0) {
            status = ER_CRYPTO_ERROR;
            QCC_LogError(status, ("Failed to update hash digest"));
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}

QStatus Crypto_Hash::Update(const qcc::String& str)
{
    return Update((const uint8_t*)str.data(), str.size());
}

QStatus Crypto_Hash::Update(const vector<uint8_t, SecureAllocator<uint8_t> >& d)
{
    return Update(d.data(), d.size());
}

QStatus Crypto_Hash::GetDigest(uint8_t* digest, bool keepAlive)
{
    /*
     * Protect the open ssl APIs.
     */
    OpenSsl_ScopedLock lock;

    QStatus status = ER_OK;

    if (nullptr == digest) {
        return ER_BAD_ARG_1;
    }
    if (initialized) {
        if (MAC) {
            /* keep alive is not allowed for HMAC */
            if (keepAlive) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Keep alive is not allowed for HMAC"));
                keepAlive = false;
            }
            HMAC_Final(ctx->hmac, digest, nullptr);
            HMAC_CTX_free(ctx->hmac);
            initialized = false;
        } else {
            Context* keep = nullptr;
            /* To keep the hash alive we need to copy the context before calling EVP_DigestFinal */
            if (keepAlive) {
                keep = new Context(false);
                keep->md = EVP_MD_CTX_create();
                EVP_MD_CTX_copy(keep->md, ctx->md);
            }
            if (EVP_DigestFinal(ctx->md, digest, nullptr) == 0) {
                status = ER_CRYPTO_ERROR;
                QCC_LogError(status, ("Failed to finalize hash digest"));
            }
            EVP_MD_CTX_free(ctx->md);
            if (nullptr != keep) {
                delete ctx;
                ctx = keep;
            } else {
                initialized = false;
            }
        }
    } else {
        status = ER_CRYPTO_HASH_UNINITIALIZED;
        QCC_LogError(status, ("Hash function not initialized"));
    }
    return status;
}
#endif
}
