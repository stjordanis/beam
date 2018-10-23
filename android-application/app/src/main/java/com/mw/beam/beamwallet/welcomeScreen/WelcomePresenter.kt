package com.mw.beam.beamwallet.welcomeScreen

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/19/18.
 */
class WelcomePresenter(currentView: WelcomeContract.View, private val repository: WelcomeContract.Repository)
    : BasePresenter<WelcomeContract.View>(currentView),
        WelcomeContract.Presenter {

    override fun viewIsReady() {
        view?.showMainFragment()
    }

    override fun onCreateWallet() {
        view?.showDescriptionFragment()
    }

    override fun onGeneratePhrase() {
        //TODO change to appropriate screen when implemented
        view?.showPasswordsFragment()
    }
}
