package com.mw.beam.beamwallet.baseScreen

import android.app.Activity
import android.support.design.widget.Snackbar
import android.support.v4.content.ContextCompat
import android.support.v7.app.AppCompatActivity
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.core.ApiConfig

/**
 * Created by vain onnellinen on 10/1/18.
 */
abstract class BaseActivity<T : BasePresenter<out MvpView>> : AppCompatActivity(), MvpView {
    private lateinit var presenter: T

    override fun onDestroy() {
        presenter.detachView()

        super.onDestroy()
    }

    fun configPresenter(presenter: T) {
        this.presenter = presenter
        this.presenter.viewIsReady()
    }

    override fun showSnackBar(status: ApiConfig.Status) {
        val snackBar = Snackbar.make(findViewById(android.R.id.content),
                when (status) {
                    ApiConfig.Status.STATUS_OK -> getString(R.string.common_successful)
                    ApiConfig.Status.STATUS_ERROR -> getString(R.string.common_error)
                }, Snackbar.LENGTH_LONG)
        snackBar.view.setBackgroundColor(ContextCompat.getColor(this, R.color.snack_bar_color))
        snackBar.view.findViewById<TextView>(android.support.design.R.id.snackbar_text).setTextColor(ContextCompat.getColor(this, R.color.colorAccent))
        snackBar.show()
    }

    override fun hideKeyboard() {
        val imm = this.getSystemService(Activity.INPUT_METHOD_SERVICE) as InputMethodManager
        imm.hideSoftInputFromWindow(findViewById<View>(android.R.id.content).windowToken, 0)
    }
}
