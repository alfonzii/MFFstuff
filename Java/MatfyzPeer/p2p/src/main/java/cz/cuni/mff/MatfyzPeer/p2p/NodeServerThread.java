package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.Socket;
import java.net.SocketException;
import java.util.Map;

public class NodeServerThread extends Thread {
    private Socket socket;
    private Map<Long, NodeServerThread> threadMap;


    NodeServerThread(Socket socket, Map<Long, NodeServerThread> threadMap) {
        super("p2p.NodeServerThread");
        this.socket = socket;
        this.threadMap = threadMap;
    }

    @Override
    public void run() {
        try (
                ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
                ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
        ) {
            ServersideSLProtocol protocol = new ServersideSLProtocol();

            while (true) {
                try {
                    Object clientQuery = ois.readUnshared();
                    /* v pripade ze inputLine obsahuje "BYE", tak sa do outputLine ulozi nejaky junk kvoli
                    jednoduchosti kodu. Ajtak nas to zaujimat nebude v takom pripade. */
                    Object myAnswer = protocol.processInput(clientQuery);
                    if (protocol.bye()) { //klient ukoncil komunikaciu so serverom prikazom "BYE"
                        break;
                    }
                    oos.writeUnshared(myAnswer);
                    oos.reset();
                    oos.flush();
                } catch (SeedConnection con) { //ak som L a pripoji sa na mna S
                    P2P.IPs.add(socket.getInetAddress());
                    Object myAnswer = 'L';
                    oos.writeUnshared(myAnswer);
                    oos.reset();
                    oos.flush();
                }
            }
        } catch (SocketException e) { //nejde rozlisit rozdiel medzi odpojenim remote a tym ked to vypnem ja
            System.out.println("Client disconnected violently, but we don't care. It's his business.");
        } catch (IOException e) { //klient sa odpojil alebo nastala nejaka IO chyba
            System.err.println("IOException in NodeServerThread.");
        } catch (ClassNotFoundException e) {
            System.err.println("Nasty exception: " + e.getClass().getSimpleName() + ", which shouldn't happen");
        } finally {
            try {
                Server.usedThreads.decrementAndGet(); //pri ukonceni spojenia vycistime threadcounter
                threadMap.remove(this.getId()); //vymazem sa z mapy spustenych threadov
                socket.close(); //uzavretie komunikacie s klientom, nech k ukonceniu doslo akokolvek
                //teda ci po chybe (odpojil sa tvrdo), alebo ukoncil prikazom "BYE"
                System.out.println("Correctly disconnected client from my serverside."); //debug hlaska
            } catch (IOException e) {
                System.err.println("Wasn't able to close socket on my side in thread " + this.getName());
            }
        }

    }

    void closeSocketToClient() {
        try {
            socket.close();
        } catch (IOException e) {
            System.err.println("Exception occured while trying to close socket in thread " +
                    this.getName() + ", which was operating with client " + socket.getInetAddress());
        }
    }

}
