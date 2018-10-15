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

	//////////////
	inline void setByteField(JNIEnv *env, jclass clazz, jobject obj, const char* name, jbyte value)
	{
		env->SetByteField(obj, env->GetFieldID(clazz, name, "B"), value);
	}

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

        virtual void test() = 0;

		virtual ~IWalletModelAsync() {}
	};

	struct WalletStatus
	{
		beam::Amount available;
		beam::Amount received;
		beam::Amount sent;
		beam::Amount unconfirmed;
		struct
		{
			beam::Timestamp lastTime;
			int done;
			int total;
		} update;

		beam::Block::SystemState::ID stateID;
	};

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

        void test() override
        {
            tx.send([](BridgeInterface& receiver_) mutable
            {
                receiver_.test();
            });
        }
	};
	
	struct WalletModel : IWalletModelAsync, IWalletObserver
	{
		WalletModel(JNIEnv *env)
			: _threadEnv(nullptr)
		{
			env->GetJavaVM(&_jvm);
			_startMutex = make_shared<mutex>();
			_startCV = make_shared<condition_variable>();
		}

		void start(const string& nodeAddr, IKeyChain::Ptr keychain, IKeyStore::Ptr keystore)
		{
			_thread = make_shared<thread>(&WalletModel::run, this, nodeAddr, keychain, keystore);

			{
				unique_lock<mutex> lock(*_startMutex);
				_startCV->wait(lock);
			}
		}

		void run(const string& nodeURI, IKeyChain::Ptr keychain, IKeyStore::Ptr keystore)
		{
			_keychain = keychain;
			_keystore = keystore;

#if defined (__ANDROID__)
			_jvm->AttachCurrentThread(&_threadEnv, NULL);
#else
			_jvm->AttachCurrentThread((void**)&_threadEnv, NULL);
#endif

			LOG_DEBUG() << "run wallet...";

			io::Reactor::Ptr reactor = io::Reactor::create();
			io::Reactor::Scope scope(*reactor);

			io::Reactor::GracefulIntHandler gih(*reactor);

			io::Address node_addr;
			
			if (!node_addr.resolve(nodeURI.c_str()))
			{
				LOG_ERROR() << "unable to resolve node address: " << nodeURI.c_str();
				return;
			}

			auto wallet_io = make_shared<WalletNetworkIO>(node_addr, keychain, keystore, reactor);

			Wallet wallet{ keychain, wallet_io};

			async = make_shared<WalletModelBridge>(*(static_cast<IWalletModelAsync*>(this)), *reactor);

			wallet.subscribe(this);

			{
				unique_lock<mutex> lock(*_startMutex);
				_startCV->notify_one();
			}

			wallet_io->start();
		}

		///////////////////////////////////////////////
		// IWalletModelAsync impl
		///////////////////////////////////////////////
		void sendMoney(const beam::WalletID& sender, const beam::WalletID& receiver, beam::Amount&& amount, beam::Amount&& fee = 0) override {}
		void sendMoney(const beam::WalletID& receiver, const std::string& comment, beam::Amount&& amount, beam::Amount&& fee = 0) override {}
		void syncWithNode() override {}
		void calcChange(beam::Amount&& amount) override {}

		void getWalletStatus() override 
		{
			LOG_DEBUG() << "getWalletStatus()";
			onStatus(getStatus());

			onTxStatus(beam::ChangeAction::Reset, _keychain->getTxHistory());
			onTxPeerUpdated(_keychain->getPeers());
			onAdrresses(false, _keychain->getAddresses(false));
		}

		void getUtxosStatus() override 
		{
			LOG_DEBUG() << "getUtxosStatus()";
			onAllUtxoChanged(getUtxos());
		}

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

        void test() override
        {
            jclass WalletListener = _threadEnv->FindClass(BEAM_JAVA_PATH "/listeners/WalletListener");

            jmethodID callback = _threadEnv->GetStaticMethodID(WalletListener, "onTest", "()V");
            _threadEnv->CallStaticVoidMethod(WalletListener, callback);
        }

		///////////////////////////////////////////////
		// callbacks
		///////////////////////////////////////////////

		void onStatus(const WalletStatus& status)
		{
			jclass WalletStatus = _threadEnv->FindClass(BEAM_JAVA_PATH "/entities/WalletStatus");
			jobject walletStatus = _threadEnv->AllocObject(WalletStatus);

			setLongField(_threadEnv, WalletStatus, walletStatus, "available", status.available);
			setLongField(_threadEnv, WalletStatus, walletStatus, "unconfirmed", status.unconfirmed);

			{
				jclass SystemState = _threadEnv->FindClass(BEAM_JAVA_PATH "/entities/SystemState");
				jobject systemState = _threadEnv->AllocObject(SystemState);

				setLongField(_threadEnv, SystemState, systemState, "height", status.stateID.m_Height);
				setByteArrayField(_threadEnv, SystemState, systemState, "hash", status.stateID.m_Hash);

                jfieldID systemStateID = _threadEnv->GetFieldID(WalletStatus, "system", "L" BEAM_JAVA_PATH "/entities/SystemState;");
				_threadEnv->SetObjectField(walletStatus, systemStateID, systemState);
			}

			////////////////

			jclass WalletListener = _threadEnv->FindClass(BEAM_JAVA_PATH "/listeners/WalletListener");

			jmethodID callback = _threadEnv->GetStaticMethodID(WalletListener, "onStatus", "(L" BEAM_JAVA_PATH "/entities/WalletStatus;)V");
			_threadEnv->CallStaticVoidMethod(WalletListener, callback, walletStatus);
		}

		void onTxStatus(beam::ChangeAction action, const std::vector<beam::TxDescription>& items) 
		{
			LOG_DEBUG() << "onTxStatus()";

			jclass WalletListener = _threadEnv->FindClass(BEAM_JAVA_PATH "/listeners/WalletListener");

			jmethodID callback = _threadEnv->GetStaticMethodID(WalletListener, "onTxStatus", "(I[L" BEAM_JAVA_PATH "/entities/TxDescription;)V");
			
			jclass TxDescription = _threadEnv->FindClass(BEAM_JAVA_PATH "/entities/TxDescription");
			jobjectArray txItems = 0;

			if(!items.empty())
			{
				txItems = _threadEnv->NewObjectArray(static_cast<jsize>(items.size()), TxDescription, NULL);

				for(int i = 0; i < items.size(); ++i)
				{
					const auto& item = items[i];

					jobject tx = _threadEnv->AllocObject(TxDescription);

					setByteArrayField(_threadEnv, 		TxDescription, tx, "id", item.m_txId);
					setLongField(_threadEnv, 			TxDescription, tx, "amount", item.m_amount);
					setLongField(_threadEnv, 			TxDescription, tx, "fee", item.m_fee);
					setLongField(_threadEnv, 			TxDescription, tx, "change", item.m_change);
					setLongField(_threadEnv, 			TxDescription, tx, "minHeight", item.m_minHeight);
					setByteArrayField(_threadEnv, 		TxDescription, tx, "peerId", item.m_peerId);
					setByteArrayField(_threadEnv, 		TxDescription, tx, "myId", item.m_myId);
					setByteArrayField(_threadEnv, 		TxDescription, tx, "message", item.m_message);
					setLongField(_threadEnv, 			TxDescription, tx, "createTime", item.m_createTime);
					setLongField(_threadEnv, 			TxDescription, tx, "modifyTime", item.m_modifyTime);
					setBooleanField(_threadEnv, 		TxDescription, tx, "sender", item.m_sender);
					setIntField(_threadEnv, 			TxDescription, tx, "status", static_cast<jint>(item.m_status));

					_threadEnv->SetObjectArrayElement(txItems, i, tx);
				}				
			}

			_threadEnv->CallStaticVoidMethod(WalletListener, callback, action, txItems);
		}

		void onTxPeerUpdated(const std::vector<beam::TxPeer>& peers) 
		{
			LOG_DEBUG() << "onTxPeerUpdated()";

			jclass WalletListener = _threadEnv->FindClass(BEAM_JAVA_PATH "/listeners/WalletListener");

			jmethodID callback = _threadEnv->GetStaticMethodID(WalletListener, "onTxPeerUpdated", "([L" BEAM_JAVA_PATH "/entities/TxPeer;)V");
			
			jclass TxPeer = _threadEnv->FindClass(BEAM_JAVA_PATH "/entities/TxPeer");
			jobjectArray peerItems = 0;

			if(!peers.empty())
			{
				peerItems = _threadEnv->NewObjectArray(static_cast<jsize>(peers.size()), TxPeer, NULL);

				for(int i = 0; i < peers.size(); ++i)
				{
					const auto& item = peers[i];

					jobject tx = _threadEnv->AllocObject(TxPeer);

					setByteArrayField(_threadEnv, TxPeer, tx, "walletID", item.m_walletID);

					{
						jfieldID fieldId = _threadEnv->GetFieldID(TxPeer, "label", "Ljava/lang/String;");
						_threadEnv->SetObjectField(tx, fieldId, _threadEnv->NewStringUTF(item.m_label.c_str()));
					}

					{
						jfieldID fieldId = _threadEnv->GetFieldID(TxPeer, "address", "Ljava/lang/String;");
                        _threadEnv->SetObjectField(tx, fieldId, _threadEnv->NewStringUTF(item.m_address.c_str()));
					}

					_threadEnv->SetObjectArrayElement(peerItems, i, tx);
				}				
			}

			_threadEnv->CallStaticVoidMethod(WalletListener, callback, peerItems);			
		}

		void onSyncProgressUpdated(int done, int total) {}
		void onChangeCalculated(beam::Amount change) {}

		void onAllUtxoChanged(const std::vector<beam::Coin>& utxosVec) 
		{
			jclass Utxo = _threadEnv->FindClass(BEAM_JAVA_PATH "/entities/Utxo");
			jobjectArray utxos = 0;

			if(!utxosVec.empty())
			{
				utxos = _threadEnv->NewObjectArray(static_cast<jsize>(utxosVec.size()), Utxo, NULL);

				for(int i = 0; i < utxosVec.size(); ++i)
				{
					const auto& coin = utxosVec[i];

					jobject utxo = _threadEnv->AllocObject(Utxo);

					setLongField(_threadEnv, Utxo, utxo, "id", coin.m_id);
					setLongField(_threadEnv, Utxo, utxo, "amount", coin.m_amount);
					setIntField(_threadEnv, Utxo, utxo, "status", coin.m_status);
					setLongField(_threadEnv, Utxo, utxo, "createHeight", coin.m_createHeight);
					setLongField(_threadEnv, Utxo, utxo, "maturity", coin.m_maturity);
					setIntField(_threadEnv, Utxo, utxo, "keyType", static_cast<jint>(coin.m_key_type));
					setLongField(_threadEnv, Utxo, utxo, "confirmHeight", coin.m_confirmHeight);
					setByteArrayField(_threadEnv, Utxo, utxo, "confirmHash", coin.m_confirmHash);
					setLongField(_threadEnv, Utxo, utxo, "lockHeight", coin.m_lockedHeight);

					if(coin.m_createTxId)
						setByteArrayField(_threadEnv, Utxo, utxo, "createTxId", *coin.m_createTxId);

					if (coin.m_spentTxId)
						setByteArrayField(_threadEnv, Utxo, utxo, "spentTxId", *coin.m_spentTxId);

					_threadEnv->SetObjectArrayElement(utxos, i, utxo);
				}
			}

			//////////////////////////////////

			jclass WalletListener = _threadEnv->FindClass(BEAM_JAVA_PATH "/listeners/WalletListener");

			jmethodID callback = _threadEnv->GetStaticMethodID(WalletListener, "onAllUtxoChanged", "([L" BEAM_JAVA_PATH "/entities/Utxo;)V");
			_threadEnv->CallStaticVoidMethod(WalletListener, callback, utxos);
		}

		void onAdrresses(bool own, const std::vector<beam::WalletAddress>& addresses) {}

		void onGeneratedNewWalletID(const beam::WalletID& walletID) {}
		void onChangeCurrentWalletIDs(beam::WalletID senderID, beam::WalletID receiverID) {}

		///////////////////////////////////////////////
		// IWalletObserver impl
		///////////////////////////////////////////////
		void onKeychainChanged() override 
		{
			onAllUtxoChanged(getUtxos());
			onStatus(getStatus());
		}

		void onTransactionChanged(beam::ChangeAction action, vector<beam::TxDescription>&& items) override
		{
			onTxStatus(action, move(items));
    		onStatus(getStatus());
		}

		void onSystemStateChanged() override
		{
			onStatus(getStatus());
		}

		void onTxPeerChanged() override
		{
			onTxPeerUpdated(_keychain->getPeers());
		}

		void onAddressChanged() override
		{
			onAdrresses(true, _keychain->getAddresses(true));
    		onAdrresses(false, _keychain->getAddresses(false));
		}

		void onSyncProgress(int done, int total) override
		{
			onSyncProgressUpdated(done, total);
		}

		WalletStatus getStatus() const
		{
			WalletStatus status{ wallet::getAvailable(_keychain), 0, 0, 0};

			auto history = _keychain->getTxHistory();

			for (const auto& item : history)
			{
			   switch (item.m_status)
			   {
			   case TxStatus::Completed:
				   (item.m_sender ? status.sent : status.received) += item.m_amount;
				   break;
			   default: break;
			   }
			}

			status.unconfirmed += wallet::getTotal(_keychain, Coin::Unconfirmed);

			status.update.lastTime = _keychain->getLastUpdateTime();
			ZeroObject(status.stateID);
			_keychain->getSystemStateID(status.stateID);

			return status;
		}

		vector<Coin> getUtxos() const
		{
			vector<Coin> utxos;
			_keychain->visit([&utxos](const Coin& c)->bool
			{
				utxos.push_back(c);
				return true;
			});
			return utxos;
		}

		IWalletModelAsync::Ptr async;
		
	private:

		JNIEnv* _threadEnv;
		JavaVM* _jvm;
		shared_ptr<thread> _thread;

		IKeyChain::Ptr _keychain;
		IKeyStore::Ptr _keystore;

		shared_ptr<mutex> _startMutex;
		shared_ptr<condition_variable> _startCV;
	};

	vector<WalletModel> wallets;

	jobject regWallet(JNIEnv *env, jobject thiz)
	{
		wallets.push_back({ env });

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
	jstring nodeAddrStr, jstring appDataStr, jstring passStr, jstring seed)
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

		jobject walletObj = regWallet(env, thiz);

		getWallet(env, walletObj).start(JString(env, nodeAddrStr).value(), wallet, keystore);

		return walletObj;
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
	jstring nodeAddrStr, jstring appDataStr, jstring passStr)
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

		jobject walletObj = regWallet(env, thiz);

		getWallet(env, walletObj).start(JString(env, nodeAddrStr).value(), wallet, keystore);

		return walletObj;
	}

	LOG_ERROR() << "wallet not opened.";

	return nullptr;
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(getWalletStatus)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "getWalletStatus()";

	getWallet(env, thiz).async->getWalletStatus();
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(getUtxosStatus)(JNIEnv *env, jobject thiz)
{
	LOG_DEBUG() << "getUtxosStatus()";

	getWallet(env, thiz).async->getUtxosStatus();
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(test)(JNIEnv *env, jobject thiz)
{
    LOG_DEBUG() << "test()";

    getWallet(env, thiz).async->test();
}

JNIEXPORT void JNICALL BEAM_JAVA_WALLET_INTERFACE(syncTest)(JNIEnv *env, jobject thiz)
{
    LOG_DEBUG() << "syncTest()";

    jclass WalletListener = env->FindClass(BEAM_JAVA_PATH "/listeners/WalletListener");

    jmethodID callback = env->GetStaticMethodID(WalletListener, "onSyncTest", "()V");
    env->CallStaticVoidMethod(WalletListener, callback);
}

#ifdef __cplusplus
}
#endif