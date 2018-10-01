package com.mw.beam.beamwallet.core;

public class Api
{
	public native int createWallet(String path, String pass, String seed);
	public native int openWallet(String path, String pass);
	public native boolean isWalletInitialized(String path);
	public native void closeWallet(int wallet);
    public native void changeWalletPassword(int wallet, String pass);
    public native SystemState getSystemState(int wallet);
    public native Utxo[] getUtxos(int wallet);

	static 
    {
	    System.loadLibrary("wallet-jni");
	}
}
