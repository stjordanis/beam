package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.BaseModel
import com.mw.beam.beamwallet.core.Api
import com.mw.beam.beamwallet.core.ApiConfig

/**
 * Created by vain onnellinen on 10/1/18.
 */
class OpenWalletModel : BaseModel() {

    fun isWalletInitialized(): Boolean {
        return Api.isWalletInitialized(ApiConfig.DB_PATH)
    }

    fun createWallet(pass: String?, seed: String?): ApiConfig.Status {
        return if (pass.isNullOrBlank() || seed.isNullOrBlank())
            ApiConfig.Status.STATUS_ERROR
        else ApiConfig.Status.fromValue(Api.createWallet(ApiConfig.DB_PATH, pass!!, seed!!))
    }

    fun openWallet(pass: String?): ApiConfig.Status {
        return if (pass.isNullOrBlank())
            ApiConfig.Status.STATUS_ERROR
        else ApiConfig.Status.fromValue(Api.openWallet(ApiConfig.DB_PATH, pass!!))
    }
}
