package com.mw.beam.beamwallet.wallet

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpRepository
import com.mw.beam.beamwallet.baseScreen.MvpView
import com.mw.beam.beamwallet.core.entities.TxDescription
import io.reactivex.Observable

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface WalletContract {
    interface View : MvpView {
        fun init()
        fun configTxHistory(txHistory: Array<TxDescription>)
        fun configAvailable(availableSum: Long)
    }

    interface Presenter : MvpPresenter<View>

    interface Repository : MvpRepository {
        fun getTxHistory(): Observable<Array<TxDescription>>
        fun getAvailable(): Observable<Long>
    }
}
