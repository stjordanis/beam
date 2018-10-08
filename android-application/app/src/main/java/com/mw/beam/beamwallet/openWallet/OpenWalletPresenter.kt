package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/1/18.
 */
class OpenWalletPresenter(currentView: OpenWalletContract.View, private val repository: OpenWalletContract.Repository)
    : BasePresenter<OpenWalletContract.View>(currentView),
        OpenWalletContract.Presenter {

    override fun viewIsReady() {
        if (repository.isWalletInitialized()) {
            view?.configForOpeningWallet()
        } else {
            view?.configForCreatingWallet()
        }
    }

    override fun onProceedPressed() {
        view?.hideKeyboard()

        if (repository.isWalletInitialized()) {
            if (view != null && view!!.hasValidPass()) {
                disposable.add(repository.openWallet(view?.getPass()).subscribe { status ->
                    view?.showSnackBar(status)
                })
            }
        } else {
            if (view != null && !view!!.hasErrors()) {
                view?.showSnackBar(repository.createWallet(view?.getPass(), view?.getSeed()))
            }
        }

        view?.startMainActivity()
    }
}
