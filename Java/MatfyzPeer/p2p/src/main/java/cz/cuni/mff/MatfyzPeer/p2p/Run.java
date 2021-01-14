package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.net.UnknownHostException;


public class Run {

    private static void pressKeyToExit() throws IOException {
        System.out.println("Press ENTER to exit...");
        new InputStreamReader(System.in).read();
        System.exit(0);
    }

    public static void main(String[] args) throws IOException {
        String answer;
        char state;
        boolean haveMeta;
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        P2P p2p = null;
        File seedFile;
        File metaFile;
        InetAddress trackerHostname;
        int trackerPort;
        File pathOfDownloaded;


        System.out.println("Do you want to seed or leech? (seed/leech)");

        while (!(answer = reader.readLine()).equals("leech") && !answer.equals("seed")) {
            System.out.println("Not correct answer. Are you seed or leech?");
        }

        state = answer.equals("leech") ? 'L' : 'S';

        System.out.println("Do you have metafile of file, which you want to " +
                (state == 'L' ? "download? (Y/N)" : "seed? (Y/N)"));

        while (!(answer = reader.readLine()).equals("Y") && !answer.equals("N")) {
            System.out.println("Write Y if you have metafile, or N if you dont have metafile.");
        }

        haveMeta = answer.equals("Y");

        if (!haveMeta && state == 'L') {
            System.out.println("You have some serious problem bro. Go get yourself some metainfo file.");
            pressKeyToExit();
        } else if (!haveMeta && state == 'S') {
            System.out.println("Path to file you want to seed:");
            seedFile = new File(reader.readLine()); //osetrit vynimku
            while (!seedFile.isFile()) {
                System.out.println("File you wrote doesn't exist. Please, check if you wrote correct " +
                        "path to the file or the name of file itself.\nWrite again path to file you want to seed:");
                seedFile = new File(reader.readLine());
            }
            System.out.println("Tracker hostname/IP:");

            while (true) {
                try {
                    trackerHostname = InetAddress.getByName(reader.readLine());
                    break;
                } catch (UnknownHostException e) {
                    System.out.println("You wrote incorrect form of hostname (textual or IP representation). Write again:");
                }
            }

            System.out.println("Tracker port number:");
            trackerPort = Integer.parseInt(reader.readLine());
            while (trackerPort < 0 || trackerPort > 65535) {
                System.out.println("Invalid port. Type valid port in range [0-65535]:");
                trackerPort = Integer.parseInt(reader.readLine());
            }
            System.out.println("Do you want to use default block size? (Y/N)");
            while (!(answer = reader.readLine()).equals("Y") && !answer.equals("N")) {
                System.out.println("Write Y if you want to use default, or N if you want own size.");
            }

            if (answer.equals("Y")) {
                try {
                    P2P.createMetainfo(trackerHostname.getHostAddress(), trackerPort, seedFile);
                } catch (IOException e) {
                    System.out.println("Exception occured while trying to create metafile");
                    System.exit(-1);
                }
            } else {
                System.out.println("Type size of one piece of block (in bytes)" +
                        " in which will be file leeched/seeded [0-2147483647]:");
                while (true) {
                    try {
                        int blockSize = Integer.parseInt(reader.readLine());
                        P2P.createMetainfo(trackerHostname.getHostAddress(), trackerPort, seedFile, blockSize);
                        break;
                    } catch (NumberFormatException e) {
                        System.out.println("You didn't write valid form of integer. Please retry:");
                    } catch (IOException e) {
                        System.out.println("Exception occured while trying to create metafile");
                        System.exit(-1);
                    }
                }
            }

            System.out.println("Metainfo file was created. Feel free to distribute it :). To actually seed, " +
                    "please re-run program and use metainfo created.");
            pressKeyToExit();
        } else if (haveMeta && state == 'S') {
            System.out.println("Path to file you want to seed:");
            seedFile = new File(reader.readLine());
            while (!seedFile.isFile()) {
                System.out.println("File you wrote doesn't exist. Please, check if you wrote correct " +
                        "path to the file or the name of the file itself.\nWrite again path to file you want to seed:");
                seedFile = new File(reader.readLine());
            }
            System.out.println("Path to your metainfo file:");
            metaFile = new File(reader.readLine());
            while (!metaFile.isFile()) {
                System.out.println("File you wrote doesn't exist. Please, check if you wrote correct " +
                        "path to the file or the name of file itself.\nWrite again path to metainfo file:");
                metaFile = new File(reader.readLine());
            }
            p2p = new P2P(state, metaFile, seedFile);

        } else { // state == 'L' && haveMeta
            System.out.println("Path to your metafile:");
            metaFile = new File(reader.readLine());
            while (!metaFile.isFile()) {
                System.out.println("File you wrote doesn't exist. Please, check if you wrote correct " +
                        "path to the file or the name of file itself.\nWrite again path to metainfo file:");
                metaFile = new File(reader.readLine());
            }
            System.out.println("Path, where do you want to download file:");
            pathOfDownloaded = new File(reader.readLine());
            while (!pathOfDownloaded.isDirectory()) {
                System.out.println("Path you wrote doesn't exist. Please, check if you wrote correct " +
                        "path for file to be downloaded and try again:");
                pathOfDownloaded = new File(reader.readLine());
            }
            p2p = new P2P(state, metaFile, pathOfDownloaded);
        }

        System.out.println("Do you want to start " + (state == 'S' ? "seeding? (Y/N)" : "downloading? (Y/N)"));
        while (!(answer = reader.readLine()).equals("Y") && !answer.equals("N")) {
            System.out.println("Write Y to start, or N to quit.");
        }

        if (answer.equals("N")) {
            System.out.println("Bye-bye.");
            pressKeyToExit();
        } else {
            p2p.startServer.start();
            int participation = p2p.clientSide.contactTracker();
            if (state == 'L' && participation == 0) {
                System.out.println("Nobody participates at seeding this file. Do you want to exit? (Y/N)");
                while (!(answer = reader.readLine()).equals("Y") && !answer.equals("N")) {
                    System.out.println("Write N to wait for someone to connect to network" +
                            " and seed file for you, or Y to exit.");
                }
                if (answer.equals("Y")) {
                    System.out.println("I'm sorry that nobody is there seeding file for you :(");
                    pressKeyToExit();
                }
            }
            if (state == 'L') {
                p2p.clientSide.download();
                System.out.println("Download completed. (Probably).");
            } else {
                p2p.clientSide.seederBroadcast();
            }
            System.out.println("You are seeding " + P2P.seedFile.getName() + " now! Write quit anytime" +
                    " to quit seeding.");
        }


        while (!(reader.readLine()).equals("quit")) ;

        p2p.clientSide.closeSocket();
        p2p.serverSide.closeServerSocket();
    }


}
