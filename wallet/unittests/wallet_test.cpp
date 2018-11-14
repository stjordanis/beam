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

#ifndef LOG_VERBOSE_ENABLED
    #define LOG_VERBOSE_ENABLED 0
#endif

#include "wallet/wallet_network.h"
#include "wallet/wallet.h"
#include "wallet/secstring.h"
#include "utility/test_helpers.h"
#include "../../core/unittest/mini_blockchain.h"
#include <string_view>

#include "test_helpers.h"

#include <assert.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "core/proto.h"
#include <boost/filesystem.hpp>
#include <boost/intrusive/list.hpp>

using namespace beam;
using namespace std;
using namespace ECC;

WALLET_TEST_INIT

namespace
{
    class BaseTestWalletDB : public IWalletDB
    {
		Key::IKdf::Ptr m_pKdf;
    public:

		BaseTestWalletDB()
		{
			std::shared_ptr<HKdf> pKdf(new HKdf);
			pKdf->m_Secret.V = 10U;
			m_pKdf = pKdf;
		}


		Key::IKdf::Ptr get_Kdf() const override
		{
			return m_pKdf;
		}

        ECC::Scalar::Native calcKey(const Coin& c) const override
        {
			ECC::Scalar::Native sk;
			m_pKdf->DeriveKey(sk, c.get_Kidv());
            return sk;
        }

		void get_IdentityKey(ECC::Scalar::Native& sk) const override
		{
			sk = Zero;
		}

        std::vector<beam::Coin> selectCoins(const ECC::Amount& amount, bool /*lock*/) override
        {
            std::vector<beam::Coin> res;
            ECC::Amount t = 0;
            for (auto& c : m_coins)
            {
                t += c.m_amount;
                c.m_status = Coin::Locked;
                res.push_back(c);
                if (t >= amount)
                {
                    break;
                }
            }
            return res;
        }

        virtual std::vector<beam::Coin> getCoinsCreatedByTx(const TxID& txId) override { return {}; };
        void store(beam::Coin& ) override {}
        void store(std::vector<beam::Coin>& ) override {}
        void update(const vector<beam::Coin>& coins) override {}
        void update(const beam::Coin& ) override {}
        void remove(const std::vector<beam::Coin>& ) override {}
        void remove(const beam::Coin& ) override {}
        void visit(std::function<bool(const beam::Coin& coin)> ) override {}
        void setVarRaw(const char* , const void* , size_t ) override {}
        int getVarRaw(const char* , void* ) const override { return 0; }
        bool getBlob(const char* name, ByteBuffer& var) const override { return false; }
        Timestamp getLastUpdateTime() const override { return 0; }
        void setSystemStateID(const Block::SystemState::ID& ) override {};
        bool getSystemStateID(Block::SystemState::ID& ) const override { return false; };

        void subscribe(IWalletDbObserver* observer) override {}
        void unsubscribe(IWalletDbObserver* observer) override {}

        std::vector<TxDescription> getTxHistory(uint64_t , int ) override { return {}; };
        boost::optional<TxDescription> getTx(const TxID& ) override { return boost::optional<TxDescription>{}; };
        void saveTx(const TxDescription &) override {};
        void deleteTx(const TxID& ) override {};
        void rollbackTx(const TxID&) override {}

        std::vector<TxPeer> getPeers() override { return {}; };
        void addPeer(const TxPeer&) override {}
        boost::optional<TxPeer> getPeer(const WalletID&) override { return boost::optional<TxPeer>{}; }
        void clearPeers() override {}

        std::vector<WalletAddress> getAddresses(bool own) override { return {}; }
        void saveAddress(const WalletAddress&) override {}
        boost::optional<WalletAddress> getAddress(const WalletID& id) override
        {
            return boost::optional<WalletAddress>{};
        }
        void deleteAddress(const WalletID&) override {}

        Height getCurrentHeight() const override
        {
            return 134;
        }

        uint64_t getKnownStateCount() const override
        {
            return 0;
        }

        Block::SystemState::ID getKnownStateID(Height height) override
        {
            return {};
        }

        void rollbackConfirmedUtxo(Height /*minHeight*/) override
        {}

        void clear() override {}

		void changePassword(const SecString& password) override {}

        bool setTxParameter(const TxID& txID, wallet::TxParameterID paramID, const ByteBuffer& blob) override
        {
            if (paramID < wallet::TxParameterID::PrivateFirstParam)
            {
                auto p = m_params.emplace(paramID, blob);
                return p.second;
            }
            m_params[paramID] = blob;
            return true;
        }
        bool getTxParameter(const TxID& txID, wallet::TxParameterID paramID, ByteBuffer& blob) override
        {
            auto it = m_params.find(paramID);
            if (it != m_params.end())
            {
                blob = it->second;
                return true;
            }
            return false;
        }

    protected:
        std::vector<beam::Coin> m_coins;
        std::map<wallet::TxParameterID, ByteBuffer> m_params;
    };

    class TestWalletDB : public BaseTestWalletDB
    {
    public:
        TestWalletDB()
        {
            m_coins.emplace_back(5);
            m_coins.emplace_back(2);
            m_coins.emplace_back(3);
        }
    };

    class TestWalletDB2 : public BaseTestWalletDB
    {
    public:
        TestWalletDB2()
        {
            m_coins.emplace_back(1);
            m_coins.emplace_back(3);
        }
    };

    template<typename T>
    IWalletDB::Ptr CreateWalletDB()
    {
        return std::static_pointer_cast<IWalletDB>(std::make_shared<T>());
    }

    IKeyStore::Ptr CreateBbsKeystore(const string& path, const string& pass, WalletID& outWalletID)
    {
        boost::filesystem::remove_all(path);
        IKeyStore::Options options;
        options.flags = IKeyStore::Options::local_file | IKeyStore::Options::enable_all_keys;
        options.fileName = path;

        auto ks = IKeyStore::create(options, pass.c_str(), pass.size());
        //WalletID senderID = {};
        ks->gen_keypair(outWalletID);
        ks->save_keypair(outWalletID, true);
        return ks;
    }

