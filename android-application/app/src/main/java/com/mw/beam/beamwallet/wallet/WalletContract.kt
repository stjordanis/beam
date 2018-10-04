package com.mw.beam.beamwallet.wallet

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpView
import com.mw.beam.beamwallet.core.Wallet

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface WalletContract {
    interface View : MvpView {
        fun init()
        fun configData(wallet : Wallet)
    }

    interface Presenter : MvpPresenter<View> {
    }
}
