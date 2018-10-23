package com.mw.beam.beamwallet.welcomeScreen.welcomeMain

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpRepository
import com.mw.beam.beamwallet.baseScreen.MvpView

/**
 * Created by vain onnellinen on 10/19/18.
 */
interface WelcomeMainContract {
    interface View : MvpView {
        fun createWallet()
    }

    interface Presenter : MvpPresenter<View> {
        fun onCreateWallet()
        fun onRestoreWallet()
    }

    interface Repository : MvpRepository
}