    IWalletDB::Ptr createSqliteWalletDB(const string& path)
    {
        ECC::NoLeak<ECC::uintBig> seed;
        seed.V = Zero;
        if (boost::filesystem::exists(path))
        {
            boost::filesystem::remove(path);
        }
               
        auto walletDB = WalletDB::init(path, string("pass123"), seed);
        beam::Block::SystemState::ID id = {};
        id.m_Height = 134;
        walletDB->setSystemStateID(id);
        return walletDB;
    }

    IWalletDB::Ptr createSenderWalletDB()
    {
        auto db = createSqliteWalletDB("sender_wallet.db");
        for (auto amount : { 5, 2, 1, 9 })
        {
            Coin coin(amount);
            coin.m_maturity = 0;
            db->store(coin);
        }
        return db;
    }

    IWalletDB::Ptr createReceiverWalletDB()
    {
        return createSqliteWalletDB("receiver_wallet.db");
    }

    struct TestGateway : wallet::INegotiatorGateway
    {
        void on_tx_completed(const TxID&) override
        {
            cout << __FUNCTION__ << "\n";
        }

        void register_tx(const TxID& , Transaction::Ptr) override
        {
            cout << "sent tx registration request\n";
        }

        void confirm_outputs(const vector<Coin>&) override
        {
            cout << "confirm outputs\n";
        }

        void confirm_kernel(const TxID&, const TxKernel&) override
        {
            cout << "confirm kernel\n";
        }

        bool get_tip(Block::SystemState::Full& state) const override
        {
            return true;
        }

        bool isTestMode() const override
        {
            return true;
        }
    };

    struct IOLoop
    {
        using Task = function<void()>;
        IOLoop()
          : m_shutdown{ false }
          , m_uses{0}
        {

        }


        void addRef()
        {
            ++m_uses;
            if (m_uses == 1)
            {
                m_shutdown.store(false);
                m_cv.notify_all();
            }
        }

        void release()
        {
            if (--m_uses == 0)
            {
                shutdown();
            }
        }

        void shutdown()
        {
            m_shutdown.store(true);
            m_cv.notify_all();
        }

        bool isShutdown() const
        {
            return m_shutdown.load();
        }

        void run()
        {
            while (!isShutdown())
            {
                Task task;
                {
                    unique_lock<mutex> lock(m_tasksMutex);
                    if (!m_tasks.empty())
                    {
                        task = move(m_tasks.front());
                        m_tasks.pop_front();
                    }
                    else
                    {
                        m_cv.wait(lock, [this] { return !m_tasks.empty() || isShutdown(); });
                    }
                }
                if (task)
                {
                    try
                    {
                        task();
                    }
                    catch (...)
                    {

                    }
                }
            }
        }

        void step()
        {
            deque<Task> tasks;
            {
                unique_lock<mutex> lock(m_tasksMutex);
                if (!m_tasks.empty())
                {
                    tasks.swap(m_tasks);
                }
            }
            for (auto& task : tasks)
            {
                try
                {
                    task();
                }
                catch (...)
                {

                }
            }
        }

        void enqueueTask(Task&& task)
        {
            lock_guard<mutex> lock{ m_tasksMutex };
            m_tasks.push_back(move(task));
            m_cv.notify_one();
        }

        deque<Task> m_tasks;
        mutex m_tasksMutex;
        condition_variable m_cv;
        atomic<bool> m_shutdown;
        atomic<int> m_uses;
    };

    //
    // Test impl of the network io. The second thread isn't really needed, though this is much close to reality
    //
    struct TestNetworkBase : public NetworkIOBase
    {
        using Task = function<void()>;
        TestNetworkBase(IOLoop& mainLoop)
            : m_peerCount{0}
            , m_mainLoop(mainLoop)
            , m_thread{ [this] { m_networkLoop.run(); } }
        {
            mainLoop.addRef();
        }

        void shutdown()
        {
            m_networkLoop.shutdown();
            m_mainLoop.enqueueTask([this]()
            {
                m_thread.join();
                m_mainLoop.release();
            });
        }

        void registerPeer(IWallet* walletPeer, bool main)
        {
            m_peers.push_back(walletPeer);

            if (main)
            {
                proto::NewTip msg;
                InitHdr(msg);
                walletPeer->handle_node_message(move(msg));
            }
        }

        virtual void InitHdr(proto::NewTip& msg)
        {
            msg.m_Description.m_Height = 134;
        }

        void enqueueNetworkTask(Task&& task)
        {
            m_networkLoop.enqueueTask([this, t = std::move(task)]()
            {
                m_mainLoop.enqueueTask([t = std::move(t)](){t(); });
            });
        }

        template<typename Msg>
        void send(size_t peerId, const WalletID& to, Msg&& msg)
        {
            Serializer s;
            s & msg;
            ByteBuffer buf;
            s.swap_buf(buf);
            enqueueNetworkTask([this, peerId, to, buf = move(buf)]()
            {
                Deserializer d;
                d.reset(&buf[0], buf.size());
                Msg msg;
                d & msg;
                m_peers[peerId]->handle_tx_message(to, move(msg));
            });
        }

        virtual void onSent()
        {

        }

        void set_node_address(io::Address nodeAddress) override {}
        void subscribe(INetworkIOObserver* observer) override {}
        void unsubscribe(INetworkIOObserver* observer) override {}

        int m_peerCount;

        vector<IWallet*> m_peers;
        IOLoop m_networkLoop;
        IOLoop& m_mainLoop;
        thread m_thread;
    };

    struct TestNetwork : public TestNetworkBase
    {
        TestNetwork(IOLoop& mainLoop) : TestNetworkBase{ mainLoop }
        {}

        void send_tx_message(const WalletID& to, beam::wallet::SetTxParameter&& data) override
        {
            cout << "SetTxParameter\n";
            send(1, to, move(data));
        }

