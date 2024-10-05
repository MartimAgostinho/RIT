/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * RouteEntry.java
 *
 * Auxiliary class to hold Routing table entries
 *
 * @author Luis Bernardo
 */
package router;


public class RouteEntry extends Entry {
// Fields inherited from Entry
//    public Address dest;
//    public int dist;
//    public AddressList path;

// New fields
    /** next_hop hop */
    public Address next_hop;

    /**
     * Constructor - create an empty instance to a destination
     */
    public RouteEntry() {
        super(Router.MAX_DISTANCE);
        next_hop= new Address();
    }

    /**
     * Constructor - create an entry defining all fields
     * @param dest      destination address
     * @param next_hop  next_hop hop address
     * @param dist      distance to next_hop hop
     * @param path      path to destination
     */
    public RouteEntry(Address dest, int dist, Address next_hop, AddressList path) {
        super(dest, dist, path);
        this.next_hop= next_hop;
    }

    /**
     * Constructor - clone an existing entry
     * @param src  object that will be cloned
     */
    public RouteEntry(RouteEntry src) {
        super((Entry)src);
        this.next_hop= src.next_hop;
    }

    /** Returns true if RouteEntry is equal to e. It does not compare the date or TTL.
     * @param e     other entry
     * @return  true if equal, false otherwise
     */
    public boolean equals(RouteEntry e) {
        return super.equals_to(e) && 
                ((e.next_hop==next_hop) || ((e.next_hop!=null) && e.next_hop.equals(next_hop)));
    }
    
}
