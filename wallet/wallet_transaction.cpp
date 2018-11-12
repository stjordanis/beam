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

#include "wallet_transaction.h"
#include "core/block_crypt.h"
#include <boost/uuid/uuid_generators.hpp>

namespace beam { namespace wallet
{
    using namespace ECC;
    using namespace std;


    TxID GenerateTxID()
    {
        boost::uuids::uuid id = boost::uuids::random_generator()();
        TxID txID{};
        copy(id.begin(), id.end(), txID.begin());
        return txID;
    }

    std::string GetFailureMessage(TxFailureReason reason)
    {
        switch (reason)
        {
#define MACRO(name, code, message) case name: return message;
            BEAM_TX_FAILURE_REASON_MAP(MACRO)
#undef MACRO
        }
        return "Unknown reason";
    }

    TransactionFailedException::TransactionFailedException(bool notify, TxFailureReason reason, const char* message)
        : std::runtime_error(message)
        , m_Notify{notify}
        , m_Reason{reason}
    {

    }
    bool TransactionFailedException::ShouldNofify() const
    {
        return m_Notify;
    }

    TxFailureReason TransactionFailedException::GetReason() const
    {
        return m_Reason;
    }

    BaseTransaction::BaseTransaction(INegotiatorGateway& gateway
                                   , beam::IWalletDB::Ptr walletDB
                                   , const TxID& txID)
        : m_Gateway{ gateway }
        , m_WalletDB{ walletDB }
        , m_ID{ txID }
    {
        assert(walletDB);
    }

    bool BaseTransaction::IsInitiator() const
    {
        if (!m_IsInitiator.is_initialized())
        {
            m_IsInitiator = GetMandatoryParameter<bool>(TxParameterID::IsInitiator);
        }
        return *m_IsInitiator;
    }

    bool BaseTransaction::GetTip(Block::SystemState::Full& state) const
    {
        return m_Gateway.get_tip(state);
    }

    const TxID& BaseTransaction::GetTxID() const
    {
        return m_ID;
    }

    void BaseTransaction::Update()
    {
        try
        {
            TxFailureReason reason = TxFailureReason::Unknown;
            if (GetParameter(TxParameterID::FailureReason, reason))
            {
                OnFailed(reason);
                return;
            }

            WalletID myID = GetMandatoryParameter<WalletID>(TxParameterID::MyID);
            auto address = m_WalletDB->getAddress(myID);
            if (address.is_initialized() && address->isExpired())
            {
                OnFailed(TxFailureReason::ExpiredAddressProvided);
                return;
            }

            UpdateImpl();
        }
        catch (const TransactionFailedException& ex)
        {
            LOG_ERROR() << GetTxID() << " exception msg: " << ex.what();
            OnFailed(ex.GetReason(), ex.ShouldNofify());
        }
        catch (const exception& ex)
        {
            LOG_ERROR() << GetTxID() << " exception msg: " << ex.what();
        }
    }

    void BaseTransaction::Cancel()
    {
        TxStatus s = TxStatus::Failed;
        GetParameter(TxParameterID::Status, s);
        if (s == TxStatus::Pending)
        {
            m_WalletDB->deleteTx(GetTxID());
        }
        else
        {
            UpdateTxDescription(TxStatus::Cancelled);
            RollbackTx();
            SetTxParameter msg;
            msg.AddParameter(TxParameterID::FailureReason, TxFailureReason::Cancelled);
            SendTxParameters(move(msg));
            m_Gateway.on_tx_completed(GetTxID());
        }
    }

    void BaseTransaction::RollbackTx()
    {
        LOG_INFO() << GetTxID() << " Transaction failed. Rollback...";
        m_WalletDB->rollbackTx(GetTxID());
    }

    void BaseTransaction::ConfirmKernel(const TxKernel& kernel)
    {
        UpdateTxDescription(TxStatus::Registered);

        auto coins = m_WalletDB->getCoinsCreatedByTx(GetTxID());

        for (auto& coin : coins)
        {
            coin.m_status = Coin::Unconfirmed;
        }
        m_WalletDB->update(coins);

        m_Gateway.confirm_kernel(GetTxID(), kernel);
    }