        void send_node_message(proto::NewTransaction&& data) override
        {
            cout << "NewTransaction\n";

            for (const auto& input : data.m_Transaction->m_vInputs)
            {
                RemoveCommitment(input->m_Commitment);
            }
            for (const auto& output : data.m_Transaction->m_vOutputs)
            {
                AddCommitment(output->m_Commitment);
            }

            enqueueNetworkTask([this, data] {m_peers[0]->handle_node_message(proto::Boolean{ true }); });
        }

        set<ECC::Point> m_Commitments;

        void AddCommitment(const ECC::Point& c)
        {
            m_Commitments.insert(c);
        }

        void RemoveCommitment(const ECC::Point& c)
        {
            m_Commitments.erase(c);
        }

        bool HasCommitment(const ECC::Point& c)
        {
            return m_Commitments.find(c) != m_Commitments.end();
        }

        void send_node_message(proto::GetMined&& data) override
        {
            cout << "GetMined\n";
            enqueueNetworkTask([this] {m_peers[0]->handle_node_message(proto::Mined{ }); });
        }

        void send_node_message(proto::Recover&& data) override
        {
            cout << "Recover\n";
            enqueueNetworkTask([this] {m_peers[0]->handle_node_message(proto::Recovered{ }); });
        }

        void send_node_message(proto::GetProofUtxo&& msg) override
        {
            cout << "GetProofUtxo\n";

            Input::Proof proof = {};
            if (HasCommitment(msg.m_Utxo))
            {
                proof.m_State.m_Maturity = 134 + 60;
                enqueueNetworkTask([this, proof = move(proof)]() {m_peers[0]->handle_node_message(proto::ProofUtxo({ proof })); });
            }
            else
            {
                enqueueNetworkTask([this, proof = move(proof)]() {m_peers[0]->handle_node_message(proto::ProofUtxo()); });
            }
        }

        void send_node_message(proto::GetHdr&&) override
        {
            cout << "GetHdr request chain header\n";
        }

        void send_node_message(beam::proto::GetProofState &&) override
        {
            cout << "GetProofState\n";

            proto::ProofState msg;

            enqueueNetworkTask([this, msg] {
                m_peers[0]->handle_node_message((proto::ProofState&&) msg);
            });
        }

        void send_node_message(beam::proto::GetProofKernel&&) override
        {
            cout << "GetProofKernel\n";
            enqueueNetworkTask([this] {
                m_peers[0]->handle_node_message(proto::ProofKernel{});
            });
        }

        void connect_node() override
        {

        }

        void close_node_connection() override
        {
        }

        void new_own_address(const WalletID&) override
        {

        }

        void address_deleted(const WalletID& address) override
        {

        }
    };
}

class TestWallet
    :public Wallet
{
    bool IsTestMode() const override { return true; }
public:
    TestWallet(IWalletDB::Ptr walletDB, INetworkIO::Ptr network, bool holdNodeConnection = false, TxCompletedAction&& action = TxCompletedAction())
        :Wallet(walletDB, network, holdNodeConnection, std::move(action))
    {
    }
};

struct TestWalletRig
{
    TestWalletRig(const string& name, IWalletDB::Ptr walletDB, io::Reactor::Ptr reactor, Wallet::TxCompletedAction&& action = Wallet::TxCompletedAction())
        : m_NodeAddress{ io::Address::localhost().port(32125) }
        , m_WalletDB{walletDB}
        , m_BBSKeystore{ CreateBbsKeystore(name + "-bbs", "123", m_WalletID) }
        , m_WalletIO{make_shared<WalletNetworkIO>(m_NodeAddress, m_WalletDB, m_BBSKeystore, reactor, 1000, 2000)}
        , m_Wallet{m_WalletDB, m_WalletIO, true, move(action) }
    {
        static_pointer_cast<INetworkIO>(m_WalletIO)->connect_node();
    }

    vector<Coin> GetCoins()
    {
        vector<Coin> coins;
        m_WalletDB->visit([&coins](const Coin& c)->bool
        {
            coins.push_back(c);
            return true;
        });
        return coins;
    }

    io::Address m_NodeAddress;
    WalletID m_WalletID;
    IWalletDB::Ptr m_WalletDB;
    IKeyStore::Ptr m_BBSKeystore;
    shared_ptr<WalletNetworkIO> m_WalletIO;
    int m_CompletedCount{1};
    TestWallet m_Wallet;
};

void TestWalletNegotiation(IWalletDB::Ptr senderDB, IWalletDB::Ptr receiverDB)
{
    cout << "\nTesting wallets negotiation...\n";

    WalletID receiver_id = {};
    receiver_id = unsigned(4);

    WalletID sender_id = {};
    sender_id = unsigned(5);

    IOLoop mainLoop;
    auto network = make_shared<TestNetwork >(mainLoop);
    auto network2 = make_shared<TestNetwork>(mainLoop);

    int count = 0;
    auto f = [&count, network, network2](const auto& /*id*/)
    {
        if (++count >= 2)
        {
            network->shutdown();
            network2->shutdown();
        }
    };

    TestWallet sender(senderDB, network, false, f);
    TestWallet receiver(receiverDB, network2, false, f);

    network->registerPeer(&sender, true);
    network->registerPeer(&receiver, false);

    network2->registerPeer(&receiver, true);
    network2->registerPeer(&sender, false);

    sender.transfer_money(sender_id, receiver_id, 6, 1, true, {});
    mainLoop.run();
}

class TestNode
{
public:
    TestNode(io::Address address)
    {
        m_Server.Listen(address);
    }

    ~TestNode() {
        KillAll();
    }

    void KillAll()
    {
        while (!m_lstClients.empty())
            DeleteClient(&m_lstClients.front());
    }

private:

