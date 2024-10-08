/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * Routing.java
 *
 * Encapsulates the routing functions, hosting multiple instances of
 * Routing_process objects, and handles DATA packets
 *
 * @author Luis Bernardo
 * @author student 1 name and number
 * @author student 2 name and number
 */
package router;

import java.io.*;
import java.net.*;
import java.util.*;
import javax.swing.*;

/**
 * Encapsulates the Routing data and functions, and handles DATA packets
 */
public class Routing {

    // Variables
    /**
     * Routing table object
     */
    RoutingTable tab;

    /**
     * Local Address
     */
    private final Address local_address;
    /**
     * Neighbour list
     */
    private final NeighbourList neig;
    /**
     * Reference to main window with GUI
     */
    private final Router win;
    /**
     * Unicast datagram socket used to send packets
     */
    private final DatagramSocket ds;
    /**
     * Reference to graphical Routing table object
     */
    private final JTable tableObj;

    /**
     * timer object to support periodic vector exchange
     */
    private javax.swing.Timer timer_announce;

    // Configuration variables
    /**
     * ROUTE sending period [sec]
     */
    private final int period;
    /**
     * Uses hierarchical Routing
     */
    private final boolean hierarchical;
    
    
    /**
     * Create a new instance of a routing object, that encapsulates routing
     * processes
     *
     * @param local_address local Address
     * @param neig Neighbour list
     * @param hierarchical hierarchical routing
     * @param period ROUTE timer period
     * @param win reference to main window object
     * @param ds unicast datagram socket
     * @param TabObject Graphical object with the 
     */
    public Routing(Address local_address, NeighbourList neig, boolean hierarchical, 
            int period, Router win, DatagramSocket ds, JTable TabObject) {
        this.local_address= local_address;
        this.neig= neig;
        this.hierarchical= hierarchical;
        this.period= period;
        this.win= win;
        this.ds= ds;
        this.tableObj= TabObject;
        // Initialize everything
        this.timer_announce= null;
        this.tab= new RoutingTable();
        Log2("new routing(local='"+local_address+"', period="+period+
            (hierarchical?",hierarchical":"")+")");
    }

    /**
     * Starts Routing thread
     */
    public void start() {
        update_routing_table();
        start_announce_timer();
    }
  
    /**
     * Stop all the Routing processes and resets the Routing state
     */
    public void stop() {
        stop_announce_timer();
        // Clean Routing table
        tab.clear();
        update_routing_window();
    }
 
    /**
     * Prepare a ROUTE packet message with the contents of tab
     * @param tab Routing table
     * @return a datagram packet object with the ROUTE message
     */
    private DatagramPacket make_ROUTE_packet(RoutingTable tab) {
        ByteArrayOutputStream os= new ByteArrayOutputStream();
        DataOutputStream dos= new DataOutputStream(os);
        try {
            dos.writeByte(Router.PKT_ROUTE);
            local_address.writeAddress(dos);
            dos.writeInt(period+5);
            tab.write_table(dos);
            byte [] buffer = os.toByteArray();
            return new DatagramPacket(buffer, buffer.length);
        }
        catch (IOException e) {
            Log("Internal error in make_ROUTE_packet: "+e+"\n");                    
            return null;
        }        
    }
    
