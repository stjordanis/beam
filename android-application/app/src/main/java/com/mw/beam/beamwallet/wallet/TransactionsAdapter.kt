package com.mw.beam.beamwallet.wallet

import android.content.Context
import android.support.v4.content.ContextCompat
import android.support.v7.widget.RecyclerView
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.core.entities.TxDescription
import com.mw.beam.beamwallet.core.helpers.EntitiesHelper
import com.mw.beam.beamwallet.core.utils.CalendarUtils
import kotlinx.android.extensions.LayoutContainer
import kotlinx.android.synthetic.main.item_transaction.*

/**
 * Created by vain onnellinen on 10/2/18.
 */
class TransactionsAdapter(private val context: Context, private var data: Array<TxDescription>, private val clickListener: OnItemClickListener) :
        RecyclerView.Adapter<TransactionsAdapter.ViewHolder>() {
    private val receivedResId = R.drawable.ic_received
    private val receivedColor = ContextCompat.getColor(context, R.color.received_color)
    private val receivedText = context.getString(R.string.wallet_status_received)
    private val sentResId = R.drawable.ic_sent
    private val sentColor = ContextCompat.getColor(context, R.color.sent_color)
    private val sentText = context.getString(R.string.wallet_status_sent)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) = ViewHolder(LayoutInflater.from(context).inflate(R.layout.item_transaction, parent, false))

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val transaction = data[position]

        holder.apply {
            when (EntitiesHelper.TxSender.fromValue(transaction.sender)) {
                EntitiesHelper.TxSender.RECEIVED -> {
                    icon.setImageResource(receivedResId)
                    sum.setTextColor(receivedColor)
                    status.setTextColor(receivedColor)
                    status.text = receivedText
                }
                EntitiesHelper.TxSender.SENT -> {
                    icon.setImageResource(sentResId)
                    sum.setTextColor(sentColor)
                    status.setTextColor(sentColor)
                    status.text = sentText
                }
            }

            date.text = CalendarUtils.fromTimestamp(transaction.modifyTime)
            sum.text = EntitiesHelper.convertToBeam(transaction.amount).toString()
            message.visibility = if (transaction.message == null) View.GONE else View.VISIBLE
            if (transaction.message != null) {
                message.text = String(transaction.message)
            }
        }
    }

    override fun getItemCount(): Int = data.size
    fun getPositionByItem(item: TxDescription) = data.indexOf(item)

    fun setData(data: Array<TxDescription>) {
        this.data = data
        notifyDataSetChanged()
    }

    interface OnItemClickListener {
        fun onItemClick(item: TxDescription)
        fun onSwipeClick(item: TxDescription)
    }

    class ViewHolder(override val containerView: View) : RecyclerView.ViewHolder(containerView), LayoutContainer
}
