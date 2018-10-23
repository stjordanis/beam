package com.mw.beam.beamwallet.welcomeScreen.welcomePasswords

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/23/18.
 */
class WelcomePasswordsPresenter(currentView: WelcomePasswordsContract.View, private val repository: WelcomePasswordsContract.Repository)
    : BasePresenter<WelcomePasswordsContract.View>(currentView),
        WelcomePasswordsContract.Presenter {
}