    void BaseTransaction::CompleteTx()
    {
        LOG_INFO() << GetTxID() << " Transaction completed";
        UpdateTxDescription(TxStatus::Completed);
        m_Gateway.on_tx_completed(GetTxID());
    }

    void BaseTransaction::UpdateTxDescription(TxStatus s)
    {
        SetParameter(TxParameterID::Status, s);
        SetParameter(TxParameterID::ModifyTime, getTimestamp());
    }

    void BaseTransaction::OnFailed(TxFailureReason reason, bool notify)
    {
        LOG_ERROR() << GetTxID() << " Failed. " << GetFailureMessage(reason);
        UpdateTxDescription((reason == TxFailureReason::Cancelled) ? TxStatus::Cancelled : TxStatus::Failed);
        RollbackTx();
        if (notify)
        {
            SetTxParameter msg;
            msg.AddParameter(TxParameterID::FailureReason, reason);
            SendTxParameters(move(msg));
        }
        m_Gateway.on_tx_completed(GetTxID());
    }

    IWalletDB::Ptr BaseTransaction::GetWalletDB()
    {
        return m_WalletDB;
    }

    vector<Coin> BaseTransaction::GetUnconfirmedOutputs() const
    {
        vector<Coin> outputs;
        m_WalletDB->visit([&](const Coin& coin)
        {
            if ((coin.m_createTxId == GetTxID() && coin.m_status == Coin::Unconfirmed)
                || (coin.m_spentTxId == GetTxID() && coin.m_status == Coin::Locked))
            {
                outputs.emplace_back(coin);
            }

            return true;
        });
        return outputs;
    }

    bool BaseTransaction::SendTxParameters(SetTxParameter&& msg) const
    {
        msg.m_txId = GetTxID();
        msg.m_Type = GetType();
        
        WalletID peerID = {0};
        if (GetParameter(TxParameterID::MyID, msg.m_from) 
            && GetParameter(TxParameterID::PeerID, peerID))
        {
            m_Gateway.send_tx_params(peerID, move(msg));
            return true;
        }
        return false;
    }

    SimpleTransaction::SimpleTransaction(INegotiatorGateway& gateway
        , beam::IWalletDB::Ptr walletDB
        , const TxID& txID)
        : BaseTransaction{ gateway, walletDB, txID }
    {

    }

    TxType SimpleTransaction::GetType() const
    {
        return TxType::Simple;
    }

