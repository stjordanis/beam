package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/1/18.
 */
class OpenWalletPresenter(currentView: OpenWalletContract.View, private val model: OpenWalletModel)
    : BasePresenter<OpenWalletContract.View>(currentView),
        OpenWalletContract.Presenter {

    override fun viewIsReady() {
        if (model.isWalletInitialized()) {
            view?.configForOpeningWallet()
        } else {
            view?.configForCreatingWallet()
        }
    }

    override fun onProceedPressed() {
        view?.hideKeyboard()

        if (model.isWalletInitialized()) {
            if (view != null && view!!.hasValidPass()) {
                view?.showSnackBar(model.openWallet(view?.getPass()))
            }
        } else {
            if (view != null && !view!!.hasErrors()) {
                view?.showSnackBar(model.createWallet(view?.getPass(), view?.getSeed()))
            }
        }

        view?.startMainActivity()
    }
}