    /**
     * Send local ROUTE to all neighbours
     *
     * @return true if successful, false otherwise
     */
    public synchronized boolean send_local_ROUTE() {
        Log2("send_local_ROUTE\n");
        if ((tab==null) || !tab.is_valid()) {
	    Log("ROUTE packet not sent - routing table not initialized\n");
            return false;
        }

        if (!hierarchical) {
            // Prepare the ROUTE packet
            DatagramPacket inROUTEpkt= make_ROUTE_packet(tab);
            if (inROUTEpkt == null)
                return false;

            try {
                Log2("send_local_ROUTE 1\n");
                neig.send_packet(ds, inROUTEpkt, null);
            }
            catch (IOException e) {
                Log("Error sending ROUTE: "+e+"\n");                    
            }
            win.ROUTE_snt++;
            Log2("send_local_ROUTE ended\n");
            return true;
            
        } else {
            
            //Log("send_local_ROUTE: hierarchical networks not supported yet\n");
            
            DatagramPacket inROUTEpkt= make_ROUTE_packet(tab);
            if (inROUTEpkt == null)
                return false;
            
            Address net = new Address(local_address);
            net.to_networkAddress();

            RoutingTable aux = new RoutingTable();
            aux.add_route(new RouteEntry(net,0,null,new AddressList() ));
            
            DatagramPacket outside = make_ROUTE_packet(aux);
            DatagramPacket DPaux;

            for(Neighbour n : neig.values()){
                
                DPaux =  local_address.equal_network(n.Address()) ? inROUTEpkt : outside;
                Log(local_address.toString() +": "+ n.Address().toString()+"\n");
                try {
                    Log2("send_local_ROUTE 1\n");
                     n.send_packet(ds, DPaux);
                }
                catch (IOException e) {
                    Log("Error sending ROUTE: "+e+"\n");                    
                }
            }

            win.ROUTE_snt++;
            Log2("send_local_ROUTE ended\n");
            return true;
            
            // Put here the code to generate the ROUTE packets sent to
            //   routers outside the area
            // Remeber that different ROUTEs should be sent to internal 
            //   and external routers

            //return false;
        }
    }
    
    
    /**
     * Unmarshall a ROUTE packet and process it
     *
     * @param sender the sender Address
     * @param dp datagram packet
     * @param ip IP Address of the sender
     * @param dis input stream object
     * @return true if packet was handled successfully, false if error
     */
    public boolean process_ROUTE(Address sender, DatagramPacket dp, 
            String ip, DataInputStream dis) {
        //Log("Packet ROUTE not supported yet\n");
        try {
            Log("PKT_ROUTE");
            String aux;
            aux="("+sender.toString()+",";
            int TTL= dis.readInt();
            aux+= "TTL="+TTL+",";
            int n= dis.readInt();
            aux+= "List("+n+": ";
            if ((n<=0) || (n>30)) {
                Log("\nInvalid list length '"+n+"'\n");
                return false;
            }
            Entry [] data= new Entry [n];
            for (int i= 0; i<n; i++) {
                try {
                    data[i]= new Entry(dis);
                } catch(IOException e) {
                    Log("\nERROR - Invalid vector Entry: "+e.getMessage()+"\n");
                    return false;                    
                }
                aux+= (i==0 ? "" : " ; ") + data[i].toString();
            }
            Log(aux+")\n");
            // Test origin
            Neighbour orig= neig.locate_neig(ip, dp.getPort());
            if (orig == null) {
                Log("Received ROUTE from invalid neighbour\n");
                return false;
            }
            if (win.is_local_address(sender)) {
                Log("ROUTE packet loop back - must not happen\n");
                return false;
            }

            //Log("ROUTE messages decoded but not processed yet\n");
             
            orig.update_vec(data, TTL);

            // Put here the code to store the incoming ROUTE messages
            
            return true;
        } catch(IOException e) {
            Log("\nERROR - Packet too short\n");
            return false;
        } catch (Exception e) {
            Log("\nERROR - " + e + "\n");
            return false;
        }
    }

    /**
     * Calculate the Routing table
     *
     * @return true if the Routing table was modified, false otherwise
     */
    private synchronized void update_routing_table() {
        RoutingTable map= new RoutingTable();    // List with all known routers

        Log2("update_routing_table\n");
        // Adds the local address to the routing table
        map.add_route(
            new RouteEntry(local_address /* address */, 0 /* distance */, 
                new Address() /* empty router */, new AddressList()/* empty path*/));

        Log2("update_routing_table\n");
        
        RouteEntry reaux = null;

        AddressList addlAux;
        Address SourceAux;
        //Log("\n\nAAAA\n\n");

        //if(!hierarchical){
            for( Neighbour n : neig.values() ){

                //map.add_route( new RouteEntry(n.Address(), n.Dist(), n.Address(), new AddressList( n.Address() )) );
                //Log("\n\nNeig\n\n");

                if(n.Vec() == null){continue;}

                for( Entry e : n.Vec() ){
                    
                    reaux =  map.get_RouteEntry(e.dest);//This is okay because I will not receive dest outside of the network
                    //Log("\n\nDEBUG DEST");
                    //Log( e.dest.toString() );

                    //if( reaux != null && reaux.dest.equals(local_address) ){continue;}

                    if(
                        reaux == null || 
                        ( (Router.MAX_DISTANCE+1) > (e.dist + n.dist) && reaux.dist > (e.dist + n.dist) )   
                        ){ 

                        SourceAux = new Address(n.Address());
                        if( !local_address.equal_network(SourceAux) ){ 
                            SourceAux.to_networkAddress();
                        }
                        
                        addlAux = new AddressList(e.path);
                        addlAux.insert(SourceAux);
                        //Log2("\n\n\n\n\n!!!!!!!DEBUG!!!!\n\n\n\n\n");
                        
                        map.add_route( 
                            new RouteEntry(e.dest, e.dist + n.dist,n.Address() , addlAux ) 
                        );
                        
                    }
                }
            }
        //}else{}
        // Put here the code that implements the path vector algorithm
        // STEP 1:
        //      Implement the flat algorithm - go through all the neighbours testing each destination, 
        //          adding them the distance is shorter or the destination is new. Ignore distances
        //          above Router.MAX_DISTANCE.

        //TODO:
        // STEP 2:
        //      Implement the hierarchical algorithm; this second step is more
        //          complex - requires using network addresses in the destination and
        //          path fields of the RouteEntry elements outside the local network

        
        // Update the routing table using map contents
        tab= map;
        
        // Echo Routing table in the GUI
        update_routing_window();
        Log2("update_routing_table ended\n");
    }
    