    struct Client
        :public proto::NodeConnection
        ,public boost::intrusive::list_base_hook<>
    {
        TestNode& m_This;
        bool m_Subscribed;

        Client(TestNode& n)
            : m_This(n)
            , m_Subscribed(false)
        {
        }


        // protocol handler
        void OnMsg(proto::NewTransaction&& data) override
        {
            for (const auto& input : data.m_Transaction->m_vInputs)
            {
                m_This.RemoveCommitment(input->m_Commitment);
            }
            for (const auto& output : data.m_Transaction->m_vOutputs)
            {
                m_This.AddCommitment(output->m_Commitment);
            }
            Send(proto::Boolean{ true });
        }

        void OnMsg(proto::GetProofUtxo&& data) override
        {
            if (m_This.HasCommitment(data.m_Utxo))
            {
                Input::Proof proof = {};
                proof.m_State.m_Maturity = 134 + 60;
                Send(proto::ProofUtxo{ {proof} });
            }
            else
            {
                Send(proto::ProofUtxo{});
            }
        }

        void OnMsg(proto::GetProofKernel&& /*data*/) override
        {
            Send(proto::ProofKernel());
        }

        void OnMsg(proto::Config&& /*data*/) override
        {
            proto::NewTip msg;

            msg.m_Description.m_Height = 134;
            Send(move(msg));
        }

        void OnMsg(proto::GetMined&&) override
        {
            Send(proto::Mined{});
        }

        void OnMsg(proto::Recover&&) override
        {
            Send(proto::Recovered{});
        }

        void OnMsg(proto::GetProofState&&) override
        {
            Send(proto::ProofState{});
        }

        void OnMsg(proto::BbsSubscribe&& msg) override
        {
            if (m_Subscribed)
                return;
            m_Subscribed = true;

            for (const auto& m : m_This.m_bbs)
            {
                Send(m);
            }
        }

        void OnMsg(proto::BbsMsg&& msg) override
        {
            m_This.m_bbs.push_back(msg);

            for (ClientList::iterator it = m_This.m_lstClients.begin(); m_This.m_lstClients.end() != it; it++)
            {
                if (it.pointed_node() != this)
                {
                    Client& c = *it;
                    if (c.m_Subscribed)
                    {
                        c.Send(msg);
                    }
                }
            }
        }

        void OnDisconnect(const DisconnectReason& r) override
        {
            switch (r.m_Type)
            {
            case DisconnectReason::Protocol:
            case DisconnectReason::ProcessingExc:
                LOG_ERROR() << "Disconnect: " << r;
                g_failureCount++;

            default: // suppress warning
                break;
            }

            m_This.DeleteClient(this);
        }
    };

    typedef boost::intrusive::list<Client> ClientList;
    ClientList m_lstClients;

    std::vector<proto::BbsMsg> m_bbs;

    void DeleteClient(Client* client)
    {
        m_lstClients.erase(ClientList::s_iterator_to(*client));
        delete client;
    }

    set<ECC::Point> m_Commitments;

    void AddCommitment(const ECC::Point& c)
    {
        m_Commitments.insert(c);
    }

    void RemoveCommitment(const ECC::Point& c)
    {
        m_Commitments.erase(c);
    }

    bool HasCommitment(const ECC::Point& c)
    {
        return m_Commitments.find(c) != m_Commitments.end();
    }

    struct Server
        :public proto::NodeConnection::Server
    {
        IMPLEMENT_GET_PARENT_OBJ(TestNode, m_Server)

        void OnAccepted(io::TcpStream::Ptr&& newStream, int errorCode) override
        {
            if (newStream)
            {
                Client* p = new Client(get_ParentObj());
                get_ParentObj().m_lstClients.push_back(*p);

                p->Accept(std::move(newStream));
                p->SecureConnect();
            }
        }
    } m_Server;
};

void TestTxToHimself()
{
    cout << "\nTesting Tx to himself...\n";

    io::Reactor::Ptr mainReactor{ io::Reactor::create() };
    io::Reactor::Scope scope(*mainReactor);

    string keystorePass = "123";
    WalletID senderID = {};
    auto senderBbsKeys = CreateBbsKeystore("sender-bbs", keystorePass, senderID);

    WalletID receiverID = {};
    senderBbsKeys->gen_keypair(receiverID);

    auto senderDB = createSqliteWalletDB("sender_wallet.db");

    // add own address
    WalletAddress own_address = {};
    own_address.m_walletID = receiverID;
    own_address.m_label = "test label";
    own_address.m_category = "test category";
    own_address.m_createTime = beam::getTimestamp();
    own_address.m_duration = 23;
    own_address.m_own = true;

    senderDB->saveAddress(own_address);

    // add coin with keyType - Coinbase
    beam::Amount coin_amount = 40;
    Coin coin(coin_amount);
    coin.m_maturity = 0;
    coin.m_status = Coin::Unspent;
    coin.m_key_type = Key::Type::Coinbase;
    senderDB->store(coin);

    auto coins = senderDB->selectCoins(24, false);
    WALLET_CHECK(coins.size() == 1);
    WALLET_CHECK(coins[0].m_key_type == Key::Type::Coinbase);
    WALLET_CHECK(coins[0].m_status == Coin::Unspent);
    WALLET_CHECK(senderDB->getTxHistory().empty());

    auto nodeAddress = io::Address::localhost().port(32125);
    TestNode node{ nodeAddress };
    auto sender_io = make_shared<WalletNetworkIO>(nodeAddress, senderDB, senderBbsKeys, mainReactor, 1000, 2000);
    TestWallet sender{ senderDB, sender_io, true, [sender_io](auto) { sender_io->stop(); } };
    helpers::StopWatch sw;

    static_pointer_cast<INetworkIO>(sender_io)->connect_node();

    sw.start();
    TxID txId = sender.transfer_money(senderID, receiverID, 24, 2);
    mainReactor->run();
    sw.stop();

    cout << "Transfer elapsed time: " << sw.milliseconds() << " ms\n";

    // check Tx
    auto txHistory = senderDB->getTxHistory();
    WALLET_CHECK(txHistory.size() == 1);
    WALLET_CHECK(txHistory[0].m_txId == txId);
    WALLET_CHECK(txHistory[0].m_amount == 24);
    WALLET_CHECK(txHistory[0].m_change == 14);
    WALLET_CHECK(txHistory[0].m_fee == 2);
    WALLET_CHECK(txHistory[0].m_status == TxStatus::Completed);

    // check coins
    vector<Coin> newSenderCoins;
    senderDB->visit([&newSenderCoins](const Coin& c)->bool
    {
        newSenderCoins.push_back(c);
        return true;
    });

    WALLET_CHECK(newSenderCoins.size() == 3);
    WALLET_CHECK(newSenderCoins[0].m_key_type == Key::Type::Coinbase);
    WALLET_CHECK(newSenderCoins[0].m_status == Coin::Spent);

    WALLET_CHECK(newSenderCoins[1].m_key_type == Key::Type::Regular);
    WALLET_CHECK(newSenderCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[1].m_amount == 14);

    WALLET_CHECK(newSenderCoins[2].m_key_type == Key::Type::Regular);
    WALLET_CHECK(newSenderCoins[2].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[2].m_amount == 24);

    cout << "\nFinish of testing Tx to himself...\n";
}

