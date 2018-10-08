package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpRepository
import com.mw.beam.beamwallet.baseScreen.MvpView
import com.mw.beam.beamwallet.core.AppConfig
import io.reactivex.Observable

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface OpenWalletContract {
    interface View : MvpView {
        fun configForCreatingWallet()
        fun configForOpeningWallet()
        fun hasErrors(): Boolean
        fun hasValidPass(): Boolean
        fun getPass(): String
        fun getSeed(): String
        fun startMainActivity()
    }

    interface Presenter : MvpPresenter<View> {
        fun onProceedPressed()
    }

    interface Repository : MvpRepository {
        fun isWalletInitialized(): Boolean
        fun createWallet(pass: String?, seed: String?): AppConfig.Status
        fun openWallet(pass: String?): Observable<AppConfig.Status>
    }
}
