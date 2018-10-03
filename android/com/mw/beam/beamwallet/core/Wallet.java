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

package com.mw.beam.beamwallet.core;

import java.util.*; 

public class Wallet
{
	long _this;

	WalletListener _listener;

	public native void closeWallet();

	public native void changeWalletPassword(String pass);
	public native SystemState getSystemState();
	public native Utxo[] getUtxos();
	public native TxDescription[] getTxHistory();
	public native void run(String nodeAddr);

	public void listen(WalletListener listener)
	{
		_listener = listener;
	}
}
