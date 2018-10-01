import com.mw.beam.beamwallet.core.*;

public class WalletJNI
{
	public static void main(String[] args)
	{
        System.out.println("Start Wallet JNI test...");

		Api api = new Api();
        int wallet = -1;

        if(api.isWalletInitialized("."))
        {
            wallet = api.openWallet(".", "pass123");

            System.out.println(wallet < 0 ? "wallet opening error" : "wallet successfully opened");
        }
        else
        {
            wallet = api.createWallet(".", "pass123", "seed123");

            System.out.println(wallet < 0 ? "wallet creation error" : "wallet successfully created");
        }

        if(wallet >= 0)
        {
            SystemState state = api.getSystemState(wallet);

            System.out.println("system state is " + state.height);
            System.out.println("system hash length is " + state.hash.length);

            api.closeWallet(wallet);
        }
	}
}
