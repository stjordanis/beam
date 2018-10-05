package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.BaseModel
import com.mw.beam.beamwallet.core.Api
import com.mw.beam.beamwallet.core.App
import com.mw.beam.beamwallet.core.AppConfig
import io.reactivex.Observable

/**
 * Created by vain onnellinen on 10/1/18.
 */
class OpenWalletModel : BaseModel() {

    fun isWalletInitialized(): Boolean {
        return Api.isWalletInitialized(AppConfig.DB_PATH)
    }

    fun createWallet(pass: String?, seed: String?): AppConfig.Status {
        return if (pass.isNullOrBlank() || seed.isNullOrBlank()) {
            AppConfig.Status.STATUS_ERROR
        } else {
            App.wallet = Api.createWallet(AppConfig.DB_PATH, pass!!, seed!!)
            AppConfig.Status.STATUS_OK
        }
    }

    fun openWallet(pass: String?): Observable<AppConfig.Status> {
        return createObservable { App.wallet = Api.openWallet(AppConfig.DB_PATH, pass!!) }
    }
}
