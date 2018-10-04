package com.mw.beam.beamwallet.openWallet

import android.content.Intent
import android.os.Bundle
import android.view.View
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseActivity
import com.mw.beam.beamwallet.wallet.WalletActivity
import kotlinx.android.synthetic.main.activity_open_wallet.*

/**
 * Created by vain onnellinen on 10/1/18.
 */
class OpenWalletActivity : BaseActivity<OpenWalletPresenter>(), OpenWalletContract.View {
    private lateinit var presenter: OpenWalletPresenter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_open_wallet)

        presenter = OpenWalletPresenter(this, OpenWalletModel())
        configPresenter(presenter)

        btnProceed.setOnClickListener {
            presenter.onProceedPressed()
        }
    }

    override fun hasErrors(): Boolean {
        var hasErrors = false
        seed.error = null
        pass.error = null
        confirmPass.error = null

        if (seed.text.isNullOrBlank()) {
            seed.error = getString(R.string.open_wallet_no_text_error)
            hasErrors = true
        }

        if (pass.text.isNullOrBlank()) {
            pass.error = getString(R.string.open_wallet_no_text_error)
            hasErrors = true
        }

        if (!pass.text.isNullOrBlank() && pass.text.toString() != confirmPass.text.toString()) {
            confirmPass.error = getString(R.string.open_wallet_pass_not_match_error)
            hasErrors = true
        }

        if (confirmPass.text.isNullOrBlank()) {
            confirmPass.error = getString(R.string.open_wallet_no_text_error)
            hasErrors = true
        }

        return hasErrors
    }

    override fun hasValidPass(): Boolean = !pass.text.isNullOrBlank()
    override fun getPass(): String = pass.text.trim().toString()
    override fun getSeed(): String = seed.text.trim().toString()

    override fun configForCreatingWallet() {
        walletTitle.text = getString(R.string.open_wallet_title_create)
    }

    override fun configForOpeningWallet() {
        walletTitle.text = getString(R.string.open_wallet_title_open)
        seed.visibility = View.GONE
        confirmPass.visibility = View.GONE
    }

    override fun startMainActivity() {
        startActivity(Intent(this, WalletActivity::class.java))
    }
}
