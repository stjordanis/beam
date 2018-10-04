package com.mw.beam.beamwallet.baseScreen

/**
 * Created by vain onnellinen on 10/1/18.
 */
abstract class BasePresenter<T : MvpView>(var view: T?) : MvpPresenter<T> {

    override fun detachView() {
        view = null
    }

    protected fun isViewAttached(): Boolean {
        return view != null
    }

    override fun viewIsReady() {
    }
}
