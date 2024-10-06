/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * AddressList.java
 *
 * Auxiliary class to hold ROUTE vector information
 *
 * @author Luis Bernardo
 */
package router;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;


/**
 * Hold a sequence of addresses 
 */
public class AddressList {
    /**
     * List of addresses
     */
    private final ArrayList<Address> list;
    
    /**
     * Constructor - Create a new empty list of Addresses
     */
    public AddressList() {
        list= new ArrayList<>();
    }
    
    /**
     * Constructor - Clone an existing address list
     * 
     * @param src address list
     */
    public AddressList(AddressList src) {
        list= new ArrayList<>();
        if (src != null)
            list.addAll(src.list);
    }
    
    /**
     * Constructor - Create a list with onee address
     * 
     * @param adr initial address 
     */
    public AddressList(Address adr) {
        list= new ArrayList<>();
        if (adr != null)
            list.add(adr);
    }
    
    /**
     * Returns a string with the address list contents
     * 
     * @return string with the list contents
     */
    @Override
    public String toString() {
        String str="[";
        for (Address a: list) {
            if (str.length()==1)
                str+= a.toString();
            else
                str+= (","+a.toString());
        }
        return str+']';
    }
    
    /**
     * Compares the list with list o
     * 
     * @param o other list
     * @return true of the list are equal, false otherwise
     */
    public boolean equals(AddressList o) {
        if ((o==null) || (o.size()!=size()))
            return false;
        for (int i=0; i<size(); i++)
            if (!get(i).equals(o.get(i)))
                return false;
        return true;
    }
    
    /**
     * Add an address to the begin of the list
     * 
     * @param addr address added
     */
    public void insert(Address addr) {
        if (addr==null)
            return;
        list.add(0, addr);
    }
    
    /**
     * Add an address to the end of the list
     * 
     * @param addr address appended
     */
    public void append(Address addr) {
        if (addr==null)
            return;
        list.add(addr);
    }
    
    /**
     * Return the address in position i of the list
     * 
     * @param i position in the list
     * @return address in position i
     */
    public Address get(int i) {
        return list.get(i);
    }
    
    /**
     * Remove the address in position i
     * 
     * @param i position in the list
     * @return the address removed
     */
    public Address remove(int i) {
        return list.remove(i);
    }
    
    /**
     * Clear all elements of the list
     */
    public void clear() {
        list.clear();
    }
    
    /**
     * Return the size of the list
     * 
     * @return the number of elements in the list
     */
    public int size() {
        return list.size();
    }
    
    /**
     * Return the first appearance of an address in the list
     * 
     * @param addr  an address
     * @return the position, or -1 if it was not found
     */
    public int indexOf(Address addr) {
        if (addr == null)
            return -1;
        for (Address a : list) {
            if (addr.equals(a)) {
                return list.indexOf(a);
            }
        }
        return -1;
    }
    
    /**
     * Replace by network addresses all the element of the list (except for 
     *    addresses of local_network), and remove duplicate addresses.
     * THIS FUNCTION DOES NOT MODIFY THE LIST CONTENTS - IT RETURNS A NEW LIST
     * 
     * @param local_network - if null, replaces addresses from all networks
     * @return a new list with the addresses replaced
     */
    public AddressList flat_network(Address local_network) {
        if (list == null)
            return null;
        Address last_net= null;
        AddressList outlst= new AddressList();
        for (Address a : list) {
            if ((local_network != null) && a.equal_network(local_network)) {
                outlst.append(a);
            } else {
                if ((last_net == null) || !last_net.equal_network(a)) {
                    last_net= a;
                    outlst.append(a.networkAddress());
                }
            } 
        }
        return outlst;
    }
    
    /**
     * Write the content of an address list to an output buffer to prepare messages
     * 
     * @param dos   data output stream
     * @return true if the list was successfully written, false if an error occurred
     */
    public boolean writeAddressList(DataOutputStream dos) {
        try {
            dos.writeInt(size());
            for (Address a : list) {
                a.writeAddress(dos);
            }
            return true;
        }
        catch (Exception e) {
            System.out.println("Internal error writing AddressList in buffer: "+e+"\n");
            return false;
        }                
    }
    
    /** 
     * Read an address list from a DataInputStream; generates an exception in case of error
     * 
     * @param dis data input stream
     * @throws java.io.IOException 
     */
    public final void readAddressList(DataInputStream dis) throws java.io.IOException {
        int len= dis.readInt();
        if (len<0)
            throw new IOException("Invalid path length '"+len+"'");
        if (len>0) {
            for (int i=0; i<len; i++) {
                Address addr= new Address();
                addr.readAddress(dis);
                list.add(addr); // Append to list
            }
        }
    }
    
}
