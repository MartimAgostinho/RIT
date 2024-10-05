/**
 * Redes Integradas de Telecomunicacoes MEEC/MIEEC/MERSIM 2024/2025
 *
 * Address.java
 *
 * Holds  address information
 *
 * @author Luis Bernardo
 */
package router;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;


/**
 * Hold addresses composed by area.machine 
 */
public final class Address {
    /**
     * Network (area) id - \in {A, ..., Z}
     */
    private char _network;
    /**
     * Machine id - \in {0, ..., 9}, where
     * 0 denotes the network and 1-9 identifies a machine
     */
    private byte _machine;
    
    /**
     * Constructor - Create a new empty instance of Address
     */
    public Address() {
        reset();
    }
    
    /**
     * Constructor - Create a new instance of Address
     * 
     * @param network
     * @param machine 
     */
    public Address(char network, int machine) {
        if (machine > 9 || machine < 0) {
            this._machine= -1;
            System.err.println("Invalid machine number ("+machine+") should be in [0,9]");
            return ;            
        }
        if (!validNetwork(network)) {            
            return ;
        }
        this._network= network;
        this._machine= (byte)machine;
    }

    /**
     * Constructor - clone an existing Address
     * @param str - string with address
     */
    public Address(String str) {
        parseAddress(str);
    }
    
    /**
     * Constructor - clone an existing Address
     * @param src - address object
     */
    public Address(Address src) {
        _network= src._network;
        _machine= src._machine;
    }
    
    
    /** 
     * returns a string with the address contents
     * 
     * @return address in string format */
    @Override
    public String toString() {
        if ((_network == ' ') || (_machine == -1))
            return "0.0";
        return ""+_network+'.'+_machine;
    }
    
    /** 
     * Return a string with the network (area) address
     * 
     * @return  network address
     */
    public char network() {
        return _network;
    }

    /**
     * Return the machine number
     * 
     * @return machine number 
     */
    public byte machine() {
        return _machine;
    }

    /**
     * Return the corresponding network address (with machine ==0)
     * 
     * @return network address
     */
    public Address networkAddress() {
        return new Address(_network, 0);
    }
    
    /**
     * Test if it is a network address, i.e. if machine == 0
     * 
     * @return true if it is network, false otherwise
     */
    public boolean is_network() {
        return is_valid() && (_machine==0);
    }
    
    /**
     * Converts the address to a netowkr address, setting machine=0
     */
    public void to_networkAddress() {
        _machine= 0;
    }
    
    /**
     * Compares the address with other
     * 
     * @param other address object
     * @return true if the addresses are equal, false otherwise
     */
    public boolean equals(Address other) {
        return (_network==other._network) && (_machine==other._machine);
    }
    
    /**
     * Compares if the network part of the address with other
     * @param other address object
     * @return true if the network is equal, false otherwise
     */
    public boolean equal_network(Address other) {
        return (_network==other._network);
    }
    
    /**
     * Test if the network part of the address is valid in a string
     * 
     * @param str   address string
     * @return true if it is valid, false otherwise
     */
    public static boolean validNetwork(char str) {
        return Character.isUpperCase(str);
    }
    
    /**
     * Test if the machine part of the address is valid
     * 
     * @param m   machine number
     * @return true if it is valid, false otherwise
     */
    public final boolean validMachine(byte m) {
        return (m>=0) && (m<=9);
    }
    
    /**
     * Test if the address is valid
     * 
     * @return true if it is valid, false otherwise
     */
    public boolean is_valid() {
        return validNetwork(_network) && validMachine(_machine);
    }
    
    /**
     * Reset the address contents to an empty address
     */
    public final void reset() {
        _network= ' ';   // Invalid
        _machine= -1;    // Invalid
    }
    
    /**
     * Parse the machine number of the address from an input string
     * 
     * @param str   input string
     * @return true if a valid machine number was parsed, false otherwise
     */
    private boolean parseMachine(char str) {
        try {
            _machine= (byte)Integer.parseInt(String.valueOf(str));
            if (!validMachine(_machine)) {
                _machine= -1;
                return false;
            }
            return true;
        }
        catch (NumberFormatException e) {
            _machine= -1;
            return false;
        }
    }

    /**
     * Parse the address from an input string
     * 
     * @param str   input string
     * @return true if a valid address was parsed, false otherwise
     */
    public boolean parseAddress(String str) {
        if (str == null)
            return false;
        if (str.length() != 3)
            return false;
        if (str.equals("0.0") || !validNetwork(str.charAt(0)) || 
                !parseMachine(str.charAt(2))) {
            reset();
            return false;
        }
        _network= str.charAt(0);
        return true;
    }

    /**
     * Write the address contents to an outstream buffer - to prepare messages 
     * 
     * @param dos   data output stream 
     * @return true if was successful
     */
    public boolean writeAddress(DataOutputStream dos) {
        try {
            dos.writeChar(_network);
            dos.writeByte(_machine);
            return true;
        }
        catch (Exception e) {
            System.out.println("Internal error writing Address in buffer: "+e+"\n");
            return false;
        }                
    }
    
    /**
     * Read the address contents from an input stream - from a message
     * 
     * @param dis   data input stream
     * @throws java.io.IOException 
     */
    public final void readAddress(DataInputStream dis) throws java.io.IOException {
        _network= dis.readChar();
        _machine= dis.readByte();
        if (!is_valid()) {
            throw new IOException("Invalid address '"+toString()+"'");
        }
    }
    
}