    void SimpleTransaction::UpdateImpl()
    {
        bool sender = GetMandatoryParameter<bool>(TxParameterID::IsSender);
        Amount amount = GetMandatoryParameter<Amount>(TxParameterID::Amount);
        Amount fee = GetMandatoryParameter<Amount>(TxParameterID::Fee);

        WalletID peerID = GetMandatoryParameter<WalletID>(TxParameterID::PeerID);
        auto address = m_WalletDB->getAddress(peerID);
        bool isSelfTx = address.is_initialized() && address->m_own;
        State txState = GetState();
        TxBuilder builder{ *this, amount, fee };
        if (!builder.GetInitialTxParams() && txState == State::Initial)
        {
            LOG_INFO() << GetTxID() << (sender ? " Sending " : " Receiving ") << PrintableAmount(amount) << " (fee: " << PrintableAmount(fee) << ")";

            if (sender)
            {
                builder.SelectInputs();
                builder.AddChangeOutput();
            }

            if (isSelfTx || !sender)
            {
                // create receiver utxo
                builder.AddOutput(amount);

                LOG_INFO() << GetTxID() << " Invitation accepted";
            }
            UpdateTxDescription(TxStatus::InProgress);
        }

        builder.GenerateNonce();
        
        if (!isSelfTx && !builder.GetPeerPublicExcessAndNonce())
        {
            assert(IsInitiator());
            if (txState == State::Initial)
            {
                SetTxParameter msg;
                msg.AddParameter(TxParameterID::Amount, builder.m_Amount)
                    .AddParameter(TxParameterID::Fee, builder.m_Fee)
                    .AddParameter(TxParameterID::MinHeight, builder.m_MinHeight)
                    .AddParameter(TxParameterID::MaxHeight, builder.m_MaxHeight)
                    .AddParameter(TxParameterID::IsSender, !sender)
                    .AddParameter(TxParameterID::PeerInputs, builder.m_Inputs)
                    .AddParameter(TxParameterID::PeerOutputs, builder.m_Outputs)
                    .AddParameter(TxParameterID::PeerPublicExcess, builder.GetPublicExcess())
                    .AddParameter(TxParameterID::PeerPublicNonce, builder.GetPublicNonce())
                    .AddParameter(TxParameterID::PeerOffset, builder.m_Offset);

                if (!SendTxParameters(move(msg)))
                {
                    OnFailed(TxFailureReason::FailedToSendParameters, false);
                }
                SetState(State::Invitation);
            }
            return;
        }

        builder.SignPartial();

        if (!isSelfTx && !builder.GetPeerSignature())
        {
            if (txState == State::Initial)
            {
                // invited participant
                assert(!IsInitiator());
                // Confirm invitation
                SetTxParameter msg;
                msg.AddParameter(TxParameterID::PeerPublicExcess, builder.GetPublicExcess())
                    .AddParameter(TxParameterID::PeerSignature, builder.m_PartialSignature)
                    .AddParameter(TxParameterID::PeerPublicNonce, builder.GetPublicNonce());

                UpdateTxDescription(TxStatus::Registered);
                SendTxParameters(move(msg));
                SetState(State::InvitationConfirmation);
            }
            return;
        }

        if (!builder.IsPeerSignatureValid())
        {
            OnFailed(TxFailureReason::InvalidPeerSignature, true);
            return;
        }

        builder.FinalizeSignature();

        bool isRegistered = false;
        if (!GetParameter(TxParameterID::TransactionRegistered, isRegistered))
        {
            if (!isSelfTx && !builder.GetPeerInputsAndOutputs())
            {
                assert(IsInitiator());
                if (txState == State::Invitation)
                {
                    // Confirm transaction
                    SetTxParameter msg;
                    msg.AddParameter(TxParameterID::PeerSignature, Scalar(builder.m_PartialSignature));

                    UpdateTxDescription(TxStatus::Registered);
                    SendTxParameters(move(msg));
                    SetState(State::PeerConfirmation);
                }
            }
            else //if ((isSelfTx && txState == State::Initial) || txState == State::InvitationConfirmation)
            {
                // Construct and verify transaction
                auto transaction = builder.CreateTransaction();

                // Verify final transaction
                TxBase::Context ctx;
                if (!transaction->IsValid(ctx))
                {
                    OnFailed(TxFailureReason::InvalidTransaction, true);
                    return;
                }
                m_Gateway.register_tx(GetTxID(), transaction);
                SetState(State::Registration);
            }
            return;
        }

        if (!isRegistered)
        {
            OnFailed(TxFailureReason::FailedToRegister, true);
            return;
        }

        TxKernel::LongProof kernelProof;
        if (!GetParameter(TxParameterID::KernelProof, kernelProof))
        {
            if (!IsInitiator() && txState == State::Registration)
            {
                // notify peer that transaction has been registered
                SetTxParameter msg;
                msg.AddParameter(TxParameterID::TransactionRegistered, true);
                SendTxParameters(move(msg));
            }
            SetState(State::KernelConfirmation);
            ConfirmKernel(*builder.m_Kernel);
            return;
        }

        Block::SystemState::Full state;
        if (!GetTip(state))
        {
            if (!state.IsValidProofKernel(*builder.m_Kernel, kernelProof) && !m_Gateway.isTestMode())
            {
                OnFailed(TxFailureReason::InvalidKernelProof, false);
                return;
            }
            return;
        }

        vector<Coin> unconfirmed = GetUnconfirmedOutputs();
        if (!unconfirmed.empty())
        {
            SetState(State::OutputsConfirmation);
            m_Gateway.confirm_outputs(unconfirmed);
            return;
        }

        CompleteTx();
    }

