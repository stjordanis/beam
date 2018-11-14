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

#include "wallet/wallet_network.h"
#include "core/common.h"

#include "wallet/wallet.h"
#include "wallet/wallet_db.h"
#include "wallet/wallet_network.h"
#include "wallet/keystore.h"
#include "wallet/secstring.h"
#include "core/ecc_native.h"
#include "core/serialization_adapters.h"
#include "unittests/util.h"

#ifndef LOG_VERBOSE_ENABLED
    #define LOG_VERBOSE_ENABLED 0
#endif

#include "utility/logger.h"
#include "utility/options.h"
#include "utility/helpers.h"
#include <iomanip>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iterator>
#include <future>
#include "version.h"

using namespace std;
using namespace beam;
using namespace ECC;

namespace beam
{
    std::ostream& operator<<(std::ostream& os, Coin::Status s)
    {
        stringstream ss;
        ss << "[";
        switch (s)
        {
        case Coin::Locked: ss << "Locked"; break;
        case Coin::Spent: ss << "Spent"; break;
        case Coin::Unconfirmed: ss << "Unconfirmed"; break;
        case Coin::Unspent: ss << "Unspent"; break;
        case Coin::Draft: ss << "Draft"; break;
        default:
            assert(false && "Unknown coin status");
        }
        ss << "]";
        string str = ss.str();
        os << str;
        size_t c = 13 - str.length();
        for (size_t i = 0; i < c; ++i) os << ' ';
        return os;
    }

    std::ostream& operator<<(std::ostream& os, Key::Type keyType)
    {
        os << "[";
        switch (keyType)
        {
        case Key::Type::Coinbase: os << "Coinbase"; break;
        case Key::Type::Comission: os << "Commission"; break;
        case Key::Type::Kernel: os << "Kernel"; break;
        case Key::Type::Regular: os << "Regular"; break;
        default:
            assert(false && "Unknown key type");
        }
        os << "]";
        return os;
    }

    const char* getTxStatus(const TxDescription& tx)
    {
        static const char* Pending = "Pending";
        static const char* Sending = "Sending";
        static const char* Receiving = "Receiving";
        static const char* Cancelled = "Cancelled";
        static const char* Sent = "Sent";
        static const char* Received = "Received";
        static const char* Failed = "Failed";

        switch (tx.m_status)
        {
        case TxStatus::Pending: return Pending;
        case TxStatus::InProgress: return tx.m_sender ? Sending : Receiving;
        case TxStatus::Cancelled: return Cancelled;
        case TxStatus::Completed: return tx.m_sender ? Sent : Received;
        case TxStatus::Failed: return Failed;
        default:
            assert(false && "Unknown key type");
        }

        return "";
    }
}
namespace
{
    void printHelp(const po::options_description& options)
    {
        cout << options << std::endl;
    }

    IKeyStore::Ptr createKeyStore(const string& path, const SecString& pass)
    {
        IKeyStore::Options options;
        options.flags = IKeyStore::Options::local_file | IKeyStore::Options::enable_all_keys;
        options.fileName = path;

        return IKeyStore::create(options, pass.data(), pass.size());
    }

    void newAddress(
        const IWalletDB::Ptr& walletDB,
        const std::string& label,
        const std::string& bbsKeysPath, const SecString& pass)
    {
        IKeyStore::Ptr ks = createKeyStore(bbsKeysPath, pass);
        WalletAddress address = {};
        address.m_own = true;
        address.m_label = label;
        address.m_createTime = getTimestamp();
        ks->gen_keypair(address.m_walletID);
        ks->save_keypair(address.m_walletID, true);
        walletDB->saveAddress(address);

        char buf[uintBig::nBytes * 2 + 1];
        to_hex(buf, address.m_walletID.m_pData, uintBig::nBytes);

        LOG_INFO() << "New address generated:\n\n" << buf << "\n";
        if (!label.empty()) {
            LOG_INFO() << "label = " << label;
        }
    }
}

io::Reactor::Ptr reactor;

static const unsigned LOG_ROTATION_PERIOD = 3*60*60*1000; // 3 hours

