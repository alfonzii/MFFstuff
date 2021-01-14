package cz.cuni.mff.MatfyzPeer.tracker;

import javafx.util.Pair;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;


public class Tracker {

    //IP adresy namapovane k suboru (hashu)
    static Map<String, Set<InetAddress>> DBsubor = (new ConcurrentHashMap<>());
    //dvojica socket a subor namapovana k IP adrese
    static Map<InetAddress, Pair<Socket, String>> DBsocket = (new ConcurrentHashMap<>());

    public static void main(String[] args) {
        if (args.length != 1) {
            System.err.println("Usage: java tracker.Tracker <port number>");
            System.exit(1);
        }

        int portNumber = Integer.parseInt(args[0]);

        try (ServerSocket serverSocket = new ServerSocket(portNumber)) {
            while (true) {
                new ConnectionThread(serverSocket.accept()).start();
            }
        } catch (IOException e) {
            System.err.println("Could not listen on port " + portNumber);
            System.exit(-1);
        }
    }

}
