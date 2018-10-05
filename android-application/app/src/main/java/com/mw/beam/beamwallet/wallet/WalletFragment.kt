package com.mw.beam.beamwallet.wallet

import android.annotation.SuppressLint
import android.os.Bundle
import android.support.v7.widget.LinearLayoutManager
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseFragment
import com.mw.beam.beamwallet.core.entities.TxDescription
import com.mw.beam.beamwallet.core.helpers.EntitiesHelper
import kotlinx.android.synthetic.main.fragment_wallet.*

/**
 * Created by vain onnellinen on 10/1/18.
 */
class WalletFragment : BaseFragment<WalletPresenter>(), WalletContract.View {
    private lateinit var presenter: WalletPresenter
    private lateinit var adapter: TransactionsAdapter

    companion object {
        fun newInstance(): WalletFragment {
            val args = Bundle()
            val fragment = WalletFragment()
            fragment.arguments = args

            return fragment
        }

        fun getFragmentTag(): String {
            return WalletFragment::class.java.simpleName
        }
    }

    @SuppressLint("InflateParams")
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return layoutInflater.inflate(R.layout.fragment_wallet, null)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        presenter = WalletPresenter(this, WalletModel())
        configPresenter(presenter)
    }

    override fun configTxHistory(txHistory: Array<TxDescription>) {
        adapter.setData(txHistory)
    }

    override fun configAvailable(availableSum: Long) {
        available.text = EntitiesHelper.convertToBeam(availableSum).toString()
    }

    override fun init() {
        val context = context ?: return
        adapter = TransactionsAdapter(context, arrayOf(), object : TransactionsAdapter.OnItemClickListener {
            override fun onItemClick(item: TxDescription) {
            }

            override fun onSwipeClick(item: TxDescription) {

            }
        })
        transactionsList.layoutManager = LinearLayoutManager(context)
        transactionsList.adapter = adapter
    }
}
