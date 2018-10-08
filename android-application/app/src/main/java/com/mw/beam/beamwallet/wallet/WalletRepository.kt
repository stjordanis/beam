package com.mw.beam.beamwallet.wallet

import com.mw.beam.beamwallet.baseScreen.BaseRepository
import com.mw.beam.beamwallet.core.entities.TxDescription
import io.reactivex.Observable

/**
 * Created by vain onnellinen on 10/1/18.
 */
class WalletRepository : BaseRepository(), WalletContract.Repository {
    override fun getTxHistory(): Observable<Array<TxDescription>> = createObservable { getWallet().getTxHistory() }
    override fun getAvailable(): Observable<Long> = createObservable { getWallet().getAvailable() }
}
