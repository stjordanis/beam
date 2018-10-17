package com.mw.beam.beamwallet.core.entities

import com.mw.beam.beamwallet.core.helpers.EntitiesHelper

/**
 * Created by vain onnellinen on 10/2/18.
 */
class TxDescription(val id: ByteArray,
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
    val senderEnum: EntitiesHelper.TxSender
        get() = EntitiesHelper.TxSender.fromValue(sender)
    val statusEnum: EntitiesHelper.TxStatus
        get() = EntitiesHelper.TxStatus.fromValue(status)
}
