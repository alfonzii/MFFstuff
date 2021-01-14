package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.IOException;
import java.io.RandomAccessFile;


class ServersideSLProtocol {
    private static final int WAITING = 0;
    private static final int CONTACTED = 1;
    private static final int BLOCK_QUERY = 2;
    private static final int ABOUT_TO_SEND_BLOCK = 3;
    private static final int SENT_BLOCK = 4;
    private static final int BYED = 6;
    private static final int AWAITING_BYE = 7;

    private static final long MAXINDEX = (long) Math.floor((double) P2P.length / P2P.piece_length);

    private int state = WAITING;
    private long index;


    Object processInput(Object query) throws SeedConnection {
        Object answer = null;

        if (state == WAITING) {
            if (P2P.state == (Character) query) { // L-L alebo S-S; vtedy sa nic nedeje.
                state = AWAITING_BYE; //ocakavam "BYE".
                answer = P2P.state;
            } else if (P2P.state == 'L') {
                state = AWAITING_BYE;
                throw new SeedConnection("Seed connected to leech.");
            } else {
                state = CONTACTED;
                answer = P2P.state; //'S'
            }
        } else if (state == CONTACTED) {
            index = (Long) query;
            if (Server.usedThreads.get() <= Server.numOfThreads) {
                if (index <= MAXINDEX) {
                    state = BLOCK_QUERY;
                    answer = "ACK";
                } else {
                    answer = "BAD";
                }
            } else {
                state = AWAITING_BYE; //nemam volne thready, ocakavam "BYE"
                answer = "NAK";
            }
        } else if (state == BLOCK_QUERY) {
            if (query.equals("ACK")) {
                //poslem mu subor
                answer = getBlock(index); //answer je pole bytov
                state = ABOUT_TO_SEND_BLOCK;
            }
        } else if (state == ABOUT_TO_SEND_BLOCK) {
            if (query.equals("ACK")) {
                answer = "MORE";
                state = SENT_BLOCK;
            } else if (query.equals("NAK")) {
                answer = getBlock(index);
            } else if (query.equals("BYE")) {
                state = BYED;
            }
        } else if (state == SENT_BLOCK) {
            if (query.equals("BYE")) {
                state = BYED;
            } else if (query.equals("MORE")) {
                answer = "ACK";
                state = CONTACTED;
            }
        } else if (state == AWAITING_BYE) {
            if (query.equals("BYE")) {
                state = BYED;
            }
        }

        return answer;
    }

    boolean bye() {
        return state == BYED;
    }

    private Object getBlock(long index) { //pole bytov
        try (RandomAccessFile rf = new RandomAccessFile(P2P.seedFile, "r");) {
            byte[] buff;
            rf.seek(index * (long) P2P.piece_length);
            if (index == MAXINDEX)
                buff = new byte[(int) (P2P.length % (long) P2P.piece_length)];
            else
                buff = new byte[P2P.piece_length];
            rf.readFully(buff);
            return buff;
        } catch (IOException e) {
            System.err.println("IOException in getBlock() method. Definitely shouldn't happen.");
        }
        return new Object(); //musi tu byt, inac Java nepusti; normalne by som sem vsak nemal nikdy dojst.
    }


}
