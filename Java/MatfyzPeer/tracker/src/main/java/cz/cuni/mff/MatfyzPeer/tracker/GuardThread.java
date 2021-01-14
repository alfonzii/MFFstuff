package cz.cuni.mff.MatfyzPeer.tracker;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.net.InetAddress;
import java.net.Socket;

public class GuardThread extends Thread {

    private Socket socket;
    private InetAddress ip;
    private String fileName;
    private ObjectInputStream ois;

    GuardThread(Socket socket, InetAddress ip, String fileName, ObjectInputStream ois) {
        super("tracker.GuardThread " + ip);
        this.socket = socket;
        this.ip = ip;
        this.fileName = fileName;
        this.ois = ois;
    }

    @Override
    public void run() {
        try {
            ois.readObject(); //zablokujem sa. Ked sa odpoji node tak nam hodi hned exception
        } catch (IOException e) {
            synchronized (Tracker.class) {
                Tracker.DBsocket.remove(ip);
                Tracker.DBsubor.get(fileName).remove(ip);
                if (Tracker.DBsubor.get(fileName).isEmpty())
                    Tracker.DBsubor.remove(fileName);
            }
        } catch (ClassNotFoundException e) {
            System.err.println("ClassNotFoundException thrown in " + this.getName());
        } finally {
            try {
                socket.close();
            } catch (IOException e) {
                System.err.println("Wasn't able to close socket on my side in thread " + this.getName());
            }
        }
    }
}






