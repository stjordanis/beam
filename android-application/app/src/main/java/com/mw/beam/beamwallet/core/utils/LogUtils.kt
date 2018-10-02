package com.mw.beam.beamwallet.core.utils

import com.mw.beam.beamwallet.BuildConfig

/**
 * Created by vain onnellinen on 10/1/18.
 */
object LogUtils {
    private const val LOG_TAG = "BeamWallet"
    private const val LOG_TAG_ERROR = "ERROR"

    fun log(message: String) {
        if (BuildConfig.DEBUG) {
            android.util.Log.e(LOG_TAG, message)
        }
    }

    fun logD(message: String) {
        if (BuildConfig.DEBUG) {
            android.util.Log.d(LOG_TAG, message)
        }
    }

    fun logE(throwable: Throwable) {
        if (BuildConfig.DEBUG) {
            android.util.Log.e(LOG_TAG + LOG_TAG_ERROR, throwable.message ?: "Exception during loadAsync")
        }
    }
}
