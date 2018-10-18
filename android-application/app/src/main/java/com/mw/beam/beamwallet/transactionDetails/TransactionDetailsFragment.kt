package com.mw.beam.beamwallet.transactionDetails

import android.annotation.SuppressLint
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import com.mw.beam.beamwallet.R
import com.mw.beam.beamwallet.baseScreen.BaseFragment
import com.mw.beam.beamwallet.core.entities.TxDescription






/**
 * Created by vain onnellinen on 10/17/18.
 */
class TransactionDetailsFragment : BaseFragment<TransactionDetailsPresenter>(), TransactionDetailsContract.View {
    private lateinit var presenter: TransactionDetailsPresenter
    private lateinit var txDescription: TxDescription

    companion object {
        private const val EXTRA_TX_DESCRIPTION = "EXTRA_TX_DESCRIPTION"

        fun newInstance(txDescription: TxDescription): TransactionDetailsFragment {
            val args = Bundle()
            val fragment = TransactionDetailsFragment()
            args.putParcelable(EXTRA_TX_DESCRIPTION, txDescription)
            fragment.arguments = args

            return fragment
        }

        fun getFragmentTag(): String {
            return TransactionDetailsFragment::class.java.simpleName
        }
    }

    @SuppressLint("InflateParams")
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return layoutInflater.inflate(R.layout.fragment_transaction_details, null)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        presenter = TransactionDetailsPresenter(this, TransactionDetailsRepository())
        configPresenter(presenter)

        txDescription = arguments?.getParcelable(EXTRA_TX_DESCRIPTION) ?: return
    }
}
