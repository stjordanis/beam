package com.mw.beam.beamwallet.baseScreen

import com.mw.beam.beamwallet.core.ApiConfig

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface MvpView {
    fun showSnackBar(status : ApiConfig.Status)
    fun hideKeyboard()
}
