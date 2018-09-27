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

#define CONCAT1(prefix, class, function)    CONCAT2(prefix, class, function)
#define CONCAT2(prefix, class, function)    Java_ ## prefix ## _ ## class ## _ ## function

#define BEAM_JAVA_PREFIX                    com_senlainc_beamwallet_core
#define BEAM_JAVA_INTERFACE(function)       CONCAT1(BEAM_JAVA_PREFIX, Api, function)

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
}

extern "C"
{
    JNIEXPORT jstring JNICALL BEAM_JAVA_INTERFACE(createWallet)(JNIEnv *env, jobject thiz, 
        jstring j_dbName, jstring j_pass, jstring j_seed)
    {
        LOGI("creating Beam wallet...");

        JString dbName(env, j_dbName);
        JString pass(env, j_pass);
        JString seed(env, j_seed);

        auto keychain = beam::Keychain::init(dbName.value(), pass.value(), beam::SecString(seed.value()).hash());

        if(keychain)
        {
            LOGI("wallet successfully created.");

            return env->NewStringUTF("creating Beam wallet!");
        }

        LOGI("wallet creation error.");

        return env->NewStringUTF("wallet creation error.");
    }    
}