void TestP2PWalletNegotiationST()
{
    cout << "\nTesting p2p wallets negotiation single thread...\n";

    auto nodeAddress = io::Address::localhost().port(32125);

    io::Reactor::Ptr mainReactor{ io::Reactor::create() };
    io::Reactor::Scope scope(*mainReactor);

    int completedCount = 2;
    auto f = [&completedCount, mainReactor](auto)
    {
        --completedCount;
        if (completedCount == 0)
        {
            mainReactor->stop();
            completedCount = 2;
        }
    };

    TestWalletRig sender("sender", createSenderWalletDB(), mainReactor, f);
    TestWalletRig receiver("receiver", createReceiverWalletDB(), mainReactor, f);

    WALLET_CHECK(sender.m_WalletDB->selectCoins(6, false).size() == 2);
    WALLET_CHECK(sender.m_WalletDB->getTxHistory().empty());
    WALLET_CHECK(receiver.m_WalletDB->getTxHistory().empty());

    helpers::StopWatch sw;
    TestNode node{ nodeAddress };
    sw.start();

    TxID txId = sender.m_Wallet.transfer_money(sender.m_WalletID, receiver.m_WalletID, 4, 2);

    mainReactor->run();
    sw.stop();
    cout << "First transfer elapsed time: " << sw.milliseconds() << " ms\n";

    // check coins
    vector<Coin> newSenderCoins = sender.GetCoins();
    vector<Coin> newReceiverCoins = receiver.GetCoins();

    WALLET_CHECK(newSenderCoins.size() == 4);
    WALLET_CHECK(newReceiverCoins.size() == 1);
    WALLET_CHECK(newReceiverCoins[0].m_amount == 4);
    WALLET_CHECK(newReceiverCoins[0].m_status == Coin::Unspent);
    WALLET_CHECK(newReceiverCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[0].m_amount == 5);
    WALLET_CHECK(newSenderCoins[0].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[1].m_amount == 2);
    WALLET_CHECK(newSenderCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[1].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[2].m_amount == 1);
    WALLET_CHECK(newSenderCoins[2].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[2].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[3].m_amount == 9);
    WALLET_CHECK(newSenderCoins[3].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[3].m_key_type == Key::Type::Regular);

    // Tx history check
    auto sh = sender.m_WalletDB->getTxHistory();
    WALLET_CHECK(sh.size() == 1);
    auto rh = receiver.m_WalletDB->getTxHistory();
    WALLET_CHECK(rh.size() == 1);
    auto stx = sender.m_WalletDB->getTx(txId);
    WALLET_CHECK(stx.is_initialized());
    auto rtx = receiver.m_WalletDB->getTx(txId);
    WALLET_CHECK(rtx.is_initialized());

    WALLET_CHECK(stx->m_txId == rtx->m_txId);
    WALLET_CHECK(stx->m_amount == rtx->m_amount);
    WALLET_CHECK(stx->m_status == TxStatus::Completed);
    WALLET_CHECK(stx->m_fee == rtx->m_fee);
    WALLET_CHECK(stx->m_message == rtx->m_message);
    WALLET_CHECK(stx->m_createTime <= rtx->m_createTime);
    WALLET_CHECK(stx->m_status == rtx->m_status);
    WALLET_CHECK(stx->m_fsmState.empty());
    WALLET_CHECK(rtx->m_fsmState.empty());
    WALLET_CHECK(stx->m_sender == true);
    WALLET_CHECK(rtx->m_sender == false);

    // second transfer
    sw.start();
    txId = sender.m_Wallet.transfer_money(sender.m_WalletID, receiver.m_WalletID, 6);
    mainReactor->run();
    sw.stop();
    cout << "Second transfer elapsed time: " << sw.milliseconds() << " ms\n";

    // check coins
    newSenderCoins = sender.GetCoins();
    newReceiverCoins = receiver.GetCoins();

    WALLET_CHECK(newSenderCoins.size() == 5);
    WALLET_CHECK(newReceiverCoins.size() == 2);

    WALLET_CHECK(newReceiverCoins[0].m_amount == 4);
    WALLET_CHECK(newReceiverCoins[0].m_status == Coin::Unspent);
    WALLET_CHECK(newReceiverCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newReceiverCoins[1].m_amount == 6);
    WALLET_CHECK(newReceiverCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newReceiverCoins[1].m_key_type == Key::Type::Regular);


    WALLET_CHECK(newSenderCoins[0].m_amount == 5);
    WALLET_CHECK(newSenderCoins[0].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[1].m_amount == 2);
    WALLET_CHECK(newSenderCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[1].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[2].m_amount == 1);
    WALLET_CHECK(newSenderCoins[2].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[2].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[3].m_amount == 9);
    WALLET_CHECK(newSenderCoins[3].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[3].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[4].m_amount == 3);
    WALLET_CHECK(newSenderCoins[4].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[4].m_key_type == Key::Type::Regular);

    // Tx history check
    sh = sender.m_WalletDB->getTxHistory();
    WALLET_CHECK(sh.size() == 2);
    rh = receiver.m_WalletDB->getTxHistory();
    WALLET_CHECK(rh.size() == 2);
    stx = sender.m_WalletDB->getTx(txId);
    WALLET_CHECK(stx.is_initialized());
    rtx = receiver.m_WalletDB->getTx(txId);
    WALLET_CHECK(rtx.is_initialized());

    WALLET_CHECK(stx->m_txId == rtx->m_txId);
    WALLET_CHECK(stx->m_amount == rtx->m_amount);
    WALLET_CHECK(stx->m_status == TxStatus::Completed);
    WALLET_CHECK(stx->m_message == rtx->m_message);
    WALLET_CHECK(stx->m_createTime <= rtx->m_createTime);
    WALLET_CHECK(stx->m_status == rtx->m_status);
    WALLET_CHECK(stx->m_fsmState.empty());
    WALLET_CHECK(rtx->m_fsmState.empty());
    WALLET_CHECK(stx->m_sender == true);
    WALLET_CHECK(rtx->m_sender == false);


    // third transfer. no enough money should appear
    sw.start();
    completedCount = 1;// only one wallet takes part in tx
    txId = sender.m_Wallet.transfer_money(sender.m_WalletID, receiver.m_WalletID, 6);
    mainReactor->run();
    sw.stop();
    cout << "Third transfer elapsed time: " << sw.milliseconds() << " ms\n";
    
    // check coins
    newSenderCoins = sender.GetCoins();
    newReceiverCoins = receiver.GetCoins();

    // no coins 
    WALLET_CHECK(newSenderCoins.size() == 5);
    WALLET_CHECK(newReceiverCoins.size() == 2);

    // Tx history check. New failed tx should be added to sender
    sh = sender.m_WalletDB->getTxHistory();
    WALLET_CHECK(sh.size() == 3);
    rh = receiver.m_WalletDB->getTxHistory();
    WALLET_CHECK(rh.size() == 2);
    stx = sender.m_WalletDB->getTx(txId);
    WALLET_CHECK(stx.is_initialized());
    rtx = receiver.m_WalletDB->getTx(txId);
    WALLET_CHECK(!rtx.is_initialized());

    WALLET_CHECK(stx->m_amount == 6);
    WALLET_CHECK(stx->m_status == TxStatus::Failed);
    WALLET_CHECK(stx->m_fsmState.empty());
    WALLET_CHECK(stx->m_sender == true);
}

