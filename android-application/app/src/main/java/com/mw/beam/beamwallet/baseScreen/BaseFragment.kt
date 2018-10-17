package com.mw.beam.beamwallet.baseScreen

import android.app.Activity
import android.support.design.widget.Snackbar
import android.support.v4.app.Fragment
import android.support.v4.content.ContextCompat
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import com.mw.beam.beamwallet.R
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

    override fun onResume() {
        super.onResume()
        presenter.onResume()
    }

    override fun onPause() {
        presenter.onPause()
        super.onPause()
    }

    override fun onDestroy() {
        presenter.detachView()

        super.onDestroy()
    }

    override fun hideKeyboard() {
        val imm = activity?.getSystemService(Activity.INPUT_METHOD_SERVICE) as InputMethodManager
        imm.hideSoftInputFromWindow(activity?.findViewById<View>(android.R.id.content)?.windowToken, 0)
    }

    override fun configNavDrawer() {}

    override fun showSnackBar(status: AppConfig.Status) {
        showSnackBar(
                when (status) {
                    AppConfig.Status.STATUS_OK -> getString(R.string.common_successful)
                    AppConfig.Status.STATUS_ERROR -> getString(R.string.common_error)
                }
        )
    }

    override fun showSnackBar(message: String) {
        val context = context ?: return
        val snackBar = Snackbar.make(activity?.findViewById(android.R.id.content) ?: return,
                message, Snackbar.LENGTH_LONG)
        snackBar.view.setBackgroundColor(ContextCompat.getColor(context, R.color.snack_bar_color))
        snackBar.view.findViewById<TextView>(android.support.design.R.id.snackbar_text).setTextColor(ContextCompat.getColor(context, R.color.colorAccent))
        snackBar.show()
    }
}
