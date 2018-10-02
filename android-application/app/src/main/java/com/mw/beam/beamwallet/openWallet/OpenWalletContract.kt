package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpView

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface OpenWalletContract {
    interface View : MvpView {
        fun configForCreatingWallet()
        fun configForOpeningWallet()
        fun hasErrors() : Boolean
        fun hasValidPass() : Boolean
        fun getPass() : String
        fun getSeed() : String
        fun startMainActivity()
    }

    interface Presenter : MvpPresenter<View> {
        fun onProceedPressed()
    }
}
