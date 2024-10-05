/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * Neighbour.java
 *
 * Holds neighbor router internal data
 *
 * @author Luis Bernardo
 */
package router;

import java.net.*;
import java.io.*;
import java.util.*;


/**
 * Holds neighbor Address internal data
 */
public final class Neighbour {
    /** neigbour's Address */       
    public Address addr;
    /** IP Address of the Neighbour socket */
    public String ip;
    /** port number of the Neighbour socket */
    public int port;
    /** distance to the Neighbour */
    public int dist;
    /** Address of the Neighbour, includes IP+port */
    public InetAddress netip;
    
// Distance-vector protocols' specific data
    /** Vector received from Neighbour Address */    
    public Entry[] vec;
    /** Date when the vector was received */
    public Date vec_date;
    /** Vector TTL */
    public long vec_TTL;    // in seconds
    
    /**
     * Return the addr of the Neighbour
     * @return the character with the addr
     */
    public Address Address() { return addr; }
    /**
     * Return the IP Address of the Neighbour
     * @return IP Address
     */
    public String Ip() { return ip; }
    /**
     * Return the port number of the Neighbour
     * @return port number
     */
    public int Port()  { return port; }
    /**
     * Return the distance to the Neighbour
     * @return distance
     */
    public int Dist()  { return dist; }
    /**
     * Return the InetAddress object to send messages to the Neighbour
     * @return InetAddress object
     */    
    public InetAddress Netip() { return netip; }
    /** Vector-distance protocol specific function:
     *          Returns a vector, if it exists
     * @return  the vector, or null if it does not exists */
    public Entry[] Vec() { return vec_valid()? vec : null; }

        
    /**
     * Constructor - create an empty instance of neighbour
     */
    public Neighbour() {
        clear();
    }
    
    /**
     * Constructor - create a new instance of neighbour from parameters
     * @param addr      neighbour's addr
     * @param ip        ip Address
     * @param port      port number
     * @param distance  distance
     */
    public Neighbour(Address addr, String ip, int port, int distance) {
        clear();
        this.ip= ip;
        if (test_IP()) {
            this.addr= new Address(addr);
            this.port= port;
            this.dist= distance;
        } else
            this.ip= null;
    }
    
    /**
     * Constructor - create a clone of an existing object
     * @param src  object to be cloned
     */
    public Neighbour(Neighbour src) {
        this.addr= new Address(src.addr);
        this.ip= src.ip;
        this.netip= src.netip;
        this.port= src.port;
        this.dist= src.dist;
    }
        
    /**
     * Update the fields of the Neighbour object
     * @param addr      Neighbour's addr
     * @param ip        ip Address
     * @param port      port number
     * @param distance  distance
     */
    public void update_neigh(Address addr, String ip, int port, int distance) {
        this.ip= ip;
        if (test_IP()) {
            this.addr= new Address(addr);
            this.port= port;
            this.dist= distance;
        } else
            clear();
    }
    
    /**
     * Vector-distance specific function:
     *  updates last vector received from neighbor
     * @param vec  vector
     * @param TTL  Time to Live [msec]
     * @throws java.lang.Exception Invalid Neighbour
     */
    public void update_vec(Entry[] vec, long TTL) throws Exception {
        if (!is_valid())
            throw new Exception ("Update vector of invalid neighbor");
        this.vec= vec;
        this.vec_date= new Date();  // Now
        this.vec_TTL= TTL;
    }
    
    /**
     * Clear the contents of the neigbour object
     */
    public void clear() {
        this.addr= new Address();
        this.ip= null;
        this.netip= null;
        this.port= 0;
        this.dist= Router.MAX_DISTANCE;
        this.vec= null;
        this.vec_date= null;
        this.vec_TTL= 0;
    }

    /**
     * Test the IP Address
     * @return true if is valid, false otherwise
     */
    private boolean test_IP() {
        try {
            netip= InetAddress.getByName(ip);
            return true;
        }
        catch (UnknownHostException e) {
            netip= null;
            return false;
        }
    }

