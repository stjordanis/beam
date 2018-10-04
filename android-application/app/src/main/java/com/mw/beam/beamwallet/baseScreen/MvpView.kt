package com.mw.beam.beamwallet.baseScreen

import com.mw.beam.beamwallet.core.AppConfig

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface MvpView {
    fun showSnackBar(status : AppConfig.Status)
    fun hideKeyboard()
    fun configNavDrawer()
}
