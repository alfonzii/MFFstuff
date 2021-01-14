package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.SocketException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/*port na ktorom prebieha stahovanie suborov bude konstanta 34999.
 * Eventualny upgrade moze byt, ze sa bude pozerat na to, ktore z portov napr. v intervale od
 * 34900-35000 su volne a pouzije sa nejaky volny z nich. Je to teda predpripravene na rozsirovanie
 * v pokrocilej Jave. */

class Server implements Runnable {

    /*pocet vlakien na CPU (server bude obsluhovat iba tolko clientov, kolko ma
     * vlakien [kvoli vyuzivaniu viac seedov a + nejake rozumne horne ohranicenie ktore dava zmysel]) */
    static final int numOfThreads = Runtime.getRuntime().availableProcessors();
    static AtomicInteger usedThreads = new AtomicInteger();

    private ServerSocket serverSocket = null;
    private Map<Long, NodeServerThread> createdThreads = new ConcurrentHashMap<>();

    @Override
    public void run() {
        try {
            serverSocket = new ServerSocket(P2P.SERVER_SEED_PORT);
            while (true) {
                NodeServerThread thread = new NodeServerThread(serverSocket.accept(), createdThreads);
                createdThreads.put(thread.getId(), thread);
                thread.start();
                usedThreads.getAndIncrement();
            }
        } catch (SocketException e) {
            System.out.println("Server side socket closed. You are not seeding anymore.");
        } catch (IOException e) {
            System.err.println("Server side of node could not listen on port ");
            System.exit(-1);
        } finally {
            try {
                assert serverSocket != null; //nastat by to mohlo len v pripade, ze su pustene 2 appky na tom istom kompe
                serverSocket.close();
            } catch (IOException e) {
                System.err.println("Wasn't able to close server socket on my node.");
            }
        }
    }

    void closeServerSocket() {
        try {
            synchronized (createdThreads) {
                for (Map.Entry<Long, NodeServerThread> t : createdThreads.entrySet()) {
                    t.getValue().closeSocketToClient();
                }
                createdThreads.clear();
            }
            serverSocket.close();
        } catch (IOException e) {
            System.err.println("Exception occured while trying to close server side socket.");
        }
    }


}
