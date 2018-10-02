package com.mw.beam.beamwallet.core

import android.app.Application
import com.crashlytics.android.Crashlytics
import com.mw.beam.beamwallet.BuildConfig
import com.squareup.leakcanary.LeakCanary
import io.fabric.sdk.android.Fabric

/**
 * Created by vain onnellinen on 10/1/18.
 */
class App : Application() {

    companion object {
        lateinit var self: App
    }

    override fun onCreate() {
        super.onCreate()
        Fabric.with(this, Crashlytics())

        if (LeakCanary.isInAnalyzerProcess(this)) {
            // This process is dedicated to LeakCanary for heap analysis.
            // You should not init your app in this process.
            return
        }

        self = this
        ApiConfig.DB_PATH = filesDir.absolutePath

        if (BuildConfig.DEBUG) {
            LeakCanary.install(self)
        }
    }
}
