/* 
 * Socket.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 * Copyright (C) 2001 David.Goodenough@DGA.co.uk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

package org.gnu.pdapilot;

import java.io.*;

/** 
 * A representation of a pilot-link socket. All pilot-link socket functions
 * can be performed on these objects. When a DLP (Sync) connection is
 * established via accept(), a Dlp object is returned.
 *
 * @see Dlp
 * @author Kenneth Albanowski
 */

public class Socket {
   static { // Make sure that the library is loaded
      System.loadLibrary("PiJni");
      }
   int socket;
   /**
    * Construct a Socket from a domain/type/protocol three-tuple. Use
    * values from constants for the arguments.
    * @param	domain	the domain the socket needs to operate in. Please use constants.PI_AF_SLP, constants.PI_AF_INETSLP, or constants.AF_INET.
    * @param	type	the type of socket to construct. Please use constants.PI_SOCK_STREAM or constants.PI_SOCK_RAW.
    * @param	protocol	the protocol speaken by the socket. May be left zero, or use constants.PI_PF_SLP for a raw connection, or constants.PI_PF_PADP for a stream connection.
    * @see constants
    */
   public Socket( int domain, int type, int protocol) throws IOException { 
      this.socket = socket( domain, type, protocol); 
      }
   public Socket( int sock) { this.socket = sock; }
   public Socket( String port) throws IOException {
      this.socket = socket( 0x51, 0x10, 0x51);
      this.bind( port);
      this.listen( 1);
      }
   synchronized public int bind( String device) throws IOException { 
      return bind( socket, device); 
      }
   synchronized public int listen( int backlog) throws IOException { 
      return listen( socket, backlog); 
      }
   synchronized public Dlp accept( ) throws IOException { 
      return new Dlp( accept(socket)); 
      }
   synchronized public int version( ) throws IOException { 
      return version( socket); 
      }
   synchronized public int tickle( ) throws IOException { 
      return tickle( socket); 
      }
   synchronized public int watchdog( int interval) throws IOException { 
      return watchdog( socket, interval); 
      }
   synchronized public int read( byte[ ]data, int len) throws IOException { 
      return read( socket, data, len); 
      }
   synchronized public int write( byte[ ]data, int len) throws IOException { 
      return write( socket, data, len); 
      }
   synchronized public void close( ) throws IOException, DlpException { 
      /* this method must be idempotent */
      if( this.socket != 0) close( socket);
      this.socket = 0;
      }
   protected void finalize( ) throws IOException, DlpException {
      close( );
      }
   private native int close( int socket) throws IOException;
   private native int socket( int domain, int type, int protocol) throws IOException;
   private native int bind( int socket, String device) throws IOException;
   private native int listen( int socket, int backlog) throws IOException;
   private native int accept( int socket) throws IOException;
   private native int version( int socket) throws IOException;
   private native int tickle( int socket) throws IOException;
   private native int watchdog( int socket, int interval) throws IOException;
   private native int read( int socket, byte[ ]data, int len) throws IOException;
   private native int write( int socket, byte[ ]data, int len) throws IOException;
   }
