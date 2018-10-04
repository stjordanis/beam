package com.mw.beam.beamwallet.core

import com.mw.beam.beamwallet.core.entities.SystemState
import com.mw.beam.beamwallet.core.entities.TxDescription
import com.mw.beam.beamwallet.core.entities.Utxo

/**
 * Created by vain onnellinen on 10/2/18.
 */
data class Wallet(val _this: Long) {
    external fun closeWallet()
    external fun changeWalletPassword(pass: String)
    external fun getSystemState(): SystemState
    external fun getUtxos(): Array<Utxo>
    external fun getTxHistory(): Array<TxDescription>
    external fun getAvailable(): Long
}
