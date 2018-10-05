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

#include "wallet/wallet.h"
#include "wallet/wallet_db.h"
#include "wallet/keystore.h"
#include "wallet/wallet_network.h"

#include <boost/filesystem.hpp>
#include <jni.h>

#define CONCAT1(prefix, class, function)    CONCAT2(prefix, class, function)
#define CONCAT2(prefix, class, function)    Java_ ## prefix ## _ ## class ## _ ## function

#define DEF2STR2(x) #x
#define DEF2STR(x) DEF2STR2(x)

#define BEAM_JAVA_PACKAGE(sep) 					com ## sep ## mw ## sep ## beam ## sep ## beamwallet ## sep ## core
#define BEAM_JAVA_PREFIX 						BEAM_JAVA_PACKAGE(_)
#define BEAM_JAVA_PATH 							"com/mw/beam/beamwallet/core" // doesn't work on clang DEF2STR(BEAM_JAVA_PACKAGE(/))
#define BEAM_JAVA_API_INTERFACE(function) 		CONCAT1(BEAM_JAVA_PREFIX, Api, function)
#define BEAM_JAVA_WALLET_INTERFACE(function) 	CONCAT1(BEAM_JAVA_PREFIX, entities_Wallet, function)

#define WALLET_FILENAME "wallet.db"
#define BBS_FILENAME "keys.bbs"

using namespace beam;
using namespace std;

namespace fs = boost::filesystem;

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

		string value() const
		{
			return data;
		}
	private:
		JNIEnv* env;
		jstring name;
		jboolean isCopy;
		const char* data;
	};

	struct WalletRecord
	{
		IKeyChain::Ptr keychain;
		IKeyStore::Ptr keystore;
	};

	vector<WalletRecord> wallets;

	inline void setLongField(JNIEnv *env, jclass clazz, jobject obj, const char* name, jlong value)
	{
		env->SetLongField(obj, env->GetFieldID(clazz, name, "J"), value);
	}

	inline void setIntField(JNIEnv *env, jclass clazz, jobject obj, const char* name, jint value)
	{
		env->SetIntField(obj, env->GetFieldID(clazz, name, "I"), value);
	}

	inline void setBooleanField(JNIEnv *env, jclass clazz, jobject obj, const char* name, jboolean value)
	{
		env->SetBooleanField(obj, env->GetFieldID(clazz, name, "Z"), value);
	}


	template <typename T>
	inline void setByteArrayField(JNIEnv *env, jclass clazz, jobject obj, const char* name, const T& value)
	{
		if (value.size())
		{
			jbyteArray hash = env->NewByteArray(static_cast<jsize>(value.size()));
			jbyte* hashBytes = env->GetByteArrayElements(hash, NULL);

			memcpy(hashBytes, &value[0], value.size());

			env->SetObjectField(obj, env->GetFieldID(clazz, name, "[B"), hash);

			env->ReleaseByteArrayElements(hash, hashBytes, 0);
		}
	}

	template <>
	inline void setByteArrayField<ECC::uintBig>(JNIEnv *env, jclass clazz, jobject obj, const char* name, const ECC::uintBig& value)
	{
		vector<uint8_t> data;
		data.assign(value.m_pData, value.m_pData + ECC::uintBig::nBytes);
		setByteArrayField(env, clazz, obj, name, data);
	}

	jobject regWallet(JNIEnv *env, jobject thiz, IKeyChain::Ptr keychain, IKeyStore::Ptr keystore)
	{
		wallets.push_back(WalletRecord{keychain, keystore});

		jclass Wallet = env->FindClass(BEAM_JAVA_PATH "/entities/Wallet");
		jobject walletObj = env->AllocObject(Wallet);

		setLongField(env, Wallet, walletObj, "_this", wallets.size() - 1);

		return walletObj;
	}

	WalletRecord getWallet(JNIEnv *env, jobject thiz)
	{
		jclass Wallet = env->FindClass(BEAM_JAVA_PATH "/entities/Wallet");
		jfieldID _this = env->GetFieldID(Wallet, "_this", "J");
		return wallets[env->GetLongField(thiz, _this)];
	}

	class WalletObserver : public beam::IWalletObserver
	{
		JNIEnv* _env;
		jobject _listener;
		jclass _WalletListenerClass;

	public:

		WalletObserver(JNIEnv *env, jobject listener)
			: _env(env)
			, _listener(listener)
		{
			_WalletListenerClass = _env->GetObjectClass(_listener);
		}

		void onKeychainChanged() override 
		{
			jmethodID callback = _env->GetMethodID(_WalletListenerClass, "onKeychainChanged", "()V");
			_env->CallVoidMethod(_listener, callback);
		}

		void onTransactionChanged(beam::ChangeAction action, vector<beam::TxDescription>&& items) override
		{
			jmethodID callback = _env->GetMethodID(_WalletListenerClass, "onTransactionChanged", "()V");
			_env->CallVoidMethod(_listener, callback);
		}

		void onSystemStateChanged() override
		{
			jmethodID callback = _env->GetMethodID(_WalletListenerClass, "onSystemStateChanged", "()V");
			_env->CallVoidMethod(_listener, callback);
		}

		void onTxPeerChanged() override
		{
			jmethodID callback = _env->GetMethodID(_WalletListenerClass, "onTxPeerChanged", "()V");
			_env->CallVoidMethod(_listener, callback);
		}

		void onAddressChanged() override
		{
			jmethodID callback = _env->GetMethodID(_WalletListenerClass, "onAddressChanged", "()V");
			_env->CallVoidMethod(_listener, callback);
		}

		void onSyncProgress(int done, int total) override
		{
			jmethodID callback = _env->GetMethodID(_WalletListenerClass, "onSyncProgress", "(II)V");
			_env->CallVoidMethod(_listener, callback, done, total);
		}
	};

	void initLogger(const string& appData)
	{
		static auto logger = beam::Logger::create(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, "wallet_", (fs::path(appData) / fs::path("logs")).string());
	}
}


