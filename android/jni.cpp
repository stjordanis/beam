// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "wallet/wallet_db.h"
#include "wallet/keystore.h"

#include <jni.h>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "beamwallet::", __VA_ARGS__))

#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, "beamwallet::", __VA_ARGS__))
#else
#define LOGI(...) {printf("beamwallet::"); printf(__VA_ARGS__); putc('\n', stdout);}
#define LOGE(...) {printf("beamwallet::"); printf(__VA_ARGS__); putc('\n', stderr);}
#endif


#define CONCAT1(prefix, class, function)    CONCAT2(prefix, class, function)
#define CONCAT2(prefix, class, function)    Java_ ## prefix ## _ ## class ## _ ## function

#define BEAM_JAVA_PREFIX                    com_mw_beam_beamwallet
#define BEAM_JAVA_INTERFACE(function)       CONCAT1(BEAM_JAVA_PREFIX, core_Api, function)

#define WALLET_FILENAME "wallet.db"
#define BBS_FILENAME "keys.bbs"

using namespace beam;

namespace
{
    struct JString
    {
        JString(JNIEnv *envVal, jstring nameVal)
            : env(envVal)
            , name(nameVal)
            , isCopy(JNI_FALSE)
            , data(env->GetStringUTFChars(name, &isCopy))
        {
        }

        ~JString()
        {
            if (isCopy == JNI_TRUE)
            {
                env->ReleaseStringUTFChars(name, data);
            }
        }

        std::string value() const
        {
            return data;
        }
    private:
        JNIEnv* env;
        jstring name;
        jboolean isCopy;
        const char* data;
    };

