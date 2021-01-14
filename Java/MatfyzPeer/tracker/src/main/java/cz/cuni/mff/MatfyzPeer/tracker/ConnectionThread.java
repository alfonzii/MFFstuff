package cz.cuni.mff.MatfyzPeer.tracker;

import javafx.util.Pair;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.net.Socket;
import java.util.Collections;
import java.util.HashSet;

public class ConnectionThread extends Thread {
    private Socket socket;

    ConnectionThread(Socket socket) {
        super("tracker.ConnectionThread");
        this.socket = socket;
    }

    private void sendObject(Object o) throws IOException {
        ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
        oos.writeUnshared(o);
        oos.reset();
        oos.flush();
    }

    @Override
    public synchronized void run() {
        try {
            ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
            String fileHash = (String) ois.readUnshared();
            if (Tracker.DBsubor.containsKey(fileHash)) {
                sendObject(Tracker.DBsubor.get(fileHash));
                Tracker.DBsubor.get(fileHash).add(socket.getInetAddress());
                Tracker.DBsocket.put(socket.getInetAddress(), new Pair<>(socket, fileHash));

            } else { //initial seeder alebo leech, co chce stahovat "neexistujuci" subor
                Tracker.DBsubor.put(fileHash, new HashSet<>(Collections.singletonList(socket.getInetAddress())));
                Tracker.DBsocket.put(socket.getInetAddress(), new Pair<>(socket, fileHash));
                sendObject(new HashSet<>());
            }
            new GuardThread(socket, socket.getInetAddress(), fileHash, ois).start();
        } catch (Exception e) {
            System.err.println("Unexpected exception in " + this.getName());
        }
    }
}
