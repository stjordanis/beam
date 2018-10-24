package com.mw.beam.beamwallet.welcomeScreen.welcomePasswords

import android.annotation.SuppressLint
import android.os.Bundle
import android.text.Editable
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseFragment
import com.mw.beam.beamwallet.core.views.PasswordStrengthView
import com.mw.beam.beamwallet.core.watchers.TextWatcher
import kotlinx.android.synthetic.main.fragment_welcome_passwords.*

/**
 * Created by vain onnellinen on 10/23/18.
 */
class WelcomePasswordsFragment : BaseFragment<WelcomePasswordsPresenter>(), WelcomePasswordsContract.View {
    private lateinit var presenter: WelcomePasswordsPresenter

    companion object {
        fun newInstance(): WelcomePasswordsFragment {
            val args = Bundle()
            val fragment = WelcomePasswordsFragment()
            fragment.arguments = args

            return fragment
        }

        fun getFragmentTag(): String {
            return WelcomePasswordsFragment::class.java.simpleName
        }
    }

    @SuppressLint("InflateParams")
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return layoutInflater.inflate(R.layout.fragment_welcome_passwords, null)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        presenter = WelcomePasswordsPresenter(this, WelcomePasswordsRepository())
        configPresenter(presenter)

        pass.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(password: Editable?) {
                presenter.onPassChanged(password?.toString())
            }
        })

        confirmPass.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(password: Editable?) {
                passMatchError.visibility = View.GONE
            }
        })
    }

    override fun hasErrors(): Boolean {
        var hasErrors = false
        pass.error = null
        confirmPass.error = null
        passMatchError.visibility = View.GONE

        if (pass.text.isNullOrBlank()) {
            pass.error = getString(R.string.open_wallet_no_text_error)
            hasErrors = true
        }

        if (!pass.text.isNullOrBlank() && pass.text.toString() != confirmPass.text.toString()) {
            passMatchError.visibility = View.VISIBLE
            hasErrors = true
        }

        if (confirmPass.text.isNullOrBlank()) {
            confirmPass.error = getString(R.string.open_wallet_no_text_error)
            hasErrors = true
        }

        return hasErrors
    }

    override fun setStrengthLevel(strength: PasswordStrengthView.Strength) {
        strengthView.strength = strength
    }
}