void TestP2PWalletReverseNegotiationST()
{
    cout << "\nTesting p2p wallets negotiation (reverse version)...\n";

    auto nodeAddress = io::Address::localhost().port(32125);

    io::Reactor::Ptr mainReactor{ io::Reactor::create() };
    io::Reactor::Scope scope(*mainReactor);

    int completedCount = 2;
    auto f = [&completedCount, mainReactor](auto)
    {
        --completedCount;
        if (completedCount == 0)
        {
            mainReactor->stop();
            completedCount = 2;
        }
    };

    TestWalletRig sender("sender", createSenderWalletDB(), mainReactor, f);
    TestWalletRig receiver("receiver", createReceiverWalletDB(), mainReactor, f);
  
    WALLET_CHECK(sender.m_WalletDB->selectCoins(6, false).size() == 2);
    WALLET_CHECK(sender.m_WalletDB->getTxHistory().empty());
    WALLET_CHECK(receiver.m_WalletDB->getTxHistory().empty());

    helpers::StopWatch sw;
    sw.start();
    TestNode node{ nodeAddress };

    TxID txId = receiver.m_Wallet.transfer_money(receiver.m_WalletID, sender.m_WalletID, 4, 2, false);

    mainReactor->run();
    sw.stop();
    cout << "First transfer elapsed time: " << sw.milliseconds() << " ms\n";

    // check coins
    vector<Coin> newSenderCoins = sender.GetCoins();
    vector<Coin> newReceiverCoins = receiver.GetCoins();

    WALLET_CHECK(newSenderCoins.size() == 4);
    WALLET_CHECK(newReceiverCoins.size() == 1);
    WALLET_CHECK(newReceiverCoins[0].m_amount == 4);
    WALLET_CHECK(newReceiverCoins[0].m_status == Coin::Unspent);
    WALLET_CHECK(newReceiverCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[0].m_amount == 5);
    WALLET_CHECK(newSenderCoins[0].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[1].m_amount == 2);
    WALLET_CHECK(newSenderCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[1].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[2].m_amount == 1);
    WALLET_CHECK(newSenderCoins[2].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[2].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[3].m_amount == 9);
    WALLET_CHECK(newSenderCoins[3].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[3].m_key_type == Key::Type::Regular);

    // Tx history check
    auto sh = sender.m_WalletDB->getTxHistory();
    WALLET_CHECK(sh.size() == 1);
    auto rh = receiver.m_WalletDB->getTxHistory();
    WALLET_CHECK(rh.size() == 1);
    auto stx = sender.m_WalletDB->getTx(txId);
    WALLET_CHECK(stx.is_initialized());
    auto rtx = receiver.m_WalletDB->getTx(txId);
    WALLET_CHECK(rtx.is_initialized());

    WALLET_CHECK(stx->m_txId == rtx->m_txId);
    WALLET_CHECK(stx->m_amount == rtx->m_amount);
    WALLET_CHECK(stx->m_status == TxStatus::Completed);
    WALLET_CHECK(stx->m_fee == rtx->m_fee);
    WALLET_CHECK(stx->m_message == rtx->m_message);
    WALLET_CHECK(stx->m_createTime >= rtx->m_createTime);
    WALLET_CHECK(stx->m_status == rtx->m_status);
    WALLET_CHECK(stx->m_fsmState.empty());
    WALLET_CHECK(rtx->m_fsmState.empty());
    WALLET_CHECK(stx->m_sender == true);
    WALLET_CHECK(rtx->m_sender == false);

    // second transfer
    sw.start();
    txId = receiver.m_Wallet.transfer_money(receiver.m_WalletID, sender.m_WalletID, 6, 0, false);
    mainReactor->run();
    sw.stop();
    cout << "Second transfer elapsed time: " << sw.milliseconds() << " ms\n";

    // check coins
    newSenderCoins = sender.GetCoins();
    newReceiverCoins = receiver.GetCoins();

    WALLET_CHECK(newSenderCoins.size() == 5);
    WALLET_CHECK(newReceiverCoins.size() == 2);

    WALLET_CHECK(newReceiverCoins[0].m_amount == 4);
    WALLET_CHECK(newReceiverCoins[0].m_status == Coin::Unspent);
    WALLET_CHECK(newReceiverCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newReceiverCoins[1].m_amount == 6);
    WALLET_CHECK(newReceiverCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newReceiverCoins[1].m_key_type == Key::Type::Regular);


    WALLET_CHECK(newSenderCoins[0].m_amount == 5);
    WALLET_CHECK(newSenderCoins[0].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[0].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[1].m_amount == 2);
    WALLET_CHECK(newSenderCoins[1].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[1].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[2].m_amount == 1);
    WALLET_CHECK(newSenderCoins[2].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[2].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[3].m_amount == 9);
    WALLET_CHECK(newSenderCoins[3].m_status == Coin::Spent);
    WALLET_CHECK(newSenderCoins[3].m_key_type == Key::Type::Regular);

    WALLET_CHECK(newSenderCoins[4].m_amount == 3);
    WALLET_CHECK(newSenderCoins[4].m_status == Coin::Unspent);
    WALLET_CHECK(newSenderCoins[4].m_key_type == Key::Type::Regular);

    // Tx history check
    sh = sender.m_WalletDB->getTxHistory();
    WALLET_CHECK(sh.size() == 2);
    rh = receiver.m_WalletDB->getTxHistory();
    WALLET_CHECK(rh.size() == 2);
    stx = sender.m_WalletDB->getTx(txId);
    WALLET_CHECK(stx.is_initialized());
    rtx = receiver.m_WalletDB->getTx(txId);
    WALLET_CHECK(rtx.is_initialized());

    WALLET_CHECK(stx->m_txId == rtx->m_txId);
    WALLET_CHECK(stx->m_amount == rtx->m_amount);
    WALLET_CHECK(stx->m_status == TxStatus::Completed);
    WALLET_CHECK(stx->m_message == rtx->m_message);
    WALLET_CHECK(stx->m_createTime >= rtx->m_createTime);
    WALLET_CHECK(stx->m_status == rtx->m_status);
    WALLET_CHECK(stx->m_fsmState.empty());
    WALLET_CHECK(rtx->m_fsmState.empty());
    WALLET_CHECK(stx->m_sender == true);
    WALLET_CHECK(rtx->m_sender == false);


    // third transfer. no enough money should appear
    sw.start();

    txId = receiver.m_Wallet.transfer_money(receiver.m_WalletID, sender.m_WalletID, 6, 0, false);
    mainReactor->run();
    sw.stop();
    cout << "Third transfer elapsed time: " << sw.milliseconds() << " ms\n";
    // check coins
    newSenderCoins = sender.GetCoins();
    newReceiverCoins = receiver.GetCoins();

    // no coins 
    WALLET_CHECK(newSenderCoins.size() == 5);
    WALLET_CHECK(newReceiverCoins.size() == 2);

    // Tx history check. New failed tx should be added to sender and receiver
    sh = sender.m_WalletDB->getTxHistory();
    WALLET_CHECK(sh.size() == 3);
    rh = receiver.m_WalletDB->getTxHistory();
    WALLET_CHECK(rh.size() == 3);
    stx = sender.m_WalletDB->getTx(txId);
    WALLET_CHECK(stx.is_initialized());
    rtx = receiver.m_WalletDB->getTx(txId);
    WALLET_CHECK(rtx.is_initialized());

    WALLET_CHECK(rtx->m_amount == 6);
    WALLET_CHECK(rtx->m_status == TxStatus::Failed);
    WALLET_CHECK(rtx->m_fsmState.empty());
    WALLET_CHECK(rtx->m_sender == false);


    WALLET_CHECK(stx->m_amount == 6);
    WALLET_CHECK(stx->m_status == TxStatus::Failed);
    WALLET_CHECK(stx->m_fsmState.empty());
    WALLET_CHECK(stx->m_sender == true);
}

