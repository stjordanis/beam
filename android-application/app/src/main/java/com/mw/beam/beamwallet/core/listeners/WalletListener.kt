package com.mw.beam.beamwallet.core.listeners

import io.reactivex.subjects.PublishSubject
import io.reactivex.subjects.Subject


/**
 * Created by vain onnellinen on 10/4/18.
 */
object WalletListener {
    private val DUMMY_OBJECT = Any()
    var subOnKeychainChanged: Subject<Any> = PublishSubject.create<Any>().toSerialized()
    var subOnTransactionChanged: Subject<Any> = PublishSubject.create<Any>().toSerialized()
    var subOnSystemStateChanged: Subject<Any> = PublishSubject.create<Any>().toSerialized()
    var subOnTxPeerChanged: Subject<Any> = PublishSubject.create<Any>().toSerialized()
    var subOnAddressChanged: Subject<Any> = PublishSubject.create<Any>().toSerialized()
    var subOnSyncProgress: Subject<Any> = PublishSubject.create<Any>().toSerialized()

    @JvmStatic
    fun onKeychainChanged() = subOnKeychainChanged.onNext(DUMMY_OBJECT)

    @JvmStatic
    fun onTransactionChanged() = subOnTransactionChanged.onNext(DUMMY_OBJECT)

    @JvmStatic
    fun onSystemStateChanged() = subOnSystemStateChanged.onNext(DUMMY_OBJECT)

    @JvmStatic
    fun onTxPeerChanged() = subOnTxPeerChanged.onNext(DUMMY_OBJECT)

    @JvmStatic
    fun onAddressChanged() = subOnAddressChanged.onNext(DUMMY_OBJECT)

    @JvmStatic
    fun onSyncProgress(done: Int, total: Int) = subOnSyncProgress.onNext(DUMMY_OBJECT)
}
