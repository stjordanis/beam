package com.mw.beam.beamwallet.welcomeScreen

import android.content.Intent
import android.os.Bundle
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseActivity
import com.mw.beam.beamwallet.main.MainActivity
import com.mw.beam.beamwallet.welcomeScreen.welcomeDescription.WelcomeDescriptionFragment
import com.mw.beam.beamwallet.welcomeScreen.welcomeMain.WelcomeMainFragment
import com.mw.beam.beamwallet.welcomeScreen.welcomePasswords.WelcomePasswordsFragment

/**
 * Created by vain onnellinen on 10/19/18.
 */
class WelcomeActivity : BaseActivity<WelcomePresenter>(), WelcomeContract.View, WelcomeMainFragment.WelcomeMainHandler, WelcomeDescriptionFragment.OnGeneratePhrase {
    private lateinit var presenter: WelcomePresenter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_welcome)

        presenter = WelcomePresenter(this, WelcomeRepository())
        configPresenter(presenter)
    }

    override fun showWelcomeMainFragment() {
        showFragment(WelcomeMainFragment.newInstance(), WelcomeMainFragment.getFragmentTag(), WelcomeMainFragment.getFragmentTag(), true)
    }

    override fun showDescriptionFragment() {
        showFragment(WelcomeDescriptionFragment.newInstance(), WelcomeDescriptionFragment.getFragmentTag(), null, false)
    }

    override fun showPasswordsFragment() {
        showFragment(WelcomePasswordsFragment.newInstance(), WelcomePasswordsFragment.getFragmentTag(), null, false)
    }

    override fun showMainActivity() {
        startActivity(Intent(this, MainActivity::class.java))
    }

    override fun createWallet() {
        presenter.onCreateWallet()
    }

    override fun generatePhrase() {
        presenter.onGeneratePhrase()
    }

    override fun openWallet() {
        presenter.onOpenWallet()
    }
}
