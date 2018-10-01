package com.mw.beam.beamwallet.core;

public class Utxo
{
	public long id;
	public long amount;
	public int status;
	public long createHeight;
	public long maturity;
	public int keyType;
	public long confirmHeight;
	public byte[] confirmHash;
	public long lockHeight;
	public byte[] createTxId;
	public byte[] spendTxId;
}
