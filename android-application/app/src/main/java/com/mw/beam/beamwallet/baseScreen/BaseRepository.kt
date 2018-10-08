package com.mw.beam.beamwallet.baseScreen

import com.mw.beam.beamwallet.core.App
import com.mw.beam.beamwallet.core.entities.Wallet
import io.reactivex.Observable
import io.reactivex.ObservableOnSubscribe
import io.reactivex.android.schedulers.AndroidSchedulers

/**
 * Created by vain onnellinen on 10/1/18.
 */
open class BaseRepository : MvpRepository {

    override fun getWallet(): Wallet {
        return App.wallet
    }

    fun <T> createObservable(block: () -> Unit): Observable<T> {
        return Observable
                .create(ObservableOnSubscribe<T> { block() })
                .subscribeOn(AndroidSchedulers.mainThread())
                //TODO return when multithreading will work
//                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
    }
}
