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

import com.mw.beam.beamwallet.core.*;
import com.mw.beam.beamwallet.core.entities.*;

public class WalletJNI
{
	public static void main(String[] args)
	{
		System.out.println("Start Wallet JNI test...");

		Api api = new Api();

		Wallet wallet;

		if(api.isWalletInitialized("test"))
		{
			wallet = api.openWallet("test", "123");

			System.out.println(wallet == null ? "wallet opening error" : "wallet successfully opened");
		}
		else
		{
			wallet = api.createWallet("test", "123", "111");

			System.out.println(wallet == null ? "wallet creation error" : "wallet successfully created");
		}

		if(wallet != null)
		{
			{
				SystemState state = wallet.getSystemState();
				System.out.println("system height is " + state.height);
			}

			{
				long available = wallet.getAvailable();
				System.out.println("available " + available/1000000 + " BEAM and " + available%1000000 + " GROTH");
			}

			{
				Utxo[] utxos = wallet.getUtxos();

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

			{
				TxDescription[] tx = wallet.getTxHistory();

				System.out.println("+-------------------------------------------------------");
				System.out.println("| TRANSACTIONS");
				System.out.println("+----------------------------------------------------------------------");
				System.out.println("| date:                         | amount:       | status:");
				System.out.println("+-------------------------------+---------------+-----------------------");

				for(int i = 0; i < tx.length; i++)
				{
					System.out.println("| " + new java.util.Date(tx[i].createTime*1000)
						+ "\t| " + tx[i].amount
						+ "\t| " + tx[i].status);
				}

				System.out.println("+-------------------------------------------------------");
			}

			wallet.run("172.104.249.212:8101", new WalletListenerJNI());
			wallet.closeWallet();
		}
	}
}
