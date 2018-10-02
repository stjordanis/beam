package com.mw.beam.beamwallet.main

import com.mw.beam.beamwallet.baseScreen.BasePresenter

/**
 * Created by vain onnellinen on 10/1/18.
 */
class MainPresenter (currentView: MainContract.View, private val model: MainModel)
    : BasePresenter<MainContract.View>(currentView),
        MainContract.Presenter {
    override fun viewIsReady() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }
}
