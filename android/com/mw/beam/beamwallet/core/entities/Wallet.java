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

package com.mw.beam.beamwallet.core.entities;

import java.util.*; 

public class Wallet
{
	long _this;

	public native void closeWallet();

	// public native void changeWalletPassword(String pass);
	// public native SystemState 		getSystemState();
	// public native Utxo[] 			getUtxos();
	// public native TxDescription[] 	getTxHistory();
	// public native long 				getAvailable();

    public native void sendMoney();//const beam::WalletID& sender, const beam::WalletID& receiver, beam::Amount&& amount, beam::Amount&& fee = 0);
    public native void sendMoney2();//const beam::WalletID& receiver, const std::string& comment, beam::Amount&& amount, beam::Amount&& fee = 0);
    public native void syncWithNode();
    public native void calcChange();//beam::Amount&& amount);
    public native void getWalletStatus();
    public native void getUtxosStatus();
    public native void getAddresses();//bool own);
    public native void cancelTx();//const beam::TxID& id);
    public native void deleteTx();//const beam::TxID& id);
    public native void createNewAddress();//beam::WalletAddress&& address);
    public native void generateNewWalletID();
    public native void changeCurrentWalletIDs();//const beam::WalletID& senderID, const beam::WalletID& receiverID);
    public native void deleteAddress();//const beam::WalletID& id);
    public native void deleteOwnAddress();//const beam::WalletID& id) ;
    public native void setNodeAddress();//const std::string& addr);
    public native void emergencyReset();
    public native void changeWalletPassword();//const beam::SecString& password);


	public native void run(String nodeAddr);
}
