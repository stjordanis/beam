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

import com.mw.beam.beamwallet.core.entities.WalletStatus;
import com.mw.beam.beamwallet.core.entities.Utxo;

public class WalletListener
{
	static void onStatus(WalletStatus status)
	{
		System.out.println(">>>>>>>>>>>>>> async status in Java, available=" + status.available/1000000 + " BEAM and " + status.available%1000000 + " GROTH, unconfirmed=" + status.unconfirmed);
	}

	static void onTxStatus(){} //beam::ChangeAction, const std::vector<beam::TxDescription>& items) {}
	static void onTxPeerUpdated(){} //const std::vector<beam::TxPeer>& peers) {}
	static void onSyncProgressUpdated(){} //int done, int total) {}
	static void onChangeCalculated(){} //beam::Amount change) {}

	static void onAllUtxoChanged(Utxo[] utxos)
	{
		System.out.println(">>>>>>>>>>>>>> async utxo in Java");
		
		if(utxos != null)
		{
			System.out.println("utxos length: " + utxos.length);

			System.out.println("+-------------------------------------------------------");
			System.out.println("| UTXO");
			System.out.println("+-------------------------------------------------------");
			System.out.println("| id:   | amount:       | type:");
			System.out.println("+-------+---------------+-------------------------------");

			for(int i = 0; i < utxos.length; i++)
			{
				System.out.println("| " + utxos[i].id 
					+ "\t| "  + utxos[i].amount
					+ "\t| "  + utxos[i].keyType);
			}

			System.out.println("+-------------------------------------------------------");			
		}
	}

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