    /**
     * Display the Routing table in the GUI
     */
    public void update_routing_window() {
        Log2("update_routing_window\n");
        // update window
        Iterator<RouteEntry> it= tab.iterator();
        RouteEntry rt;
        for (int i= 0; i<tableObj.getRowCount(); i++) {
            if (it.hasNext()) {
                rt= it.next();
                Log2("("+rt.dest+" : "+rt.next_hop+" : "+rt.dist+" : "+rt.path+")");
                tableObj.setValueAt(""+rt.dest,i,0);
                tableObj.setValueAt(""+rt.next_hop,i,1);
                tableObj.setValueAt(""+rt.dist,i,2);
                tableObj.setValueAt(""+rt.path,i,3);
            } else {
                tableObj.setValueAt("",i,0);
                tableObj.setValueAt("",i,1);
                tableObj.setValueAt("",i,2);
                tableObj.setValueAt("",i,3);
            }
        }
    }


    /* ------------------------------------ */
    // Announce timer

    /**
     * Launches timer responsible for sending periodic distance packets to
     * neighbours. Defines the timer function - responsible for sending periodic ROUTE packets to routers
     * within the area.
     * 
     * 
     */
    private void start_announce_timer() {

        java.awt.event.ActionListener act;

        act = ( java.awt.event.ActionEvent env ) -> {
            send_local_ROUTE();
            update_routing_table();
        };

        timer_announce = new javax.swing.Timer( period * 500, act);
        timer_announce.start();
                   
    }

    /**
     * Stops the timer responsible for sending periodic distance packets to
     * neighbours
     */
    private void stop_announce_timer() {
        if (timer_announce != null) {
            timer_announce.stop();
            timer_announce= null;
        }
    }
    
    
    /**
     * *************************************************************************
     * DATA HANDLING
     */
    
    /**
     * returns next_hop hop to reach destination
     *
     * @param dest destination Address
     * @return the Address of the next_hop hop, or null if not found.
     */
    public Address next_Hop(Address dest) {
        //Log("Method Routing.next_Hop not implemented yet");

        if( tab == null )
            return null;

        Address aux = dest;
        if( !local_address.equal_network(dest) ){
            aux = new Address(dest);
            aux.to_networkAddress();
        }
        return tab.nextHop(aux.toString());

        // Put here the code that consult the Routing table and returns the next
        //    hop, or null if no path is known to the destination

    }
    
    /**
     * send a DATA packet using the Routing table and the neighbor information
     *
     * @param dest destination Address
     * @param dp datagram packet object
     */
    public void send_data_packet(Address dest, DatagramPacket dp) {
        if (local_address.equals(dest)) {
            // Send to local node
            try {
                dp.setAddress(InetAddress.getLocalHost());
                dp.setPort(ds.getLocalPort());
                ds.send(dp);
                win.DATA_snt++;
            }
            catch (UnknownHostException e) {
                Log("Error sending packet to himself: "+e+"\n");
            }
            catch (IOException e) {
                Log("Error sending packet to himself: "+e+"\n");
            }
            
        } else { // Send to Neighbour Address
            Address prox= next_Hop(dest);
            if (prox == null) {
                Log("No route to destination: packet discarded\n");
            } else {
                // Lookup Neighbour
                Neighbour pt= neig.locate_neig(prox);
                if (pt == null) {
                    Log("Invalid neighbour ("+prox+
                        ") in routing table: packet discarder\n");
                    return;
                }
                try {
                    pt.send_packet(ds, dp);    
                    win.DATA_snt++;
                }
                catch(IOException e) {
                    Log("Error sending DATA packet: "+e+"\n");
                }
            }            
        }
    }
 
