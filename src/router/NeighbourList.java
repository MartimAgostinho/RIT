/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * NeighbourList.java
 *
 * Holds the neighbor list router internal data
 *
 * @author Luis Bernardo
 */
package router;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.net.*;
import java.io.*;
import javax.swing.*;

/**
 * Holds the neighbor list Address internal data
 */
public class NeighbourList {

    /**
     * Maximum number of Neighbour objects in the list
     */
    private final int max_range;
    /**
     * Reference to the main window of the GUI
     */
    private final Router win;
    /**
     * List of Neighbour objects
     */
    private final ConcurrentHashMap<String, Neighbour> list;

    /**
     * Constructor - create a new instance of neighbourList
     *
     * @param max_range maximum number of neigbours in the list
     * @param win main window
     */
    public NeighbourList(int max_range, Router win) {
        this.max_range = max_range;
        this.win = win;
        list = new ConcurrentHashMap<>();
    }

    /**
     * Return a collection with all the neighbours in the list
     *
     * @return collection for all neigbours in the list
     */
    public Collection<Neighbour> values() {
        return list.values();
    }

    /**
     * Create an Iterator for all neigbour objects in the list
     *
     * @return iterator for all neigbours in the list
     */
    public Iterator<Neighbour> iterator() {
        return list.values().iterator();
    }

    /**
     * Add a new Neighbour to the list
     *
     * @param addr Neighbour's addr
     * @param ip ip Address
     * @param port port number
     * @param distance distance
     * @param ds datagram socket
     * @return true if new Neighbour was created and added, false otherwise
     */
    public boolean add_neig(Address addr, String ip, int port, int distance, DatagramSocket ds) {
        Address local_addr = win.local_address();
        boolean novo;
        System.out.println("add_neig(" + addr.toString() + ")");
        if ((novo = !list.containsKey(addr.toString())) && (list.size() == max_range)) {
            System.out.println("List is full\n");
            return false;
        }
        Neighbour pt = locate_neig(ip, port);
        if (local_addr.equals(addr)) {
            System.out.println("Address equals local_addr");
            return false;
        }
        if ((pt != null) && (!pt.Address().equals(addr))) {
            System.out.println("Duplicated IP and port\n");
            return false;
        }
        if ((distance < 1) || (distance > Router.MAX_DISTANCE)) {
            System.out.println("Invalid distance (" + distance + ")");
            return false;
        }
        // Prepare Neighbour entry
        pt = new Neighbour(addr, ip, port, distance);
        if (!pt.is_valid()) {
            System.out.println("Invalid neighbour data\n");
            return false;
        }
        // Adds or replaces a member of the table
        list.put(addr.toString(), pt);
        if (novo) // If not known
        {
            pt.send_Hello(ds, win);
        }
        return true;
    }

    /**
     * Update the field values of a Neighbour with the ip+port
     *
     * @param addr Neighbour's Address
     * @param ip ip Address
     * @param port port number
     * @param distance distance
     * @return true if updated the fields, false otherwise
     */
    public synchronized boolean update_neig(Address addr, String ip, int port, int distance) {
        System.out.println("update_neig(" + addr.toString() + ")");
        Neighbour pt = locate_neig(ip, port);
        if (pt == null) {
            System.out.println("Inexistant Neighbour\n");
            return false;
        }
        if ((distance < 1) || (distance > Router.MAX_DISTANCE)) {
            System.out.println("Invalid distance (" + distance + ")");
            return false;
        }
        if (!addr.equal_network(pt.Address())) {
            System.out.println("Invalid address - address previously associated with IP/port");
            return false;
        }
        if (pt.Dist() == distance) {
            // Did not change distance
            return false;
        }
        // Prepare Neighbour entry
        pt.update_neigh(pt.Address(), ip, port, distance);
        return true;
    }

    /**
     * Delete a Neighbour from the list, selected by addr
     *
     * @param addr addr of Neighbour
     * @param send_msg if true, sends a BYE message
     * @param ds datagram socket
     * @return true if deleted successfully, false otherwise
     */
    public boolean del_neig(Address addr, boolean send_msg, DatagramSocket ds) {
        Neighbour neig;
        try {
            neig = list.get(addr.toString());
        } catch (Exception e) {
            return false;
        }
        if (neig == null) {
            win.Log("Neighbour " + addr.toString() + " not deleted\n");
            return false;
        }
        if (send_msg) {
            neig.send_Bye(ds, win);
        }
        // Adds or replaces a member of the table
        list.remove(addr.toString());
        return true;
    }