    SimpleTransaction::State SimpleTransaction::GetState() const
    {
        State state = State::Initial;
        GetParameter(TxParameterID::State, state);
        return state;
    }

    TxBuilder::TxBuilder(BaseTransaction& tx, Amount amount, Amount fee)
        : m_Tx{ tx }
        , m_Amount{ amount }
        , m_Fee{ fee }
        , m_Change{0}
        , m_MinHeight{0}
        , m_MaxHeight{MaxHeight}
    {
    }

    void TxBuilder::SelectInputs()
    {
        Amount amountWithFee = m_Amount + m_Fee;
        auto coins = m_Tx.GetWalletDB()->selectCoins(amountWithFee);
        if (coins.empty())
        {
            LOG_ERROR() << "You only have " << PrintableAmount(getAvailable(m_Tx.GetWalletDB()));
            throw TransactionFailedException(!m_Tx.IsInitiator(), TxFailureReason::NoInputs);
        }

        m_Inputs.reserve(m_Inputs.size() + coins.size());
        Amount total = 0;
        for (auto& coin : coins)
        {
            coin.m_spentTxId = m_Tx.GetTxID();

            Scalar::Native blindingFactor = m_Tx.GetWalletDB()->calcKey(coin);
            auto& input = m_Inputs.emplace_back(make_unique<Input>());
            input->m_Commitment = Commitment(blindingFactor, coin.m_amount);
            m_BlindingExcess += blindingFactor;
            total += coin.m_amount;
        }

        m_Change += total - amountWithFee;

        m_Tx.SetParameter(TxParameterID::BlindingExcess, m_BlindingExcess);
        m_Tx.SetParameter(TxParameterID::Change, m_Change);
        m_Tx.SetParameter(TxParameterID::Inputs, m_Inputs);
        m_Tx.SetParameter(TxParameterID::Offset, m_Offset);

        m_Tx.GetWalletDB()->update(coins);
    }

    void TxBuilder::AddChangeOutput()
    {
        if (m_Change == 0)
        {
            return;
        }

        AddOutput(m_Change);
    }

    void TxBuilder::AddOutput(Amount amount)
    {
        m_Outputs.push_back(CreateOutput(amount, m_MinHeight));
        m_Tx.SetParameter(TxParameterID::Outputs, m_Outputs);
    }

    Output::Ptr TxBuilder::CreateOutput(Amount amount, bool shared, Height incubation)
    {
        Coin newUtxo{ amount, Coin::Draft, m_MinHeight };
        newUtxo.m_createTxId = m_Tx.GetTxID();
        m_Tx.GetWalletDB()->store(newUtxo);

        Scalar::Native blindingFactor;
        Output::Ptr output = make_unique<Output>();
        output->Create(blindingFactor, *m_Tx.GetWalletDB()->get_Kdf(), newUtxo.get_Kidv());

        auto[privateExcess, newOffset] = splitKey(blindingFactor, newUtxo.m_keyIndex);
        blindingFactor = -privateExcess;
        m_BlindingExcess += blindingFactor;
        m_Offset += newOffset;

        m_Tx.SetParameter(TxParameterID::BlindingExcess, m_BlindingExcess);
        m_Tx.SetParameter(TxParameterID::Offset, m_Offset);

        return output;
    }

    void TxBuilder::GenerateNonce()
    {
        // create kernel
        assert(!m_Kernel);
        m_Kernel = make_unique<TxKernel>();
        m_Kernel->m_Fee = m_Fee;
        m_Kernel->m_Height.m_Min = m_MinHeight;
        m_Kernel->m_Height.m_Max = m_MaxHeight;
        m_Kernel->m_Commitment = Zero;

		if (!m_Tx.GetParameter(TxParameterID::MyNonce, m_MultiSig.m_Nonce))
		{
			Coin c;
			c.m_keyIndex = m_Tx.GetWalletDB()->get_AutoIncrID();
			c.m_key_type = Key::Type::Nonce;

			m_MultiSig.m_Nonce = m_Tx.GetWalletDB()->calcKey(c);

			m_Tx.SetParameter(TxParameterID::MyNonce, m_MultiSig.m_Nonce);
		}
    }

