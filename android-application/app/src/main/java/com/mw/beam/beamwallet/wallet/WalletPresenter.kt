package com.mw.beam.beamwallet.wallet

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/1/18.
 */
class WalletPresenter(currentView: WalletContract.View, private val model: WalletModel)
    : BasePresenter<WalletContract.View>(currentView),
        WalletContract.Presenter {

    override fun viewIsReady() {
        super.viewIsReady()
        view?.init()
        view?.configData(model.wallet)
    }
}