int main_impl(int argc, char* argv[])
{
	beam::Crash::InstallHandler(NULL);

    try
    {
        auto options = createOptionsDescription(GENERAL_OPTIONS | WALLET_OPTIONS);

        po::variables_map vm;
        try
        {
            vm = getOptions(argc, argv, "beam-wallet.cfg", options);
        }
        catch (const po::error& e)
        {
            cout << e.what() << std::endl;
            printHelp(options);

            return 0;
        }

        if (vm.count(cli::HELP))
        {
            printHelp(options);

            return 0;
        }

        if (vm.count(cli::VERSION))
        {
            cout << PROJECT_VERSION << endl;
            return 0;
        }

        if (vm.count(cli::GIT_COMMIT_HASH))
        {
            cout << GIT_COMMIT_HASH << endl;
            return 0;
        }

        int logLevel = getLogLevel(cli::LOG_LEVEL, vm, LOG_LEVEL_DEBUG);
        int fileLogLevel = getLogLevel(cli::FILE_LOG_LEVEL, vm, LOG_LEVEL_DEBUG);

        const auto path = boost::filesystem::system_complete("./logs");
        auto logger = beam::Logger::create(logLevel, logLevel, fileLogLevel, "wallet_", path.string());

        try
        {
            po::notify(vm);

            Rules::get().UpdateChecksum();
            LOG_INFO() << "Rules signature: " << Rules::get().Checksum;

            // TODO later auto port = vm[cli::PORT].as<uint16_t>();

            {
                reactor = io::Reactor::create();
                io::Reactor::Scope scope(*reactor);

                io::Reactor::GracefulIntHandler gih(*reactor);

                NoLeak<uintBig> walletSeed;
                walletSeed.V = Zero;

                io::Timer::Ptr logRotateTimer = io::Timer::create(*reactor);
                logRotateTimer->start(
                    LOG_ROTATION_PERIOD, true,
                    []() {
                        Logger::get()->rotate();
                    }
                );

                {
                    if (vm.count(cli::COMMAND))
                    {
                        auto command = vm[cli::COMMAND].as<string>();
                        if (command != cli::INIT
                            && command != cli::SEND
                            && command != cli::RECEIVE
                            && command != cli::LISTEN
                            && command != cli::TREASURY
                            && command != cli::INFO
                            && command != cli::NEW_ADDRESS
                            && command != cli::CANCEL_TX)
                        {
                            LOG_ERROR() << "unknown command: \'" << command << "\'";
                            return -1;
                        }

                        assert(vm.count(cli::WALLET_STORAGE) > 0);
                        auto walletPath = vm[cli::WALLET_STORAGE].as<string>();

                        assert(vm.count(cli::BBS_STORAGE) > 0);
                        auto bbsKeysPath = vm[cli::BBS_STORAGE].as<string>();

                        if (!WalletDB::isInitialized(walletPath) && command != cli::INIT)
                        {
                            LOG_ERROR() << "Please initialize your wallet first... \nExample: beam-wallet --command=init";
                            return -1;
                        }

                        LOG_INFO() << "starting a wallet...";

                        SecString pass;
                        if (!beam::read_wallet_pass(pass, vm))
                        {
                            LOG_ERROR() << "Please, provide password for the wallet.";
                            return -1;
                        }

                        if (command == cli::INIT)
                        {
                            if (!beam::read_wallet_seed(walletSeed, vm))
                            {
                                LOG_ERROR() << "Please, provide seed phrase for the wallet.";
                                return -1;
                            }
                            auto walletDB = WalletDB::init(walletPath, pass, walletSeed);
                            if (walletDB)
                            {
                                LOG_INFO() << "wallet successfully created...";

                                // generate default address
                                newAddress(walletDB, "default", bbsKeysPath, pass);

                                return 0;
                            }
                            else
                            {
                                LOG_ERROR() << "something went wrong, wallet not created...";
                                return -1;
                            }
                        }

                        auto walletDB = WalletDB::open(walletPath, pass);
                        if (!walletDB)
                        {
                            LOG_ERROR() << "Wallet data unreadable, restore wallet.db from latest backup or delete it and reinitialize the wallet";
                            return -1;
                        }

                        if (command == cli::NEW_ADDRESS)
                        {
                            auto label = vm[cli::NEW_ADDRESS_LABEL].as<string>();
                            newAddress(walletDB, label, bbsKeysPath, pass);

                            if (!vm.count(cli::LISTEN)) {
                                return 0;
                            }
                        }

                        LOG_INFO() << "wallet sucessfully opened...";

                        if (command == cli::TREASURY)
                        {
							uint32_t nCount = vm[cli::TR_COUNT].as<uint32_t>();
							Height dh = vm[cli::TR_DH].as<uint32_t>();

							Amount v = vm[cli::TR_BEAMS].as<uint32_t>();
							v *= Rules::Coin;

							return GenerateTreasury(walletDB.get(), vm[cli::TREASURY_BLOCK].as<string>(), nCount, dh, v);
                        }

                        if (command == cli::INFO)
                        {
                            Block::SystemState::ID stateID = {};
                            walletDB->getSystemStateID(stateID);
                            cout << "____Wallet summary____\n\n"
                                << "Current height............" << stateID.m_Height << '\n'
                                << "Current state ID.........." << stateID.m_Hash << "\n\n"
                                << "Available................." << PrintableAmount(wallet::getAvailable(walletDB)) << '\n'
                                << "Unconfirmed..............." << PrintableAmount(wallet::getTotal(walletDB, Coin::Unconfirmed)) << '\n'
                                << "Locked...................." << PrintableAmount(wallet::getTotal(walletDB, Coin::Locked)) << '\n'
                                << "Draft....................." << PrintableAmount(wallet::getTotal(walletDB, Coin::Draft)) << '\n'
                                << "Available coinbase ......." << PrintableAmount(wallet::getAvailableByType(walletDB, Coin::Unspent, Key::Type::Coinbase)) << '\n'
                                << "Total coinbase............" << PrintableAmount(wallet::getTotalByType(walletDB, Coin::Unspent, Key::Type::Coinbase)) << '\n'
                                << "Avaliable fee............." << PrintableAmount(wallet::getAvailableByType(walletDB, Coin::Unspent, Key::Type::Comission)) << '\n'
                                << "Total fee................." << PrintableAmount(wallet::getTotalByType(walletDB, Coin::Unspent, Key::Type::Comission)) << '\n'
                                << "Total unspent............." << PrintableAmount(wallet::getTotal(walletDB, Coin::Unspent)) << "\n\n";
                            if (vm.count(cli::TX_HISTORY))
                            {
                                auto txHistory = walletDB->getTxHistory();
                                if (txHistory.empty())
                                {
                                    cout << "No transactions\n";
                                    return 0;
                                }

                                cout << "TRANSACTIONS\n\n"
                                    << "| datetime            | amount, BEAM        | status    | ID\t|\n";
                                for (auto& tx : txHistory)
                                {
                                    cout << "  " << format_timestamp("%Y.%m.%d %H:%M:%S", tx.m_createTime * 1000, false)
                                        << setw(17) << PrintableAmount(tx.m_amount, true)
                                        << "  " << getTxStatus(tx) << "  " << tx.m_txId << '\n';
                                }
                                return 0;
                            }

                            cout << "| id\t| amount(Beam)\t| amount(c)\t| height\t| maturity\t| status \t| key type\t|\n";
                            walletDB->visit([](const Coin& c)->bool
                            {
                                cout << setw(8) << c.m_id
                                    << setw(16) << PrintableAmount(Rules::Coin * ((Amount)(c.m_amount / Rules::Coin)))
                                    << setw(16) << PrintableAmount(c.m_amount % Rules::Coin)
                                    << setw(16) << static_cast<int64_t>(c.m_createHeight)
                                    << setw(16) << static_cast<int64_t>(c.m_maturity)
                                    << "  " << c.m_status
                                    << "  " << c.m_key_type << '\n';
                                return true;
                            });
                            return 0;
                        }

                        if (vm.count(cli::NODE_ADDR) == 0)
                        {
                            LOG_ERROR() << "node address should be specified";
                            return -1;
                        }

                        string nodeURI = vm[cli::NODE_ADDR].as<string>();
                        io::Address node_addr;
                        if (!node_addr.resolve(nodeURI.c_str()))
                        {
                            LOG_ERROR() << "unable to resolve node address: " << nodeURI;
                            return -1;
                        }

                        io::Address receiverAddr;
                        Amount amount = 0;
                        Amount fee = 0;
                        ECC::Hash::Value receiverWalletID;
                        bool isTxInitiator = command == cli::SEND || command == cli::RECEIVE;
                        if (isTxInitiator)
                        {
                            if (vm.count(cli::RECEIVER_ADDR) == 0)
                            {
                                LOG_ERROR() << "receiver's address is missing";
                                return -1;
                            }
                            if (vm.count(cli::AMOUNT) == 0)
                            {
                                LOG_ERROR() << "amount is missing";
                                return -1;
                            }

                            ECC::Hash::Value receiverID = from_hex(vm[cli::RECEIVER_ADDR].as<string>());
                            receiverWalletID = receiverID;

                            auto signedAmount = vm[cli::AMOUNT].as<double>();
                            if (signedAmount < 0)
                            {
                                LOG_ERROR() << "Unable to send negative amount of coins";
                                return -1;
                            }

                            signedAmount *= Rules::Coin; // convert beams to coins

                            amount = static_cast<ECC::Amount>(signedAmount);
                            if (amount == 0)
                            {
                                LOG_ERROR() << "Unable to send zero coins";
                                return -1;
                            }

                            auto signedFee = vm[cli::FEE].as<double>();
                            if (signedFee < 0)
                            {
                                LOG_ERROR() << "Unable to take negative fee";
                                return -1;
                            }

                            signedFee *= Rules::Coin; // convert beams to coins

                            fee = static_cast<ECC::Amount>(signedFee);
                        }

                        bool is_server = (command == cli::LISTEN || vm.count(cli::LISTEN));

                        IKeyStore::Ptr keystore = createKeyStore(bbsKeysPath, pass);

                        auto wallet_io = make_shared<WalletNetworkIO >( node_addr
                                                                      , walletDB
                                                                      , keystore
                                                                      , reactor);
                        Wallet wallet{ walletDB
                                     , wallet_io
                                     , false
                                     , is_server ? Wallet::TxCompletedAction() : [wallet_io](auto) { wallet_io->stop(); } };

                        static_pointer_cast<INetworkIO>(wallet_io)->connect_node();

                        if (isTxInitiator)
                        {
                            // TODO: make db request by 'default' label
                            auto addresses = walletDB->getAddresses(true);
                            assert(!addresses.empty());
                            wallet.transfer_money(addresses[0].m_walletID, receiverWalletID, move(amount), move(fee), command == cli::SEND);
                        }

                        if (command == cli::CANCEL_TX) {
                            auto txIdVec = from_hex(vm[cli::TX_ID].as<string>());
                            TxID txId;
                            std::copy_n(txIdVec.begin(), 16, txId.begin());
                            wallet.cancel_tx(txId);
                        }

                        wallet_io->start();
                    }
                    else
                    {
                        LOG_ERROR() << "command parameter not specified.";
                        printHelp(options);
                    }
                }
            }
        }
        catch (const po::error& e)
        {
            LOG_ERROR() << e.what();
            printHelp(options);
        }
        catch (const std::runtime_error& e)
        {
            LOG_ERROR() << e.what();
        }
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    return main_impl(argc, argv);
#else
    block_sigpipe();
    auto f = std::async(
        std::launch::async,
        [argc, argv]() -> int {
            // TODO: this hungs app on OSX
            //lock_signals_in_this_thread();
            int ret = main_impl(argc, argv);
            kill(0, SIGINT);
            return ret;
        }
    );

    wait_for_termination(0);

    if (reactor) reactor->stop();

    return f.get();
#endif
}