#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jobject JNICALL BEAM_JAVA_API_INTERFACE(createWallet)(JNIEnv *env, jobject thiz, 
	jstring appDataStr, jstring passStr, jstring seed)
{
	auto appData = JString(env, appDataStr).value();

	initLogger(appData);
	
	LOG_DEBUG() << "creating wallet...";

	auto pass = JString(env, passStr).value();

	auto wallet = Keychain::init(
		appData + "/" WALLET_FILENAME,
		pass,
		SecString(JString(env, seed).value()).hash());

	if(wallet)
	{
		LOG_DEBUG() << "wallet successfully created.";

		IKeyStore::Options options;
		options.flags = IKeyStore::Options::local_file | IKeyStore::Options::enable_all_keys;
		options.fileName = appData + "/" BBS_FILENAME;

		IKeyStore::Ptr keystore = IKeyStore::create(options, pass.data(), pass.size());

		// generate default address
		WalletAddress defaultAddress = {};
		defaultAddress.m_own = true;
		defaultAddress.m_label = "default";
		defaultAddress.m_createTime = getTimestamp();
		defaultAddress.m_duration = numeric_limits<uint64_t>::max();
		keystore->gen_keypair(defaultAddress.m_walletID);
		keystore->save_keypair(defaultAddress.m_walletID, true);

		wallet->saveAddress(defaultAddress);

		return regWallet(env, thiz, wallet, keystore);
	}

	LOG_ERROR() << "wallet creation error.";

	return nullptr;
}

JNIEXPORT jboolean JNICALL BEAM_JAVA_API_INTERFACE(isWalletInitialized)(JNIEnv *env, jobject thiz, 
	jstring appData)
{
	LOG_DEBUG() << "checking if wallet exists...";

	return Keychain::isInitialized(JString(env, appData).value() + "/" WALLET_FILENAME) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jobject JNICALL BEAM_JAVA_API_INTERFACE(openWallet)(JNIEnv *env, jobject thiz, 
	jstring appDataStr, jstring passStr)
{
	auto appData = JString(env, appDataStr).value();

	initLogger(appData);

	LOG_DEBUG() << "opening wallet...";

	string pass = JString(env, passStr).value();
	auto wallet = Keychain::open(appData + "/" WALLET_FILENAME, pass);

	if(wallet)
	{
		LOG_DEBUG() << "wallet successfully opened.";

		IKeyStore::Options options;
		options.flags = IKeyStore::Options::local_file | IKeyStore::Options::enable_all_keys;
		options.fileName = appData + "/" BBS_FILENAME;

		IKeyStore::Ptr keystore = IKeyStore::create(options, pass.data(), pass.size());

		return regWallet(env, thiz, wallet, keystore);
	}

	LOG_ERROR() << "wallet not opened.";

	return nullptr;
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(closeWallet)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "closing wallet...";

	getWallet(env, thiz).keychain.reset();
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(checkWalletPassword)(JNIEnv *env, jobject thiz, jstring passStr)
{
	LOG_DEBUG() << "changing wallet password...";

	getWallet(env, thiz).keychain->changePassword(JString(env, passStr).value());
}

JNIEXPORT jobject JNICALL BEAM_JAVA_WALLET_INTERFACE(getSystemState)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "getting System State...";

	Block::SystemState::ID stateID = {};
	getWallet(env, thiz).keychain->getSystemStateID(stateID);

	jclass SystemState = env->FindClass(BEAM_JAVA_PATH "/entities/SystemState");
	jobject systemState = env->AllocObject(SystemState);

	setLongField(env, SystemState, systemState, "height", stateID.m_Height);
	setByteArrayField(env, SystemState, systemState, "hash", stateID.m_Hash);

	return systemState;
}

JNIEXPORT jobject JNICALL BEAM_JAVA_WALLET_INTERFACE(getUtxos)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "getting Utxos...";

	jclass Utxo = env->FindClass(BEAM_JAVA_PATH "/entities/Utxo");
	vector<jobject> utxosVec;

	getWallet(env, thiz).keychain->visit([&](const Coin& coin)->bool
	{
		jobject utxo = env->AllocObject(Utxo);

		setLongField(env, Utxo, utxo, "id", coin.m_id);
		setLongField(env, Utxo, utxo, "amount", coin.m_amount);
		setIntField(env, Utxo, utxo, "status", coin.m_status);
		setLongField(env, Utxo, utxo, "createHeight", coin.m_createHeight);
		setLongField(env, Utxo, utxo, "maturity", coin.m_maturity);
		setIntField(env, Utxo, utxo, "keyType", static_cast<jint>(coin.m_key_type));
		setLongField(env, Utxo, utxo, "confirmHeight", coin.m_confirmHeight);
		setByteArrayField(env, Utxo, utxo, "confirmHash", coin.m_confirmHash);
		setLongField(env, Utxo, utxo, "lockHeight", coin.m_lockedHeight);

		if(coin.m_createTxId)
			setByteArrayField(env, Utxo, utxo, "createTxId", *coin.m_createTxId);

		if (coin.m_spentTxId)
			setByteArrayField(env, Utxo, utxo, "spentTxId", *coin.m_spentTxId);

		utxosVec.push_back(utxo);

		return true;
	});

	jobjectArray utxos = env->NewObjectArray(static_cast<jsize>(utxosVec.size()), Utxo, NULL);

	for (int i = 0; i < utxosVec.size(); ++i)
		env->SetObjectArrayElement(utxos, i, utxosVec[i]);

	return utxos;
}

