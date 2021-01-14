package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.*;
import java.net.ConnectException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.util.*;

class Client {

    private Socket trackerSocket;
    private File helpFile;

    Client() {
        if (P2P.state == 'L') {
            helpFile = new File(P2P.pathToDownload + File.separator + "help-" + P2P.name);
            try (
                    FileOutputStream fw = new FileOutputStream(helpFile, true);
                    FileInputStream fr = new FileInputStream(helpFile);
            ) {
                if (helpFile.length() == 0) {
                    List<Long> indices = new ArrayList<>();
                    for (int i = 0; i < Math.ceil((double) P2P.length / P2P.piece_length); i++) {
                        fw.write(0);
                        indices.add((long) i);
                    }
                    /* Momentalne neni shuffle nutny, ale je to predpripravene na eventualny upgrade,
                     * ked budu medzi sebou komunikovat aj leecheri, aby bola siet omnoho dynamickejsia,
                     * nez aby sa vsetko stahovalo linearne. */
                    Collections.shuffle(indices);
                    P2P.need2down = new LinkedList<>(indices);
                } else {
                    List<Long> indices = new ArrayList<>();
                    for (int i = 0; i < helpFile.length(); i++) {
                        if (fr.read() == 0)
                            indices.add((long) i);
                    }
                    Collections.shuffle(indices);
                    P2P.need2down = new LinkedList<>(indices);
                }
            } catch (IOException e) {
                System.err.println("Error while reading helpfile.");
                System.exit(-1);
            }
        }
    }

    @SuppressWarnings("unchecked")
    int contactTracker() {
        try {
            trackerSocket = new Socket(P2P.hostname, P2P.port);  //klient naviaze spojenie
            //Socket je mimo try-with-resources bloku, lebo na zaklade
            //"otvorenosti" socketu Tracker sleduje zivotnost clientov
            //v Guardthreade


            ObjectOutputStream oos = new ObjectOutputStream(trackerSocket.getOutputStream());
            oos.writeUnshared(P2P.pieces); //klient posle HashPieces suboru
            oos.reset();
            oos.flush();


            try {
                /* Neuzatvarame, pretoze uzavretim I/O streamu sa zavre aj socket, co ale nechceme;
                 * sposobilo by to zle fungovanie GuardThread. */
                ObjectInputStream ois = new ObjectInputStream(trackerSocket.getInputStream());
                P2P.IPs = (Set<InetAddress>) ois.readUnshared();
            } catch (ClassNotFoundException e) {
                System.out.println("Exception occured while receiving IPs from Tracker server");
                System.exit(-1);
            }
        } catch (IOException e) {
            System.out.println("Exception occured while trying to contact tracker server.");
            System.exit(-1);
        }
        return P2P.IPs.size();
    }

    void download() { //postupne hladame seedera s ktorym mozem komunikovat
        while (P2P.state == 'L') {
            if (P2P.IPs.size() == 0) {
                try {
                    Thread.sleep(10000);
                } catch (InterruptedException e) {
                    System.err.println("Some exception occured while waiting for some seeds to connect.");
                }
            } else {
                synchronized (P2P.IPs) {
                    for (InetAddress ip : P2P.IPs) {
                        download(ip, P2P.SERVER_SEED_PORT);
                    }
                    try {
                        Thread.sleep(10000);
                    } catch (InterruptedException e) {
                        System.err.println("Some exception occured while waiting for some seeds to connect.");
                    }
                }
            }
        }
    }

    void seederBroadcast() {
        if (P2P.state == 'S') { //ma sa to sice spustat vzdy iba pri seederovi, no istota je gulomet
            synchronized (P2P.IPs) {
                for (InetAddress ip : P2P.IPs) {
                    pingWithSeeder(ip, P2P.SERVER_SEED_PORT);
                }
            }
        }
    }

    private void download(InetAddress seedAddress, int seedPort) {
        try (
                Socket downloadSocket = new Socket(seedAddress, seedPort);
                ObjectInputStream ois = new ObjectInputStream(downloadSocket.getInputStream());
                ObjectOutputStream oos = new ObjectOutputStream(downloadSocket.getOutputStream());
        ) {
            ClientsideSLProtocol protocol = new ClientsideSLProtocol(helpFile);

            Object myAnswer = protocol.processInput(P2P.state);
            oos.writeUnshared(myAnswer);
            oos.reset();
            oos.flush();

            while (true) {
                Object serverQuery = ois.readUnshared();
                myAnswer = protocol.processInput(serverQuery);
                oos.writeUnshared(myAnswer);
                oos.reset();
                oos.flush();
            }
        } catch (ConnectException e) {
            System.out.println("Can't connect to " + seedAddress.getHostAddress() + ". Probably down." +
                    " Removing from IP set.");
            P2P.IPs.remove(seedAddress);
        } catch (SocketException e) {
            System.out.println("SocketException from disconnection of server.");
        } catch (IOException e) { //Prerusenie spojenia.
            System.out.println("Server disconnected.");
        } catch (ClassNotFoundException e) {
            System.err.println("Nasty exception: " + e.getClass().getSimpleName() + ", which shouldn't happen");
        }

        if (P2P.need2down.size() == 0) {
            P2P.state = 'S';
            helpFile.delete();
            P2P.seedFile = new File(P2P.pathToDownload + File.separator + P2P.name);
            seederBroadcast();
        }

    }

    private void pingWithSeeder(InetAddress address, int seedPort) {
        try (
                Socket socket = new Socket(address, seedPort);
                ObjectInputStream ois = new ObjectInputStream(socket.getInputStream());
                ObjectOutputStream oos = new ObjectOutputStream(socket.getOutputStream());
        ) {
            Object answer = 'S';
            oos.writeUnshared(answer);
            oos.reset();
            oos.flush();

            ois.readUnshared();

            answer = "BYE";
            oos.writeUnshared(answer);
            oos.reset();
            oos.flush();

            ois.readUnshared();
        } catch (SocketException e) {
            System.out.println("Server got pinged with me and so he disconnected.");
        } catch (EOFException e) {
            System.out.println("I pinged node, so he can add me.");
        } catch (IOException e) {
            System.err.println("IOException occured while trying to ping nodes in network with seeder." + e.toString());
            //System.out.println(e.getMessage());
            //e.getCause();
            //e.printStackTrace();

        } catch (ClassNotFoundException e) {
            System.err.println("Nasty exception: " + e.getClass().getSimpleName() + ", which shouldn't happen" +
                    " while pinging with seeder.");
        }
    }


    void printIPs() {
        System.out.println("My address is: " + trackerSocket.getLocalAddress().getHostAddress());
        if (P2P.IPs.isEmpty())
            System.out.println("Ziadna IP k danemu suboru.");
        else {
            synchronized (P2P.IPs) {
                for (InetAddress i : P2P.IPs) {
                    System.out.println(i.getHostAddress());
                }
            }
        }
    }

    void closeSocket() {
        try {
            trackerSocket.close();
        } catch (IOException e) {
            System.err.println("Exception occured while trying to close client-tracker socket.");
        }
    }

}
