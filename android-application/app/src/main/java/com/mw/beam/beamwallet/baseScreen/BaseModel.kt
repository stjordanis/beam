package com.mw.beam.beamwallet.baseScreen

import com.mw.beam.beamwallet.core.App
import com.mw.beam.beamwallet.core.Wallet

/**
 * Created by vain onnellinen on 10/1/18.
 */
open class BaseModel {
    val wallet: Wallet
        get() = App.wallet

}
