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

#include <jni.h>
#include <android/log.h>

#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, "beamwallet::", __VA_ARGS__))

#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, "beamwallet::", __VA_ARGS__))

#define CONCAT1(prefix, class, function)    CONCAT2(prefix, class, function)
#define CONCAT2(prefix, class, function)    Java_ ## prefix ## _ ## class ## _ ## function

#define BEAM_JAVA_PREFIX                    com_senlainc_beamwallet
#define BEAM_JAVA_INTERFACE(function)       CONCAT1(BEAM_JAVA_PREFIX, core_Api, function)

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

    using WalletDBList = std::vector<beam::IKeyChain::Ptr>;
    static WalletDBList wallets;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL BEAM_JAVA_INTERFACE(createWallet)(JNIEnv *env, jobject thiz, 
    jstring dbName, jstring pass, jstring seed)
{
    LOGI("creating wallet...");

    auto wallet = beam::Keychain::init(
        JString(env, dbName).value(), 
        JString(env, pass).value(), 
        beam::SecString(JString(env, seed).value()).hash());

    if(wallet)
    {
        LOGI("wallet successfully created.");

        wallets.push_back(wallet);

        return wallets.size() - 1;
    }

    LOGE("wallet creation error.");

    return -1;
}

JNIEXPORT jboolean JNICALL BEAM_JAVA_INTERFACE(isWalletInitialized)(JNIEnv *env, jobject thiz, 
    jstring dbName)
{
    LOGI("checking if wallet exists...");

    return beam::Keychain::isInitialized(JString(env, dbName).value()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL BEAM_JAVA_INTERFACE(openWallet)(JNIEnv *env, jobject thiz, 
    jstring dbName, jstring pass)
{
    LOGI("opening wallet...");

    auto wallet = beam::Keychain::open(JString(env, dbName).value(), JString(env, pass).value());

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

    LOGE("wallet doesn't exist or already closed.");
}

#ifdef __cplusplus
}
#endif