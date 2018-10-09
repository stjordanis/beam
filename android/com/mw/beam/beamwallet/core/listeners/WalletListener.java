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

package com.mw.beam.beamwallet.core.listeners;

public class WalletListener
{
	static void onStatus(){} //const WalletStatus& status)
	static void onTxStatus(){} //beam::ChangeAction, const std::vector<beam::TxDescription>& items) {}
	static void onTxPeerUpdated(){} //const std::vector<beam::TxPeer>& peers) {}
	static void onSyncProgressUpdated(){} //int done, int total) {}
	static void onChangeCalculated(){} //beam::Amount change) {}
	static void onAllUtxoChanged(){} //const std::vector<beam::Coin>& utxos) {}
	static void onAdrresses(){} //bool own, const std::vector<beam::WalletAddress>& addresses) {}
	static void onGeneratedNewWalletID(){} //const beam::WalletID& walletID) {}
	static void onChangeCurrentWalletIDs(){} //beam::WalletID senderID, beam::WalletID receiverID) {}

	static void onKeychainChanged() {}
	static void onTransactionChanged() {}
	static void onSystemStateChanged() {}
	static void onTxPeerChanged() {}
	static void onAddressChanged() {}
	static void onSyncProgress(int done, int total) {}
}
