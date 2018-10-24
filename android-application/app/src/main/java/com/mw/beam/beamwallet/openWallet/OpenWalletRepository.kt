package com.mw.beam.beamwallet.openWallet

import com.mw.beam.beamwallet.baseScreen.BaseRepository
import com.mw.beam.beamwallet.core.Api
import com.mw.beam.beamwallet.core.AppConfig
import com.mw.beam.beamwallet.core.utils.LogUtils

/**
 * Created by vain onnellinen on 10/1/18.
 */
@Suppress("RECEIVER_NULLABILITY_MISMATCH_BASED_ON_JAVA_ANNOTATIONS")
class OpenWalletRepository : BaseRepository(), OpenWalletContract.Repository {

    override fun isWalletInitialized(): Boolean {
        val result = Api.isWalletInitialized(AppConfig.DB_PATH)
        LogUtils.logResponse(result, object {}.javaClass.enclosingMethod.name)
        return result

    }

    override fun createWallet(pass: String?, seed: String?): AppConfig.Status {
        var result = AppConfig.Status.STATUS_ERROR

        if (!pass.isNullOrBlank() && !seed.isNullOrBlank()) {
            setWallet(Api.createWallet(AppConfig.NODE_ADDRESS, AppConfig.DB_PATH, pass!!, seed!!))
            result = AppConfig.Status.STATUS_OK
        }

        LogUtils.logResponse(result, object {}.javaClass.enclosingMethod.name)
        return result
    }

    override fun openWallet(pass: String?): AppConfig.Status {
        var result = AppConfig.Status.STATUS_ERROR

        if (!pass.isNullOrBlank()) {
          //  setWallet(Api.openWallet(AppConfig.NODE_ADDRESS, AppConfig.DB_PATH, pass!!))
            result = AppConfig.Status.STATUS_OK
        }

        LogUtils.logResponse(result, object {}.javaClass.enclosingMethod.name)
        return result
    }
}
