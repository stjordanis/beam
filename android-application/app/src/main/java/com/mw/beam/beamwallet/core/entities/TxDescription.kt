package com.mw.beam.beamwallet.core.entities

/**
 * Created by vain onnellinen on 10/2/18.
 */
data class TxDescription(val id: ByteArray,
                         val amount: Long,
                         val fee: Long,
                         val change: Long,
                         val minHeight: Long,
                         val peerId: ByteArray,
                         val myId: ByteArray,
                         val message: ByteArray?,
                         val createTime: Long,
                         val modifyTime: Long,
                         val sender: Boolean,
                         val status: Int) {
}
