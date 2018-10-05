package com.mw.beam.beamwallet.baseScreen

import android.app.Activity
import android.content.res.Configuration
import android.os.Bundle
import android.os.PersistableBundle
import android.support.design.widget.Snackbar
import android.support.v4.app.Fragment
import android.support.v4.app.FragmentManager
import android.support.v4.app.FragmentTransaction
import android.support.v4.content.ContextCompat
import android.support.v4.view.GravityCompat
import android.support.v7.app.ActionBarDrawerToggle
import android.support.v7.app.AppCompatActivity
import android.view.Gravity
import android.view.MenuItem
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.TextView
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.core.AppConfig
import com.mw.beam.beamwallet.core.utils.LogUtils
import com.mw.beam.beamwallet.wallet.WalletFragment
import kotlinx.android.synthetic.main.activity_main.*

/**
 * Created by vain onnellinen on 10/1/18.
 */
abstract class BaseActivity<T : BasePresenter<out MvpView>> : AppCompatActivity(), MvpView {
    private lateinit var presenter: T
    private lateinit var drawerToggle: ActionBarDrawerToggle

    fun configPresenter(presenter: T) {
        this.presenter = presenter
        this.presenter.viewIsReady()
    }

    private fun showFragment(
            fragment: Fragment,
            tag: String,
            clearToTag: String?,
            clearInclusive: Boolean
    ) {
        drawerLayout.closeDrawer(Gravity.START)
        val fragmentManager = supportFragmentManager
        val currentFragment = supportFragmentManager.findFragmentById(R.id.container)

        if (currentFragment == null || tag != fragment.tag) {
            if (clearToTag != null || clearInclusive) {
                fragmentManager.popBackStack(
                        clearToTag,
                        if (clearInclusive) FragmentManager.POP_BACK_STACK_INCLUSIVE else 0
                )
            }

            val transaction = fragmentManager.beginTransaction()
            transaction.replace(R.id.container, fragment, tag)
            transaction.addToBackStack(tag)
            transaction.setTransition(FragmentTransaction.TRANSIT_FRAGMENT_FADE)
            transaction.commit()
        }
    }

    override fun configNavDrawer() {
        setSupportActionBar(toolbarLayout.findViewById(R.id.toolbar))
        supportActionBar?.setDisplayShowTitleEnabled(false)
        drawerToggle = ActionBarDrawerToggle(this, drawerLayout, toolbarLayout.findViewById(R.id.toolbar), R.string.common_drawer_open, R.string.common_drawer_close)
        drawerLayout.addDrawerListener(drawerToggle)
        drawerToggle.syncState()
        navView.itemIconTintList = getColorStateList(R.color.menu_selector)
        navView.setNavigationItemSelectedListener { menuItem ->
            when (menuItem.itemId) {
                R.id.nav_dashboard -> LogUtils.log("dashboard")
                R.id.nav_wallet -> showFragment(WalletFragment.newInstance(), WalletFragment.getFragmentTag(), null, true)
                R.id.nav_address_book -> LogUtils.log("address_book")
                R.id.nav_utxo -> LogUtils.log("utxo")
                R.id.nav_blockchain_info -> LogUtils.log("blockchain_info")
                R.id.nav_notifications -> LogUtils.log("notifications")
                R.id.nav_help -> LogUtils.log("help")
                R.id.nav_settings -> LogUtils.log("settings")
            }
            true
        }
        navView.apply {
            setCheckedItem(R.id.nav_wallet)
            menu.performIdentifierAction(R.id.nav_wallet, 0)
        }
    }

    override fun showSnackBar(status: AppConfig.Status) {
        val snackBar = Snackbar.make(findViewById(android.R.id.content),
                when (status) {
                    AppConfig.Status.STATUS_OK -> getString(R.string.common_successful)
                    AppConfig.Status.STATUS_ERROR -> getString(R.string.common_error)
                }, Snackbar.LENGTH_LONG)
        snackBar.view.setBackgroundColor(ContextCompat.getColor(this, R.color.snack_bar_color))
        snackBar.view.findViewById<TextView>(android.support.design.R.id.snackbar_text).setTextColor(ContextCompat.getColor(this, R.color.colorAccent))
        snackBar.show()
    }

    override fun hideKeyboard() {
        val imm = this.getSystemService(Activity.INPUT_METHOD_SERVICE) as InputMethodManager
        imm.hideSoftInputFromWindow(findViewById<View>(android.R.id.content).windowToken, 0)
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

    override fun onBackPressed() {
        if (drawerLayout.isDrawerVisible(GravityCompat.START)) {
            drawerLayout.closeDrawer(GravityCompat.START)
            return
        }

        if (supportFragmentManager.backStackEntryCount == 1) {
            finish()
            return
        }

        super.onBackPressed()
    }

    override fun onPostCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {
        super.onPostCreate(savedInstanceState, persistentState)
        drawerToggle.syncState()
    }

    override fun onConfigurationChanged(newConfig: Configuration?) {
        super.onConfigurationChanged(newConfig)
        drawerToggle.onConfigurationChanged(newConfig)
    }

    override fun onOptionsItemSelected(item: MenuItem?): Boolean {
        if (drawerToggle.onOptionsItemSelected(item)) {
            return true
        }

        return super.onOptionsItemSelected(item)
    }
}
