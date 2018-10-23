package com.mw.beam.beamwallet.welcomeScreen.welcomeMain

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/19/18.
 */
class WelcomeMainPresenter(currentView: WelcomeMainContract.View, private val repository: WelcomeMainContract.Repository)
    : BasePresenter<WelcomeMainContract.View>(currentView),
        WelcomeMainContract.Presenter {

    override fun onCreateWallet() {
        view?.createWallet()
    }

    override fun onRestoreWallet() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }
}