    /**
     * Delete a Neighbour from the list, selected by object
     *
     * @param neig Neighbour to be deleted
     * @param send_msg if true, sends a BYE message
     * @param ds datagram socket
     * @return true if deleted successfully, false otherwise
     */
    public synchronized boolean del_neig(Neighbour neig, boolean send_msg, DatagramSocket ds) {
        if (!list.containsValue(neig)) {
            return false;
        }
        if (send_msg) {
            neig.send_Bye(ds, win);
        }
        // Removes a member from the list
        list.remove(neig.Address().toString());
        return true;
    }

    /**
     * empty Neighbour list and send BYE to all members
     *
     * @param ds datagram socket
     */
    public void clear_BYE(DatagramSocket ds) {
        for (Neighbour pt : list.values()) {
            pt.send_Bye(ds, win);
        }
        clear();
    }

    /**
     * Empty list
     */
    public void clear() {
        list.clear();
    }

    /**
     * Locate a Neighbour by Address string in the list
     *
     * @param addr_str addr_str to look for
     * @return the Neighbour object, or null if not found
     */
    public Neighbour locate_neig(String addr_str) {
        return list.get(addr_str);
    }

    /**
     * Locate a Neighbour by Address in the list
     *
     * @param addr addr to look for
     * @return the Neighbour object, or null if not found
     */
    public Neighbour locate_neig(Address addr) {
        return list.get(addr.toString());
    }

    /**
     * Locate a Neighbour by ip+port in the list
     *
     * @param ip IP Address
     * @param port port number
     * @return the Neighbour object, or null if not found
     */
    public Neighbour locate_neig(String ip, int port) {
        System.out.println("locate_neig("+ip+","+port+") - list contents:");
        for (Neighbour pt : list.values()) {
            System.out.println("\t"+pt);
            if (((ip.compareTo(pt.Ip()) == 0)
                    || (ip.startsWith("127.") && pt.Ip().startsWith("127.")))
                    && (port == pt.Port())) {
                return pt;
            }
        }
        return null;
    }

    /**
     * Send a packet to all neighbours in the list except 'exc'
     *
     * @param ds datagram socket
     * @param dp datagram packet to be sent
     * @param exc Neighbour to exclude, or null
     * @throws IOException  Error sending packet
     */
    public void send_packet(DatagramSocket ds, DatagramPacket dp,
            Neighbour exc) throws IOException {
        for (Neighbour pt : list.values()) {
            if (pt != exc) {
                pt.send_packet(ds, dp);
            }
        }
    }

    /**
     * Print the Neighbour list in the table at the GUI
     *
     * @param table reference to the graphical table
     * @return true if successful, false otherwise
     */
    public boolean refresh_table(JTable table) {
        if (table.getColumnCount() < 4) // Invalid number of columns
        {
            return false;
        }
        if (table.getRowCount() < max_range) // Invalid number of rows
        {
            return false;
        }

        // Update table
        Iterator<Neighbour> it = values().iterator();
        for (int i = 0; i < max_range; i++) { // For every row
            if (it.hasNext()) {
                Neighbour pt = it.next();
                table.setValueAt("" + pt.Address().toString(), i, 0);
                table.setValueAt(pt.Ip(), i, 1);
                table.setValueAt("" + pt.Port(), i, 2);
                table.setValueAt("" + pt.Dist(), i, 3);
            } else {
                for (int j = 0; j < 4; j++) {
                    table.setValueAt("", i, j);
                }
            }
        }
        return true;
    }

    
    /* ********************************************************************* */
    /* Functions for link state support                                      */
    /* ********************************************************************* */
    /**
     * For link state protocols - checks is addr already exists in the array
     *
     * @param list array list with addresses
     * @param addr addr to be tested
     * @return true if it exists, false otherwise
     */
    static private boolean duplicate_entry(ArrayList<Entry> list, Address addr) {
        for (Entry pt : list) {
            if (pt.dest.equals(addr)) {
                return true;
            }
        }
        return false;
    }

    /**
     * For link state protocols - returns the vector with the neighbors that
     * belong to a given area
     *
     * @param add_local if true, the list includes the node Address
     * @return a vector with the Neighbour nodes
     */
    public Entry[] local_vec(boolean add_local) {
        ArrayList<Entry> aux = new ArrayList<>();

        if (add_local) {
            // Adds the local Address
            aux.add(new Entry(win.local_address(), 0, new AddressList()));
        }

        for (Neighbour pt : list.values()) {
            if (pt.is_valid()) {
                aux.add(new Entry(pt.Address(), pt.Dist(), new AddressList(pt.Address())));
            }
        }

        Entry[] vec= null;
        if (!aux.isEmpty()) {
            // Creates an array with all elements
            vec = new Entry[aux.size()];
            vec = aux.toArray(vec);
        }
        aux.clear();
        return vec;
    }

    /* ********************************************************************* */
    /* End of functions for link state support                               */
    /* ********************************************************************* */
}
