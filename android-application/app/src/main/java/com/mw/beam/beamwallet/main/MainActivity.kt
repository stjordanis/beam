package com.mw.beam.beamwallet.main

import android.os.Bundle
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseActivity

/**
 * Created by vain onnellinen on 10/1/18.
 */
class MainActivity : BaseActivity<MainPresenter>() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }
}