void TestSwapTransaction()
{
    cout << "\nTesting atomic swap transaction...\n";

    io::Reactor::Ptr mainReactor{ io::Reactor::create() };
    io::Reactor::Scope scope(*mainReactor);

    int completedCount = 2;
    auto f = [&completedCount, mainReactor](auto)
    {
        --completedCount;
        if (completedCount == 0)
        {
            mainReactor->stop();
            completedCount = 2;
        }
    };
    TestWalletRig sender("sender", createSenderWalletDB(), mainReactor, f);
    TestWalletRig receiver("receiver", createReceiverWalletDB(), mainReactor, f);
    TestNode node{ sender.m_NodeAddress };

    /*TxID txID =*/ sender.m_Wallet.swap_coins(sender.m_WalletID, receiver.m_WalletID, 4, 1, wallet::AtomicSwapCoin::Bitcoin, 2);

    auto receiverCoins = receiver.GetCoins();
    WALLET_CHECK(receiverCoins.empty());

    mainReactor->run();

    receiverCoins = receiver.GetCoins();
    WALLET_CHECK(receiverCoins.size() == 1);
    WALLET_CHECK(receiverCoins[0].m_amount == 4);
}

void TestSplitKey()
{
    Scalar::Native nonce;
    nonce = (uint64_t) 0xa231234f92381353UL;

    auto res1 = beam::wallet::splitKey(nonce, 123456789);
    auto res2 = beam::wallet::splitKey(nonce, 123456789);
    auto res3 = beam::wallet::splitKey(nonce, 123456789);
    WALLET_CHECK(res1.first == res2.first && res2.first == res3.first);
    WALLET_CHECK(res1.second == res2.second && res2.second == res3.second);
    Scalar::Native s2 = res1.second;
    s2 = -s2;
    Scalar::Native s1 = res1.first+s2;
    WALLET_CHECK(s1 == nonce);
}

