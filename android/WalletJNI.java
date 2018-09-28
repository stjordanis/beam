import com.senlainc.beamwallet.core.Api;

public class WalletJNI
{
	public static void main(String[] args)
	{
        System.out.println("Start Wallet JNI test...");

		Api api = new Api();
        int wallet = -1;

        if(api.isWalletInitialized("wallet.db"))
        {
            wallet = api.openWallet("wallet.db", "pass123");

            System.out.println(wallet < 0 ? "wallet opening error" : "wallet successfully opened");
        }
        else
        {
            wallet = api.createWallet("wallet.db", "pass123", "seed123");

            System.out.println(wallet < 0 ? "wallet creation error" : "wallet successfully created");
        }

        if(wallet >= 0)
            api.closeWallet(wallet);
	}
}
