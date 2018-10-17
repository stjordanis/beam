package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.BasePresenter
import com.mw.beam.beamwallet.core.AppConfig

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
                if (AppConfig.Status.STATUS_OK == repository.openWallet(view?.getPass())) {
                    view?.startMainActivity()
                } else {
                    view?.showSnackBar(AppConfig.Status.STATUS_ERROR)
                }
            }
        } else {
            if (view != null && !view!!.hasErrors()) {
                if (AppConfig.Status.STATUS_OK == repository.createWallet(view?.getPass(), view?.getSeed())) {
                    view?.startMainActivity()
                } else {
                    view?.showSnackBar(AppConfig.Status.STATUS_ERROR)
                }
            }
        }
    }
}