struct MyMmr : public Merkle::Mmr
{
    typedef std::vector<Merkle::Hash> HashVector;
    typedef std::unique_ptr<HashVector> HashVectorPtr;

    std::vector<HashVectorPtr> m_vec;

    Merkle::Hash& get_At(const Merkle::Position& pos)
    {
        if (m_vec.size() <= pos.H)
            m_vec.resize(pos.H + 1);

        HashVectorPtr& ptr = m_vec[pos.H];
        if (!ptr)
            ptr.reset(new HashVector);


        HashVector& vec = *ptr;
        if (vec.size() <= pos.X)
            vec.resize(pos.X + 1);

        return vec[pos.X];
    }

    virtual void LoadElement(Merkle::Hash& hv, const Merkle::Position& pos) const override
    {
        hv = ((MyMmr*)this)->get_At(pos);
    }

    virtual void SaveElement(const Merkle::Hash& hv, const Merkle::Position& pos) override
    {
        get_At(pos) = hv;
    }
};

struct RollbackIO : public TestNetwork
{
    RollbackIO(IOLoop& mainLoop, MiniBlockChain& mcm, Height branch, unsigned step)
        : TestNetwork(mainLoop)
        , m_mcm(mcm)
        , m_branch(branch)
        , m_step(step)
    {


    }

    void InitHdr(proto::NewTip& msg) override
    {
        msg.m_Description = m_mcm.m_vStates.back().m_Hdr;
    }

    void send_node_message(beam::proto::GetProofState&& msg) override
    {
        cout << "Rollback. GetProofState Height=" << msg.m_Height << "\n";

        proto::ProofState msgOut;

        assert(msg.m_Height >= Rules::HeightGenesis);

        // TODO: Wallet must not request proofs beyond max height (this doesn't make sense)
        if (msg.m_Height < m_mcm.m_vStates.back().m_Hdr.m_Height)
        {
            Merkle::ProofBuilderHard bld;
            m_mcm.m_Mmr.get_Proof(bld, msg.m_Height - Rules::HeightGenesis);

            msgOut.m_Proof.swap(bld.m_Proof);
            msgOut.m_Proof.push_back(m_mcm.m_hvLive);
        }

        enqueueNetworkTask([this, msgOut]{
            m_peers[0]->handle_node_message((proto::ProofState&&) msgOut);
        });
    }

    void send_node_message(proto::GetMined&& data) override
    {
        Height h = m_step > 1 ? m_step * Height((m_branch - 1) / m_step) : m_branch - 1;
        assert(data.m_HeightMin == Rules::HeightGenesis || data.m_HeightMin == h);
        WALLET_CHECK(data.m_HeightMin == Rules::HeightGenesis || data.m_HeightMin == h);
        TestNetwork::send_node_message(move(data));
    }

    void close_node_connection() override
    {
        shutdown();
    }

    MiniBlockChain& m_mcm;
    Height m_branch;
    unsigned m_step;
};

void TestRollback(Height branch, Height current, unsigned step = 1)
{
    cout << "\nRollback from " << current << " to " << branch << " step: " << step <<'\n';
    auto db = createSqliteWalletDB("wallet.db");
    
    MiniBlockChain mcmOld, mcmNew;

    for (Height i = Rules::HeightGenesis; i <= current; ++i)
    {
        mcmOld.Add();

        if (i == branch)
            mcmNew.m_hvLive = 1U; // branching
        mcmNew.Add();

        if (i % step == 0)
        {
            Coin coin1 = { 5, Coin::Unspent, 0, 0, Key::Type::Regular, i };
            mcmOld.m_vStates.back().m_Hdr.get_Hash(coin1.m_confirmHash);

            db->store(coin1);
        }
    }

    Block::SystemState::ID id;
    mcmOld.m_vStates.back().m_Hdr.get_ID(id);
    db->setSystemStateID(id);

    IOLoop mainLoop;
    auto network = make_shared<RollbackIO>(mainLoop, mcmNew, branch, step);

    Wallet sender(db, network);
    
    network->registerPeer(&sender, true);
    
    mainLoop.run();
}

void TestRollback()
{
    cout << "\nTesting wallet rollback...\n";
    Height s = 10;
    for (Height i = 1; i <= s; ++i)
    {
        TestRollback(i, s);
        TestRollback(i, s, 2);
    }
    s = 11;
    for (Height i = 1; i <= s; ++i)
    {
        TestRollback(i, s);
        TestRollback(i, s, 2);
    }
    
    TestRollback(1, 1);
    TestRollback(2, 50);
    TestRollback(2, 51);
    TestRollback(93, 120);
    TestRollback(93, 120, 6);
    TestRollback(93, 120, 7);
    TestRollback(99, 100);
}

int main()
{
    int logLevel = LOG_LEVEL_DEBUG;
#if LOG_VERBOSE_ENABLED
    logLevel = LOG_LEVEL_VERBOSE;
#endif
    auto logger = beam::Logger::create(logLevel, logLevel);

	Rules::get().FakePoW = true;
	Rules::get().UpdateChecksum();

    TestSplitKey();
    TestP2PWalletNegotiationST();
    TestP2PWalletReverseNegotiationST();

    TestWalletNegotiation(CreateWalletDB<TestWalletDB>(), CreateWalletDB<TestWalletDB2>());
    TestWalletNegotiation(createSenderWalletDB(), createReceiverWalletDB());

    //TestSwapTransaction();

    TestTxToHimself();

    TestRollback();

    assert(g_failureCount == 0);
    return WALLET_CHECK_RESULT;
}
