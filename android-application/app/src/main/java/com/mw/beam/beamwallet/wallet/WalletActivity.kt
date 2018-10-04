package com.mw.beam.beamwallet.wallet

import android.os.Bundle
import android.support.v7.widget.LinearLayoutManager
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseActivity
import com.mw.beam.beamwallet.core.TxDescription
import com.mw.beam.beamwallet.core.Wallet
import kotlinx.android.synthetic.main.activity_wallet.*

/**
 * Created by vain onnellinen on 10/1/18.
 */
class WalletActivity : BaseActivity<WalletPresenter>(), WalletContract.View {
    private lateinit var presenter: WalletPresenter
    private lateinit var adapter: TransactionsAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_wallet)

        presenter = WalletPresenter(this, WalletModel())
        configPresenter(presenter)
    }

    override fun configData(wallet: Wallet) {
        adapter.setData(wallet.getTxHistory())
    }

    override fun init() {
        adapter = TransactionsAdapter(this, arrayOf(), object : TransactionsAdapter.OnItemClickListener {
            override fun onItemClick(item: TxDescription) {

            }

            override fun onSwipeClick(item: TxDescription) {

            }
        })
        transactionsList.layoutManager = LinearLayoutManager(this)
        transactionsList.adapter = adapter
    }
}
