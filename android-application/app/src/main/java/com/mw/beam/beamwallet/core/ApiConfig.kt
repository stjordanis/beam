package com.mw.beam.beamwallet.core

/**
 * Created by vain onnellinen on 10/1/18.
 */
object ApiConfig {
    var DB_PATH = ""

    enum class Status(val value : Int) {
        STATUS_OK (0), STATUS_ERROR (-1);

        companion object {
            private val map: MutableMap<Int, Status> = HashMap()

            init {
                Status.values().forEach {
                    map[it.value] = it
                }
            }

            fun fromValue(type: Int?): Status {
                return map[type] ?: STATUS_ERROR
            }
        }
    }
}
