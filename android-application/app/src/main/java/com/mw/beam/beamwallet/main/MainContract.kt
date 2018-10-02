package com.mw.beam.beamwallet.main

import com.mw.beam.beamwallet.baseScreen.MvpPresenter
import com.mw.beam.beamwallet.baseScreen.MvpView

/**
 * Created by vain onnellinen on 10/1/18.
 */
interface MainContract {
    interface View : MvpView {
    }

    interface Presenter : MvpPresenter<View> {
    }
}
