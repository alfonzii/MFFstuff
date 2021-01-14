package cz.cuni.mff.MatfyzPeer.p2p;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

class ClientsideSLProtocol {
    private static final int WAITING = 0;
    private static final int SENT_STATE = 1;
    private static final int SENT_BLOCK_QUERY = 2;
    private static final int AWAITING_BLOCK = 3;
    private static final int DOWNLOADED_BLOCK = 4;
    private static final int WANT_MORE = 5;
    private static final int SENT_BYE = 6; //vycistime po sebe RandomAccessFile

    private int state = WAITING;
    private RandomAccessFile rf, rfhelp;
    private int TTL = 64;
    private long blocksNum = (long) Math.ceil((double) P2P.length / P2P.piece_length);

    ClientsideSLProtocol(File helpfile) {
        try {
            rf = new RandomAccessFile(P2P.pathToDownload + File.separator + P2P.name, "rw");
            rfhelp = new RandomAccessFile(helpfile, "rw");
        } catch (FileNotFoundException e) {
            System.err.println("File " + P2P.name + " or " + helpfile.getName() + " was not found.");
        }
    }

    Object processInput(Object query) {
        Object answer = null;

        if (state == WAITING) {
            answer = P2P.state;
            state = SENT_STATE;
        } else if (state == SENT_STATE) {
            if (query.equals(P2P.state) || P2P.state == 'S') {
                answer = "BYE";
                state = SENT_BYE;
            } else {
                answer = P2P.need2down.getFirst(); //indexy bloku na stiahnutie
                state = SENT_BLOCK_QUERY;
            }
        } else if (state == SENT_BLOCK_QUERY) {
            if (query.equals("ACK")) {
                answer = "ACK";
                state = AWAITING_BLOCK;
            } else if (query.equals("NAK")) {
                answer = "BYE";
                state = SENT_BYE;
            } else if (query.equals("BAD")) {
                answer = P2P.need2down.getFirst();
            }
        } else if (state == AWAITING_BLOCK) {
            if (checkBlockHash(P2P.need2down.getFirst(), (byte[]) query)) {
                try {
                    saveBlock(P2P.need2down.getFirst(), (byte[]) query);
                    updateHelp(P2P.need2down.getFirst());
                    P2P.need2down.removeFirst();
                    answer = "ACK";
                    System.out.print("\r" + Math.round(((blocksNum - P2P.need2down.size()) / (float) blocksNum) * 100) + "% downloaded");
                    state = DOWNLOADED_BLOCK;
                } catch (IOException e) {
                    answer = "NAK";
                    TTL--;
                } finally {
                    if (TTL == 0) {
                        answer = "BYE";
                        state = SENT_BYE;
                    }
                }
            } else {
                if (TTL == 0) {
                    answer = "BYE";
                    state = SENT_BYE;
                } else {
                    TTL--;
                    answer = "NAK";
                }
            }
        } else if (state == DOWNLOADED_BLOCK) {
            if (query.equals("MORE")) {
                if (P2P.need2down.size() == 0) {
                    answer = "BYE";
                    state = SENT_BYE;
                } else {
                    answer = "MORE";
                    state = WANT_MORE;
                }
            }
        } else if (state == WANT_MORE) {
            if (query.equals("ACK")) {
                answer = P2P.need2down.getFirst();
                state = SENT_BLOCK_QUERY;
            }
        }


        if (state == SENT_BYE) { //provedeme cleanup
            try {
                cleanup();
            } catch (IOException e) {
                System.err.println("Exception while cleaning RandomAccessFile.");
            }
        }


        return answer;
    }


    private void cleanup() throws IOException {
        rf.close();
        rfhelp.close();
    }

    private boolean checkBlockHash(long index, byte[] block) {
        boolean isSame;
        String hash = P2P.pieces.substring(128 * (int) index, 128 * (int) (index + 1));
        String blockHash = org.apache.commons.codec.digest.DigestUtils.sha512Hex(block);
        isSame = hash.equals(blockHash);

        return isSame;
    }

    private void saveBlock(long index, byte[] block) throws IOException {
        rf.seek(index * (long) P2P.piece_length);
        rf.write(block);
    }

    private void updateHelp(long index) throws IOException {
        rfhelp.seek(index);
        rfhelp.write(1);
    }


}
