package com.mw.beam.beamwallet.wallet

import com.mw.beam.beamwallet.baseScreen.BaseModel
import com.mw.beam.beamwallet.core.entities.TxDescription
import io.reactivex.Observable

/**
 * Created by vain onnellinen on 10/1/18.
 */
class WalletModel : BaseModel() {

    fun getTxHistory(): Observable<Array<TxDescription>> = createObservable { wallet.getTxHistory() }
    fun getAvailable(): Observable<Long> = createObservable { wallet.getAvailable() }
}
