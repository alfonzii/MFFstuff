package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.*;
import java.net.InetAddress;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

class P2P {

    static final int SERVER_SEED_PORT = 34999; //staci zmenit premennu a netreba menit konstanty v celom kode

    static Set<InetAddress> IPs = ConcurrentHashMap.newKeySet();
    static LinkedList<Long> need2down;
    static char state;

    static String hostname;
    static int port;
    static String name;
    static int piece_length;
    static long length;
    static String pieces;

    static File seedFile = null;
    static File pathToDownload = null;

    Client clientSide;
    Server serverSide;
    Thread startServer;

    P2P(char state, File metaFile, File pathOrSeedfile) {
        P2P.state = state; //'L'
        if (state == 'L') {
            pathToDownload = pathOrSeedfile;
        } else {
            P2P.seedFile = pathOrSeedfile;
        }
        try {
            parseMetainfo(metaFile);
            clientSide = new Client();
            serverSide = new Server();
            startServer = new Thread(serverSide);
        } catch (Exception e) {
            System.err.println("Exception occured while trying to parse file " + metaFile.getName());
            if (state == 'S') {
                P2P.seedFile = null;
            }
            System.exit(-1);
        }
    }

    static void createMetainfo(String hostname, int port, File file, int pieceLength) throws IOException {
        try (
                FileInputStream fileReader = new FileInputStream(file);
                FileWriter metainfoWriter = new FileWriter("metainfo-" + file.getName() + ".mff");
        ) {
            StringBuilder hashBulider = new StringBuilder();
            byte[] buffer = new byte[pieceLength];
            int loaded;
            while ((loaded = fileReader.read(buffer)) != -1) {
                if (loaded == pieceLength)
                    hashBulider.append(org.apache.commons.codec.digest.DigestUtils.sha512Hex(buffer));
                else
                    hashBulider.append(org.apache.commons.codec.digest.DigestUtils.sha512Hex(
                            Arrays.copyOf(buffer, loaded)));
            }

            //vytvorime metainfo subor
            metainfoWriter.write("tracker: <" + hostname + ":" + port + ">\n");
            metainfoWriter.write("name: " + file.getName() + '\n');
            metainfoWriter.write("piece_length: " + pieceLength + '\n');
            metainfoWriter.write("length: " + file.length() + '\n');
            metainfoWriter.write("pieces: " + hashBulider.toString() + '\n');
        }
    }

    static void createMetainfo(String hostname, int port, File file) throws IOException { //4194304 su 4MB
        createMetainfo(hostname, port, file, 4194304);
    }

    private void parseMetainfo(File metainfoFile) throws IOException {
        try (FileReader fileReader = new FileReader(metainfoFile)) {
            int ch;
            StringBuilder sb = new StringBuilder();
            while (fileReader.read() != '<') ; //zbavime sa info policka
            while ((ch = fileReader.read()) != ':')
                sb.append(Character.toChars(ch));
            hostname = sb.toString();
            sb = new StringBuilder();
            while ((ch = fileReader.read()) != '>')
                sb.append(Character.toChars(ch));
            port = Integer.decode(sb.toString());
            sb = new StringBuilder();
            fileReader.read(); //zbavime sa newline. Tracker vybaveny.

            while (fileReader.read() != ' ') ; //ulozime name
            while ((ch = fileReader.read()) != '\n')
                sb.append(Character.toChars(ch));
            name = sb.toString();
            sb = new StringBuilder();

            while (fileReader.read() != ' ') ; //piece_length
            while ((ch = fileReader.read()) != '\n')
                sb.append(Character.toChars(ch));
            piece_length = Integer.decode(sb.toString());
            sb = new StringBuilder();

            while (fileReader.read() != ' ') ; //length
            while ((ch = fileReader.read()) != '\n')
                sb.append(Character.toChars(ch));
            length = Long.decode(sb.toString());
            sb = new StringBuilder();

            while (fileReader.read() != ' ') ; //pieces Hashes
            while ((ch = fileReader.read()) != '\n')
                sb.append(Character.toChars(ch));
            pieces = sb.toString();
        }
    }


}
