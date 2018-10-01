package com.senlainc.beamwallet.core;

public class Api
{
	public native int createWallet(String dbName, String pass, String seed);
	public native int openWallet(String dbName, String pass);
	public native boolean isWalletInitialized(String dbName);
	public native void closeWallet(int wallet);

	static 
    {
	    System.loadLibrary("wallet-jni");
	}
}
