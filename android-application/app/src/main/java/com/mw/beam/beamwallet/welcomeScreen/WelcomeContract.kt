package com.mw.beam.beamwallet.welcomeScreen

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpRepository
import com.mw.beam.beamwallet.baseScreen.MvpView

/**
 * Created by vain onnellinen on 10/19/18.
 */
interface WelcomeContract {
    interface View : MvpView {
        fun showMainFragment()
        fun showDescriptionFragment()
        fun showPasswordsFragment()
    }

    interface Presenter : MvpPresenter<View> {
        fun onCreateWallet()
        fun onGeneratePhrase()
    }

    interface Repository : MvpRepository
}
