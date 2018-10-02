package com.mw.beam.beamwallet.baseScreen

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface MvpPresenter<V : MvpView> {
    fun viewIsReady()
    fun detachView()
}
