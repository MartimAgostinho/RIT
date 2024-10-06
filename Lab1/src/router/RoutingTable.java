/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * RoutingTable.java
 *
 * Objects that store routing table information and provide supporting functions
 *
 * @author Luis Bernardo
 */
package router;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Collection;
import java.util.Iterator;
import java.util.concurrent.ConcurrentHashMap;

public final class RoutingTable {
    /** Routing table object */
    private final ConcurrentHashMap<String,RouteEntry> rtab;
    
    /**
     * Constructor
     */
    public RoutingTable() {
        rtab= new ConcurrentHashMap<>();
    }

    /**
     * Constructor that clones table received
     * @param src  Initial table 
     */
    public RoutingTable(RoutingTable src) {
        rtab= new ConcurrentHashMap<>();
        rtab.putAll(src.rtab);
    }
    
    /**
     * Check if the routing table is defined 
     * @return true if it is defined
     */
    public boolean is_valid() {
        return (rtab!=null);
    }
    
    /**
     * Clear the routing table contents
     */
    public void clear() {
        if (rtab!=null)
            rtab.clear();
    }
    
    /**
     * Add route entry to routing table
     * @param re RouteEntry1 object
     */
    public void add_route(RouteEntry re) {
        rtab.put(re.dest.toString(), re);
    }
    
    /**
     * Returns the RouteEntry associated to a destination
     * @param dest destination
     * @return RouteEntry object
     */
    public RouteEntry get_RouteEntry(String dest) {
        if (!is_valid())
            return null;
        return rtab.get(dest);
    }
    
    /**
     * Returns the RouteEntry associated to a destination
     * @param dest destination
     * @return RouteEntry object
     */
    public RouteEntry get_RouteEntry(Address dest) {
        if (!is_valid())
            return null;
        return rtab.get(dest.toString());
    }
    
    /**
     * Return the route's set
     * @return set of all RouteEntry1 
     */
    public Collection<RouteEntry> get_routeset() {
        if (!is_valid())
            return null;
        return rtab.values();
    }
    
    /**
     * Return the routing table as an array of Entry
     * @return Entry vector with table contents 
     */
    public Entry[] get_Entry_vector() {
        if (!is_valid())
            return null;
        Entry[] vec= new Entry[rtab.size()];
        rtab.values().toArray(vec);
        return vec;
    }   
    
    /**
     * Returns the next hop address in the path to dest
     * @param dest destination
     * @return the next hop address
     */
    public Address nextHop(String dest) {
        RouteEntry re= get_RouteEntry(dest);
        if (!is_valid())
            return new Address();
        return re.next_hop;
    }
    
    /**
     * Builds an iterator to the RouteEntry1 values
     * @return 
     */
    public Iterator<RouteEntry> iterator() {
        if (!is_valid())
            return null;
        return rtab.values().iterator();
    }
    
    /**
     * Compare the local routing tables with rt
     * @param rt - routing table
     * @return true if rt is equal to rtab and not null, false otherwise
     */
    public boolean equal_RoutingTable(RoutingTable rt) {
        if ((rt == null) || !rt.is_valid() || is_valid() )
            return false;
        ConcurrentHashMap<String, RouteEntry> map= rt.rtab;
        if (rtab.size() != map.size()) {
            return false;
        }
        for (RouteEntry re : rt.rtab.values()) {
            if (!re.equals_to(rtab.get(re.dest.toString()))) {
                return false;
            }                
        }      
        return true;
    } 
    
    /**
     * Log the content of a routing table object
     * @param win Main window
     */
    public void Log_routing_table(Router win) {
        if (rtab==null) {
            return;
        }
        for (RouteEntry re: rtab.values()) {
            win.Log(re.toString()+"\n");
        }
    }
    
    /**
     * Write the routing table contents to a DataOutputStream
     * @param dos           the output buffer
     * @throws IOException  
     */
    public void write_table(DataOutputStream dos) throws IOException {
        dos.writeInt(rtab.size());
        for (RouteEntry re : rtab.values()) {
            re.writeEntry(dos);
        }
        
    }
    
}