    Point::Native TxBuilder::GetPublicExcess() const
    {
        return Context::get().G * m_BlindingExcess;
    }

    Point::Native TxBuilder::GetPublicNonce() const
    {
        return Context::get().G * m_MultiSig.m_Nonce;
    }

    bool TxBuilder::GetPeerPublicExcessAndNonce()
    {
        return m_Tx.GetParameter(TxParameterID::PeerPublicExcess, m_PeerPublicExcess)
            && m_Tx.GetParameter(TxParameterID::PeerPublicNonce, m_PeerPublicNonce);
    }

    bool TxBuilder::GetPeerSignature()
    {
        if (m_Tx.GetParameter(TxParameterID::PeerSignature, m_PeerSignature))
        {
            LOG_DEBUG() << "Received PeerSig:\t" << Scalar(m_PeerSignature);
            return true;
        }
        
        return false;
    }

    bool TxBuilder::GetInitialTxParams()
    {
        m_Tx.GetParameter(TxParameterID::Inputs, m_Inputs);
        m_Tx.GetParameter(TxParameterID::Outputs, m_Outputs);
        m_Tx.GetParameter(TxParameterID::MinHeight, m_MinHeight);
        m_Tx.GetParameter(TxParameterID::MaxHeight, m_MaxHeight);
        return m_Tx.GetParameter(TxParameterID::BlindingExcess, m_BlindingExcess)
            && m_Tx.GetParameter(TxParameterID::Offset, m_Offset);
    }

    bool TxBuilder::GetPeerInputsAndOutputs()
    {
        vector<Input::Ptr> inputs;
        vector<Output::Ptr> outputs;
        Scalar::Native peerOffset;
        return m_Tx.GetParameter(TxParameterID::PeerInputs, m_PeerInputs)
            && m_Tx.GetParameter(TxParameterID::PeerOutputs, m_PeerOutputs)
            && m_Tx.GetParameter(TxParameterID::PeerOffset, m_PeerOffset);
    }

    void TxBuilder::SignPartial()
    {
        // create signature
        Point::Native totalPublicExcess = GetPublicExcess();
        totalPublicExcess += m_PeerPublicExcess;
        m_Kernel->m_Commitment = totalPublicExcess;

        m_Kernel->get_Hash(m_Message);
        m_MultiSig.m_NoncePub = GetPublicNonce() + m_PeerPublicNonce;
        
        
        m_MultiSig.SignPartial(m_PartialSignature, m_Message, m_BlindingExcess);
    }

    void TxBuilder::FinalizeSignature()
    {
        // final signature
        m_Kernel->m_Signature.m_NoncePub = GetPublicNonce() + m_PeerPublicNonce;
        m_Kernel->m_Signature.m_k = m_PartialSignature + m_PeerSignature;
    }

    Transaction::Ptr TxBuilder::CreateTransaction()
    {
        assert(m_Kernel);
        Merkle::Hash kernelID = { 0 };
        m_Kernel->get_ID(kernelID);
        LOG_INFO() << m_Tx.GetTxID() << " Transaction kernel: " << kernelID;
        // create transaction
        auto transaction = make_shared<Transaction>();
        transaction->m_vKernels.push_back(move(m_Kernel));
        transaction->m_Offset = m_Offset + m_PeerOffset;
        transaction->m_vInputs = move(m_Inputs);
        transaction->m_vOutputs = move(m_Outputs);
        move(m_PeerInputs.begin(), m_PeerInputs.end(), back_inserter(transaction->m_vInputs));
        move(m_PeerOutputs.begin(), m_PeerOutputs.end(), back_inserter(transaction->m_vOutputs));

        transaction->Normalize();

        // Verify final transaction
        TxBase::Context ctx;
        assert(transaction->IsValid(ctx));
        return transaction;
    }

    bool TxBuilder::IsPeerSignatureValid() const
    {
        Signature peerSig;
        peerSig.m_NoncePub = m_MultiSig.m_NoncePub;
        peerSig.m_k = m_PeerSignature;
        return peerSig.IsValidPartial(m_Message, m_PeerPublicNonce, m_PeerPublicExcess);
    }
}}