    using WalletDBList = std::vector<IKeyChain::Ptr>;
    static WalletDBList wallets;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL BEAM_JAVA_INTERFACE(createWallet)(JNIEnv *env, jobject thiz, 
    jstring appDataStr, jstring passStr, jstring seed)
{
    LOGI("creating wallet...");

    auto appData = JString(env, appDataStr).value();
    auto pass = JString(env, passStr).value();

    auto wallet = Keychain::init(
        appData + "/" WALLET_FILENAME,
        pass,
        SecString(JString(env, seed).value()).hash());

    if(wallet)
    {
        LOGI("wallet successfully created.");

        wallets.push_back(wallet);

        {
            IKeyStore::Options options;
            options.flags = IKeyStore::Options::local_file | IKeyStore::Options::enable_all_keys;
            options.fileName = appData + "/" BBS_FILENAME;

            IKeyStore::Ptr keystore = IKeyStore::create(options, pass.data(), pass.size());

            // generate default address
            WalletAddress defaultAddress = {};
            defaultAddress.m_own = true;
            defaultAddress.m_label = "default";
            defaultAddress.m_createTime = getTimestamp();
            defaultAddress.m_duration = std::numeric_limits<uint64_t>::max();
            keystore->gen_keypair(defaultAddress.m_walletID);
            keystore->save_keypair(defaultAddress.m_walletID, true);

            wallet->saveAddress(defaultAddress);
        }

        return wallets.size() - 1;
    }

    LOGE("wallet creation error.");

    return -1;
}

JNIEXPORT jboolean JNICALL BEAM_JAVA_INTERFACE(isWalletInitialized)(JNIEnv *env, jobject thiz, 
    jstring appData)
{
    LOGI("checking if wallet exists...");

    return Keychain::isInitialized(JString(env, appData).value() + "/" WALLET_FILENAME) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL BEAM_JAVA_INTERFACE(openWallet)(JNIEnv *env, jobject thiz, 
    jstring appData, jstring pass)
{
    LOGI("opening wallet...");

    auto wallet = Keychain::open(JString(env, appData).value() + "/" WALLET_FILENAME, JString(env, pass).value());

    if(wallet)
    {
        LOGI("wallet successfully opened.");

        wallets.push_back(wallet);

        return wallets.size() - 1;
    }

    LOGE("wallet not opened.");

    return -1;
}


JNIEXPORT void JNICALL BEAM_JAVA_INTERFACE(closeWallet)(JNIEnv *env, jobject thiz, 
    int index)
{
    LOGI("closing wallet...");

    if(index < wallets.size() && wallets[index])
    {
        wallets[index].reset();
        LOGI("wallet successfully closed.");
        return;
    }
    else
    {
        // raise error here
    }

    LOGE("wallet doesn't exist or already closed.");
}

JNIEXPORT void JNICALL BEAM_JAVA_INTERFACE(checkWalletPassword)(JNIEnv *env, jobject thiz,
    int index, jstring passStr)
{
    LOGI("changing wallet password...");

    if (index < wallets.size() && wallets[index])
    {
        auto wallet = wallets[index];
        wallet->changePassword(JString(env, passStr).value());
    }
    else
    {
        // raise error here
    }
}

JNIEXPORT jobject JNICALL BEAM_JAVA_INTERFACE(getSystemState)(JNIEnv *env, jobject thiz,
    int index)
{
    LOGI("getting System State...");

    if (index < wallets.size() && wallets[index])
    {
        auto wallet = wallets[index];

        Block::SystemState::ID stateID = {};
        wallet->getSystemStateID(stateID);

        {
            jclass SystemState = env->FindClass("com/mw/beam/beamwallet/core/SystemState");
            jobject systemState = env->AllocObject(SystemState);

            {
                jfieldID height = env->GetFieldID(SystemState, "height", "J");
                env->SetLongField(systemState, height, stateID.m_Height);
            }

            {
                jbyteArray hash = env->NewByteArray(ECC::uintBig::nBytes);
                jbyte* hashBytes = env->GetByteArrayElements(hash, NULL);

                memcpy(hashBytes, stateID.m_Hash.m_pData, ECC::uintBig::nBytes);

                jfieldID hashField = env->GetFieldID(SystemState, "hash", "[B");
                env->SetObjectField(systemState, hashField, hash);

                env->ReleaseByteArrayElements(hash, hashBytes, 0);
            }

            return systemState;
        }
    }
    else
    {
        // raise error here
    }

    return nullptr;
}

JNIEXPORT jobject JNICALL BEAM_JAVA_INTERFACE(getUtxos)(JNIEnv *env, jobject thiz,
    int index)
{
    LOGI("getting System State...");

    if (index < wallets.size() && wallets[index])
    {
        auto wallet = wallets[index];

        jclass Utxo = env->FindClass("com/mw/beam/beamwallet/core/Utxo");
        std::vector<jobject> utxosVec;

        wallet->visit([&](const Coin& coin)->bool
        {
            jobject utxo = env->AllocObject(Utxo);

            {
                jfieldID height = env->GetFieldID(Utxo, "id", "J");
                env->SetLongField(utxo, height, coin.m_id);
            }

            {
                jfieldID amount = env->GetFieldID(Utxo, "amount", "J");
                env->SetLongField(utxo, amount, coin.m_amount);
            }

            {
                jfieldID status = env->GetFieldID(Utxo, "status", "I");
                env->SetIntField(utxo, status, coin.m_status);
            }

            {
                jfieldID createHeight = env->GetFieldID(Utxo, "createHeight", "J");
                env->SetLongField(utxo, createHeight, coin.m_createHeight);
            }

            {
                jfieldID maturity = env->GetFieldID(Utxo, "maturity", "J");
                env->SetLongField(utxo, maturity, coin.m_maturity);
            }

            {
                jfieldID keyType = env->GetFieldID(Utxo, "keyType", "I");
                env->SetIntField(utxo, keyType, static_cast<jint>(coin.m_key_type));
            }

            utxosVec.push_back(utxo);

            return true;
        });

        jobjectArray utxos = env->NewObjectArray(utxosVec.size(), Utxo, NULL);

        for(int i = 0; i < utxosVec.size(); ++i)
            env->SetObjectArrayElement(utxos, i, utxosVec[i]);

        return utxos;
    }
    else
    {
        // raise error here
    }

    return nullptr;
}

#ifdef __cplusplus
}
#endif