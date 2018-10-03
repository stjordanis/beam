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

public class WalletListenerJNI extends WalletListener
{
	@Override
	public void onKeychainChanged()
	{
		System.out.println("onKeychainChanged");
	}

	@Override
	public void onTransactionChanged()
	{
		System.out.println("onTransactionChanged");
	}

	@Override
	public void onSystemStateChanged()
	{
		System.out.println("onSystemStateChanged");
	}

	@Override
	public void onTxPeerChanged()
	{
		System.out.println("onTxPeerChanged");
	}

	@Override
	public void onAddressChanged()
	{
		System.out.println("onAddressChanged");
	}

    @Override
	public void onSyncProgress(int done, int total)
	{
		System.out.println("onSyncProgress [" + done + "/" + total + "]");
	}
}
