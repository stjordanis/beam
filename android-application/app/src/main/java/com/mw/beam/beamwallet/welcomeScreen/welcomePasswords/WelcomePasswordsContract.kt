package com.mw.beam.beamwallet.welcomeScreen.welcomePasswords

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpRepository
import com.mw.beam.beamwallet.baseScreen.MvpView

/**
 * Created by vain onnellinen on 10/23/18.
 */
interface WelcomePasswordsContract {
    interface View : MvpView {
    }

    interface Presenter : MvpPresenter<View> {
    }

    interface Repository : MvpRepository
}
