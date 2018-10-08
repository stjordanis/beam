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

#include "utility/bridge.h"

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

	// TODO: remove copy paste from UI
	struct IWalletModelAsync
	{
		using Ptr = std::shared_ptr<IWalletModelAsync>;

		virtual void sendMoney(const beam::WalletID& sender, const beam::WalletID& receiver, beam::Amount&& amount, beam::Amount&& fee = 0) = 0;
		virtual void sendMoney(const beam::WalletID& receiver, const std::string& comment, beam::Amount&& amount, beam::Amount&& fee = 0) = 0;
		virtual void syncWithNode() = 0;
		virtual void calcChange(beam::Amount&& amount) = 0;
		virtual void getWalletStatus() = 0;
		virtual void getUtxosStatus() = 0;
		virtual void getAddresses(bool own) = 0;
		virtual void cancelTx(const beam::TxID& id) = 0;
		virtual void deleteTx(const beam::TxID& id) = 0;
		virtual void createNewAddress(beam::WalletAddress&& address) = 0;
		virtual void generateNewWalletID() = 0;
		virtual void changeCurrentWalletIDs(const beam::WalletID& senderID, const beam::WalletID& receiverID) = 0;

		virtual void deleteAddress(const beam::WalletID& id) = 0;
		virtual void deleteOwnAddress(const beam::WalletID& id) = 0 ;

		virtual void setNodeAddress(const std::string& addr) = 0;
		virtual void emergencyReset() = 0;

		virtual void changeWalletPassword(const beam::SecString& password) = 0;

		virtual ~IWalletModelAsync() {}
	};

	struct WalletModel : IWalletModelAsync, IWalletObserver
	{
		IKeyChain::Ptr keychain;
		IKeyStore::Ptr keystore;

		IWalletModelAsync::Ptr async;

        void initObserver(JNIEnv *env, jobject listener)
        {
            _env = env;
            _listener = listener;
            _WalletListenerClass = _env->GetObjectClass(_listener);
        }

		///////////////////////////////////////////////
		// IWalletModelAsync impl
		///////////////////////////////////////////////
		void sendMoney(const beam::WalletID& sender, const beam::WalletID& receiver, beam::Amount&& amount, beam::Amount&& fee = 0) override {}
		void sendMoney(const beam::WalletID& receiver, const std::string& comment, beam::Amount&& amount, beam::Amount&& fee = 0) override {}
		void syncWithNode() override {}
		void calcChange(beam::Amount&& amount) override {}
		void getWalletStatus() override {}
		void getUtxosStatus() override {}
		void getAddresses(bool own) override {}
		void cancelTx(const beam::TxID& id) override {}
		void deleteTx(const beam::TxID& id) override {}
		void createNewAddress(beam::WalletAddress&& address) override {}
		void generateNewWalletID() override {}
		void changeCurrentWalletIDs(const beam::WalletID& senderID, const beam::WalletID& receiverID) override {}
		void deleteAddress(const beam::WalletID& id) override {}
		void deleteOwnAddress(const beam::WalletID& id)  override {}
		void setNodeAddress(const std::string& addr) override {}
		void emergencyReset() override {}
		void changeWalletPassword(const beam::SecString& password) override {}

		///////////////////////////////////////////////
		// IWalletObserver impl
		///////////////////////////////////////////////
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

		IWalletObserver* observer()
		{
			return this;
		}
	private:
		JNIEnv* _env;
		jobject _listener;
		jclass _WalletListenerClass;
	};

	vector<WalletModel> wallets;

	struct WalletModelBridge : public Bridge<IWalletModelAsync>
	{
		BRIDGE_INIT(WalletModelBridge);

		void sendMoney(const beam::WalletID& senderID, const beam::WalletID& receiverID, Amount&& amount, Amount&& fee) override
		{
			tx.send([senderID, receiverID, amount{ move(amount) }, fee{ move(fee) }](BridgeInterface& receiver_) mutable
			{
				receiver_.sendMoney(senderID, receiverID, move(amount), move(fee));
			});
		}

		void sendMoney(const beam::WalletID& receiverID, const std::string& comment, beam::Amount&& amount, beam::Amount&& fee) override
		{
			tx.send([receiverID, comment, amount{ move(amount) }, fee{ move(fee) }](BridgeInterface& receiver_) mutable
			{
				receiver_.sendMoney(receiverID, comment, move(amount), move(fee));
			});
		}

		void syncWithNode() override
		{
			tx.send([](BridgeInterface& receiver_) mutable
			{
				receiver_.syncWithNode();
			});
		}

		void calcChange(beam::Amount&& amount) override
		{
			tx.send([amount{move(amount)}](BridgeInterface& receiver_) mutable
			{
				receiver_.calcChange(move(amount));
			});
		}

		void getWalletStatus() override
		{
			tx.send([](BridgeInterface& receiver_) mutable
			{
				receiver_.getWalletStatus();
			});
		}

		void getUtxosStatus() override
		{
			tx.send([](BridgeInterface& receiver_) mutable
			{
				receiver_.getUtxosStatus();
			});
		}

		void getAddresses(bool own) override
		{
			tx.send([own](BridgeInterface& receiver_) mutable
			{
				receiver_.getAddresses(own);
			});
		}

		void cancelTx(const beam::TxID& id) override
		{
			tx.send([id](BridgeInterface& receiver_) mutable
			{
				receiver_.cancelTx(id);
			});
		}

		void deleteTx(const beam::TxID& id) override
		{
			tx.send([id](BridgeInterface& receiver_) mutable
			{
				receiver_.deleteTx(id);
			});
		}

		void createNewAddress(WalletAddress&& address) override
		{
			tx.send([address{ move(address) }](BridgeInterface& receiver_) mutable
			{
				receiver_.createNewAddress(move(address));
			});
		}

		void changeCurrentWalletIDs(const beam::WalletID& senderID, const beam::WalletID& receiverID) override
		{
			tx.send([senderID, receiverID](BridgeInterface& receiver_) mutable
			{
				receiver_.changeCurrentWalletIDs(senderID, receiverID);
			});
		}

		void generateNewWalletID() override
		{
			tx.send([](BridgeInterface& receiver_) mutable
			{
				receiver_.generateNewWalletID();
			});
		}

		void deleteAddress(const beam::WalletID& id) override
		{
			tx.send([id](BridgeInterface& receiver_) mutable
			{
				receiver_.deleteAddress(id);
			});
		}

		void deleteOwnAddress(const beam::WalletID& id) override
		{
			tx.send([id](BridgeInterface& receiver_) mutable
			{
				receiver_.deleteOwnAddress(id);
			});
		}

		void setNodeAddress(const std::string& addr) override
		{
			tx.send([addr](BridgeInterface& receiver_) mutable
			{
				receiver_.setNodeAddress(addr);
			});
		}

		void emergencyReset() override
		{
			tx.send([](BridgeInterface& receiver_) mutable
			{
				receiver_.emergencyReset();
			});
		}

		void changeWalletPassword(const SecString& pass) override
		{
			// TODO: should be investigated, don't know how to "move" SecString into lambda
			std::string passStr(pass.data(), pass.size());

			tx.send([passStr](BridgeInterface& receiver_) mutable
			{
				receiver_.changeWalletPassword(passStr);
			});
		}
	};

	//////////////
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
		WalletModel model;
		model.keychain = keychain;
		model.keystore = keystore;
		wallets.push_back(model);

		jclass Wallet = env->FindClass(BEAM_JAVA_PATH "/entities/Wallet");
		jobject walletObj = env->AllocObject(Wallet);

		setLongField(env, Wallet, walletObj, "_this", wallets.size() - 1);

		return walletObj;
	}

	WalletModel& getWallet(JNIEnv *env, jobject thiz)
	{
		jclass Wallet = env->FindClass(BEAM_JAVA_PATH "/entities/Wallet");
		jfieldID _this = env->GetFieldID(Wallet, "_this", "J");
		return wallets[env->GetLongField(thiz, _this)];
	}

	void initLogger(const string& appData)
	{
		static auto logger = beam::Logger::create(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, "wallet_", (fs::path(appData) / fs::path("logs")).string());

		Rules::get().UpdateChecksum();
		LOG_INFO() << "Rules signature: " << Rules::get().Checksum;
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
		setIntField(env, TxDescription, txObj, "status", static_cast<jint>(tx.m_status));

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

	auto& model = getWallet(env, thiz);

    model.initObserver(env, listener);

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

	auto wallet_io = make_shared<WalletNetworkIO>(node_addr, model.keychain, model.keystore, reactor);

	Wallet wallet{ model.keychain, wallet_io};

	model.async = make_shared<WalletModelBridge>(*(static_cast<IWalletModelAsync*>(&model)), *reactor);

	wallet.subscribe(model.observer());

	wallet_io->start();
}

#ifdef __cplusplus
}
#endif