JNIEXPORT jobject JNICALL BEAM_JAVA_WALLET_INTERFACE(getTxHistory)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "getting transaction history...";

	jclass TxDescription = env->FindClass(BEAM_JAVA_PATH "/entities/TxDescription");

	auto txHistory = getWallet(env, thiz).keychain->getTxHistory();

	jobjectArray txs = env->NewObjectArray(static_cast<jsize>(txHistory.size()), TxDescription, NULL);

	for (int i = 0; i < txHistory.size(); ++i)
	{
		const auto& tx = txHistory[i];
		jobject txObj = env->AllocObject(TxDescription);

		setByteArrayField(env, TxDescription, txObj, "id", tx.m_txId);
		setLongField(env, TxDescription, txObj, "amount", tx.m_amount);
		setLongField(env, TxDescription, txObj, "fee", tx.m_fee);
		setLongField(env, TxDescription, txObj, "change", tx.m_change);
		setLongField(env, TxDescription, txObj, "minHeight", tx.m_minHeight);
		setByteArrayField(env, TxDescription, txObj, "peerId", tx.m_peerId);
		setByteArrayField(env, TxDescription, txObj, "myId", tx.m_myId);
		setByteArrayField(env, TxDescription, txObj, "message", tx.m_message);
		setLongField(env, TxDescription, txObj, "createTime", tx.m_createTime);
		setLongField(env, TxDescription, txObj, "modifyTime", tx.m_modifyTime);
		setBooleanField(env, TxDescription, txObj, "sender", tx.m_sender);
		setIntField(env, TxDescription, txObj, "status", tx.m_status);

		env->SetObjectArrayElement(txs, i, txObj);
	}

	return txs;
}

JNIEXPORT jlong JNICALL BEAM_JAVA_WALLET_INTERFACE(getAvailable)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "getting available money...";

	return wallet::getAvailable(getWallet(env, thiz).keychain);
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(run)(JNIEnv *env, jobject thiz,
	jstring nodeAddr, jobject listener)
{
	LOG_DEBUG() << "run wallet...";

	io::Reactor::Ptr reactor = io::Reactor::create();
	io::Reactor::Scope scope(*reactor);

	io::Reactor::GracefulIntHandler gih(*reactor);

	string nodeURI = JString(env, nodeAddr).value();
	io::Address node_addr;
	
	if (!node_addr.resolve(nodeURI.c_str()))
	{
		LOG_ERROR() << "unable to resolve node address: " << nodeURI.c_str();
		return;
	}

	auto keychain = getWallet(env, thiz).keychain;
	auto wallet_io = make_shared<WalletNetworkIO>(node_addr, keychain, getWallet(env, thiz).keystore, reactor);

	Wallet wallet{ keychain, wallet_io};
	WalletObserver observer(env, listener);

	wallet.subscribe(&observer);

	wallet_io->start();
}

#ifdef __cplusplus
}
#endif