    /**
     * Test if the Neighbour is valid
     * @return true if is valid, false otherwise
     */
    public boolean is_valid() { return (netip!=null) && addr.is_valid(); }
    
    /**
     * Vector-distance protocol specific: test if the vector is valid
     * @return true if is valid, false otherwise
     */
    public boolean vec_valid() { return (vec!=null) && 
            ((new Date().getTime() - vec_date.getTime())<=vec_TTL*1000); }
        
    /**
     * Send a packet to the Neighbour
     * @param ds  datagram socket
     * @param dp  datagram packet with the packet contents
     * @throws IOException Error sending packet
     */
    public void send_packet(DatagramSocket ds, 
                                DatagramPacket dp) throws IOException {
        try {
            dp.setAddress(this.netip);
            dp.setPort(this.port);
            ds.send(dp);
        }
        catch (IOException e) {
            throw e;
        }        
    }
    
    /**
     * Send a packet to the Neighbour
     * @param ds  datagram socket
     * @param os  output stream with the packet contents
     * @throws IOException Error sending packet
     */
    public void send_packet(DatagramSocket ds, 
                                ByteArrayOutputStream os) throws IOException {
        try {
            byte [] buffer = os.toByteArray();
            DatagramPacket dp= new DatagramPacket(buffer, buffer.length, 
                this.netip, this.port);
            ds.send(dp);
        }
        catch (IOException e) {
            throw e;
        }        
    }
    
    /**
     * Create and send a HELLO message to the Neighbour
     * @param ds    datagram socket
     * @param win   main window object 
     * @return true if sent successfully, false otherwise
     */
    public boolean send_Hello(DatagramSocket ds, Router win) {
        // Send HELLO packet
        ByteArrayOutputStream os= new ByteArrayOutputStream();
        DataOutputStream dos= new DataOutputStream(os);
        try {
            dos.writeByte(Router.PKT_HELLO);
            // addr ('network.machine')
            win.local_address().writeAddress(dos);
            // Distance
            dos.writeInt(dist);
            send_packet(ds, os);
            win.HELLO_snt++;
            return true;
        }
        catch (IOException e) {
            System.out.println("Internal error sending packet HELLO: "+e+"\n");
            return false;
        }        
    }
    
    /**
     * Create and send a BYE message to the Neighbour
     * @param ds    datagram socket
     * @param win   main window object 
     * @return true if sent successfully, false otherwise
     */
    public boolean send_Bye(DatagramSocket ds, Router win) {
        ByteArrayOutputStream os= new ByteArrayOutputStream();
        DataOutputStream dos= new DataOutputStream(os);
        try {
            dos.writeByte(Router.PKT_BYE);
            // addr ('network.machine')
            win.local_address().writeAddress(dos);
            send_packet(ds, os);
            win.BYE_snt++;
            return true;
        }
        catch (IOException e) {
            System.out.println("Internal error sending packet BYE: "+e+"\n");
            return false;
        }        
    }
    
    /**
     * return a string with the Neighbour contents; replaces default function
     * @return string with the Neighbour contents
     */
    @Override
    public String toString() {
        String str= addr.toString();
        if (!addr.is_valid())
            str= "INVALID";
        return "("+str+" ; "+ip+" ; "+port+" ; "+dist+")";
    }
    
    /**
     * parses a string for the Neighbour field values
     * @param str  string with the values
     * @return true if parsing successful, false otherwise
     */
    public boolean parseString(String str) {
        StringTokenizer st = new StringTokenizer(str, " ();");
        if (st.countTokens( ) != 4)
            return false;
        try {
            // Parse addr
            Address _addr= new Address();
            if (!_addr.parseAddress(st.nextToken()))
                return false;
            String _ip= st.nextToken();
            int _port= Integer.parseInt(st.nextToken());
            int _dist= Integer.parseInt(st.nextToken());
            update_neigh(addr, _ip, _port, _dist);
            return is_valid();
        }
        catch (NumberFormatException e) {
            return false;
        }
    }
}
