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
            wallet = api.openWallet(".", "123");

            System.out.println(wallet < 0 ? "wallet opening error" : "wallet successfully opened");
        }
        else
        {
            wallet = api.createWallet(".", "123", "seed123");

            System.out.println(wallet < 0 ? "wallet creation error" : "wallet successfully created");
        }

        if(wallet >= 0)
        {
            {
                SystemState state = api.getSystemState(wallet);
                System.out.println("system height is " + state.height);
            }

            {
                Utxo[] utxos = api.getUtxos(wallet);

                System.out.println("utxos length: " + utxos.length);

                System.out.println("+-------------------------------------------------------");
                System.out.println("| id:   | amount:       | type:");
                System.out.println("+-------+---------------+-------------------------------");

                for(int i = 0; i < utxos.length; i++)
                {
                    System.out.println("| " + utxos[i].id 
                        + "\t| "  + utxos[i].amount
                        + "\t| "  + utxos[i].keyType);
                }

                System.out.println("+-------------------------------------------------------");
            }

            api.closeWallet(wallet);
        }
	}
}