    /**
     * prepares a data packet; adds local_address_string to path
     *
     * @param sender sender addr
     * @param dest destination addr
     * @param msg message contents
     * @param path path already transverse
     * @return datagram packet to send
     */
    public DatagramPacket make_data_packet(Address sender, Address dest, String msg,
            AddressList path) {
        ByteArrayOutputStream os= new ByteArrayOutputStream();
        DataOutputStream dos= new DataOutputStream(os);
        try {
            dos.writeByte(Router.PKT_DATA);
            sender.writeAddress(dos);
            dest.writeAddress(dos);
            dos.writeShort(msg.length());
            dos.writeBytes(msg);
            path.append(local_address);// Appends the local Address to the path list
            path.writeAddressList(dos);
        }
        catch (IOException e) {
            Log("Error encoding data packet: "+e+"\n");
            return null;
        }
        byte [] buffer = os.toByteArray();
        return new DatagramPacket(buffer, buffer.length);
    }
    
    /**
     * prepares a data packet; adds local_address_string to path and send the packet
     *
     * @param sender sender addr
     * @param dest destination addr
     * @param msg message contents
     * @param path path already transverse
     */
    public synchronized void send_data_packet(Address sender, Address dest, String msg,
            AddressList path) {
        DatagramPacket dp= make_data_packet(sender, dest, msg, path);
        if (dp != null)
            send_data_packet(dest, dp);
    }

    /**
     * unmarshals DATA packet e process it
     *
     * @param sender the sender of the packet
     * @param dp datagram packet received
     * @param ip IP of the sender
     * @param dis data input stream
     * @return true if decoding was successful
     */
    public boolean process_DATA(Address sender, DatagramPacket dp, 
            String ip, DataInputStream dis) {
        try {
            Log("PKT_DATA");
            // Read Dest
            Address dest= new Address();
            dest.readAddress(dis);
            // Read message
            int len_msg= dis.readShort();
            if (len_msg>255) {
                Log(": message too long ("+len_msg+">255)\n");
                return false;
            }
            byte [] sbuf1= new byte [len_msg];
            int n= dis.read(sbuf1,0,len_msg);
            if (n != len_msg) {
                Log(": Invalid message length\n");
                return false;
            }
            String msg= new String(sbuf1,0,n);
            // Read path
            AddressList path= new AddressList();
            path.readAddressList(dis);
            if (path.size()>Router.MAX_PATH_LEN) {
                Log(": path length too long ("+path.size()+">"+Router.MAX_PATH_LEN+
                    ")\n");
                return false;
            }
            if (dis.available() > 0) {
                Log(": packet too long ("+dis.available()+" after path\n");
                return false;                
            }

            Log(" ("+sender.toString()+"-"+dest.toString()+"):'"+msg+"':Path='"+path.toString()+"'\n");
            // Test Routing table
            if (dest.equals(local_address)) {
                // Arrived at destination
                Log("DATA packet reached destination\n");
                return true;
            } else {
                Address prox= next_Hop(dest);
                if (prox == null) {
                    Log("No route to destination: packet discarded\n");
                    return false;
                } else {
                    // Send packet to next_hop hop
                    send_data_packet(sender, dest, msg, path);
                    return true;
                }
            }
        }
        catch (IOException e) {
            Log(" Error decoding data packet: " + e + "\n");
        }
        return false;       
    }

    
    /**
     * *************************************************************************
     * Log functions
     */
    /**
     * Output the string to the log text window and command line
     *
     * @param s log string
     */
    private void Log(String s) {
        win.Log(s);
    }

    /**
     * Auxiliary log function - when more detail is required remove the comments
     *
     * @param s log string
     */
    public final void Log2(String s) {
        //System.err.println(s);
        //if (win != null)
        //    win.Log(s);  // For detailed debug purposes
    }
}
