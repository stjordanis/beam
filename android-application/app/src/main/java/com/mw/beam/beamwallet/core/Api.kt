package com.mw.beam.beamwallet.core

/**
 * Created by vain onnellinen on 10/1/18.
 */
object Api {
    init {
        System.loadLibrary("wallet-jni")
    }

    external fun createWallet(dbPath: String, pass: String, seed: String): Int
    external fun openWallet(dbPath: String, pass: String): Int
    external fun isWalletInitialized(dbPath: String): Boolean
    external fun closeWallet(wallet : Int)
}
