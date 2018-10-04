package com.mw.beam.beamwallet.baseScreen

import android.support.v4.app.Fragment
import com.mw.beam.beamwallet.core.AppConfig

/**
 * Created by vain onnellinen on 10/4/18.
 */
abstract class BaseFragment<T : BasePresenter<out MvpView>> : Fragment(), MvpView {
    private lateinit var presenter: T

    fun configPresenter(presenter: T) {
        this.presenter = presenter
        this.presenter.viewIsReady()
    }

    override fun onDestroy() {
        presenter.detachView()

        super.onDestroy()
    }

    override fun hideKeyboard() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun configNavDrawer() {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }

    override fun showSnackBar(status: AppConfig.Status) {
        TODO("not implemented") //To change body of created functions use File | Settings | File Templates.
    }
}
