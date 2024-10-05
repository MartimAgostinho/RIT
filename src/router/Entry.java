/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * Entry.java
 *
 * Auxiliary class to hold ROUTE vector information
 *
 * @author Luis Bernardo
 */
package router;

import java.io.*;

/** 
 * Hold ROUTE vector elements 
 */
public class Entry {
    
    /** Destination */
    public Address dest;
    /** Distance */
    public int dist;
    /** path vector*/
    public AddressList path;
    
    /** 
     * Constructor - Create a new instance of Entry to a void destination
     * @param dist  distance
     */
    public Entry(int dist){
        this.dest= new Address();
        this.dist= dist;
        this.path= new AddressList();
    }
    
    /**
     * Constructor - Create a new instance of Entry
     * @param dest  destination address
     * @param dist  distance
     * @param path  path to destination
     */
    public Entry(Address dest, int dist, AddressList path){
        this.dest= new Address(dest);
        this.dist= dist;
        this.path= path;
    }
    
    /**
     * Constructor - Create a new instance of Entry dupping another object
     * @param src object that will be copied
     */
    public Entry(Entry src){
        this.dest= new Address(src.dest);
        this.dist= src.dist;
        this.path= new AddressList(src.path);
    }
    
    /**
     * Constructor - Create a new instance of Entry from an input stream
     * @param dis  input stream
     * @throws java.io.IOException  Read error
     */
    public Entry(DataInputStream dis) throws java.io.IOException {
        this.dest= new Address();
        this.path= new AddressList();
        readEntry(dis);
    }
    
    /**
     * Return a string with the entry contents
     * @return string with the entry contents
     */
    @Override
    public String toString() {
        return "("+dest.toString()+","+dist+","+path.toString()+")";
    }
 
    /**
     * Update the Entry fields
     * @param dest  new destination
     * @param dist  new distance
     * @param path  new path
     */
    public void update(Address dest, int dist, AddressList path) {
        this.dest= dest;
        this.dist= dist;
        this.path= path;
    }

    /**
     * Update the path field
     * @param path  new path
     */
    public void update_path(AddressList path) {
        this.path= path;
    }
     
    /**
     * Compare with another Entry object
     * @param e  an Entry object
     * @return true if entry is equal to e
     */
    public boolean equals_to(Entry e) {
        return (e.dest.equals(dest)) && (e.dist == dist) && 
            (e.path==null ? path==null : e.path.equals(path));
    }
    
    /**
     * Compares to the destination field of another object
     * @param e  an Entry object
     * @return true if the destination is equal
     */
    public boolean equals_dest(Entry e) {
        return (e.dest.equals(dest));
    }
    
    /**
     * Static function that compares the entry vectors v1 and v2
     *
     * @param v1 - vector 1
     * @param v2 - vector 2
     * @return true if they are equal, false otherwise
     */
    public static boolean equal_EntryVec(Entry [] v1, Entry [] v2) {
        if (v1 == v2)    
            return true;
        if ((v1 == null) || (v2 == null) || (v1.length != v2.length))
            return false;
        boolean [] map= new boolean [v1.length];
        for (int i= 0; i<v1.length; i++)
            map[i]= false;
        for (int i= 0; i<v1.length; i++) {
            Entry c= v1[i];
            for (Entry v21 : v2) {
                if (!map[i] && v21.equals_to(c)) {
//                if (!map[i] && (v21.dest.equals(c.dest)) && (c.dist == v21.dist)) {
//                    // Test path
//                    if ((c.path == v21.path) || (c.path != null) && (v21.path != null) && c.path.equals(v21.path)) {
                        map[i]= true;
                        break;
//                    }
                }
            }
        }
        for (int i= 0; i<map.length; i++) {
            if (!map[i])
                return false;
        }
        return true;
    }
    
    /**
     * Write the Entry content to a DataOutputStream
     * @param dos  output stream
     * @throws java.io.IOException Write error
     */
    public void writeEntry(DataOutputStream dos) throws java.io.IOException {
        dest.writeAddress(dos);
        dos.writeInt(dist);
        path.writeAddressList(dos);
    }
    
    /**
     * Read the Entry contents from one DataInputStream
     * @param dis  input stream
     * @throws java.io.IOException Read error
     */
    public final void readEntry(DataInputStream dis) throws java.io.IOException {
        dest.readAddress(dis);
        dist= dis.readInt();
        if ((dist<0) || (dist>Router.MAX_DISTANCE))
            throw new IOException("Invalid distance '"+dist+"'");
        path.readAddressList(dis);
    }
    
}
