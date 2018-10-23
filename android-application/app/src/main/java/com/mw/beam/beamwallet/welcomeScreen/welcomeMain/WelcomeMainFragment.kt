package com.mw.beam.beamwallet.welcomeScreen.welcomeMain

import android.annotation.SuppressLint
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseFragment
import kotlinx.android.synthetic.main.fragment_welcome_main.*

/**
 * Created by vain onnellinen on 10/19/18.
 */
class WelcomeMainFragment : BaseFragment<WelcomeMainPresenter>(), WelcomeMainContract.View {
    private lateinit var presenter: WelcomeMainPresenter

    companion object {
        fun newInstance(): WelcomeMainFragment {
            val args = Bundle()
            val fragment = WelcomeMainFragment()
            fragment.arguments = args

            return fragment
        }

        fun getFragmentTag(): String {
            return WelcomeMainFragment::class.java.simpleName
        }
    }

    @SuppressLint("InflateParams")
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return layoutInflater.inflate(R.layout.fragment_welcome_main, null)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        presenter = WelcomeMainPresenter(this, WelcomeMainRepository())
        configPresenter(presenter)

        btnCreate.setOnClickListener {
            presenter.onCreateWallet()
        }

        btnRestore.setOnClickListener {
            presenter.onRestoreWallet()
        }
    }

    override fun createWallet() {
        (activity as OnCreateWallet).createWallet()
    }

    interface OnCreateWallet {
        fun createWallet()
    }
}
