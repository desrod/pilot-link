/* 
 * Dlp.java:
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

public class Dlp extends Socket {
   Dlp( int sock) { super( sock); }
   /**
    * These are the real external calls.
    */
   public void close( ) throws DlpException, IOException {
      /* This method must be idempotent */
      if( socket != 0) 
         {
            endOfSync( socket, Constants.dlpEndCodeNormal);
            super.close( );
            }
      }
   public void close( int status) throws DlpException, IOException {
      /* This method must be idempotent */
      if( socket != 0) 
         {
            endOfSync( socket, status);
            super.close( );
            }
      }
   public void close( End status) throws DlpException, IOException {
      /* This method must be idempotent */
      if( socket != 0)  
         {
            endOfSync( socket, status.getValue( ));
            super.close( );
            }
      }
   public void reset( ) throws DlpException {
      resetSystem( socket);
      }
   public DB open( Database dbClass) throws DlpException {
      int handle = openDB( socket, 0, 0x40 | 0x80 | 0x20, dbClass.dbname( ));
      return new DB( this, handle, dbClass.dbname( ), 0x40 | 0x80 | 0x20, 0, dbClass);
      }
   public DB open( String name) throws DlpException { return open( name, 0x40 | 0x80 | 0x20); }
   public DB open( String name, int mode) throws DlpException { return open( name, mode, 0); }
   public DB open( String name, int mode, int card) throws DlpException {
      int handle = openDB( socket, card, mode, name);
      return new DB( this, handle, name, mode, card);
      }
   public DB open( String name, String mode) throws DlpException { return open( name, mode, 0); }
   public DB open( String name, String mode, int card) throws DlpException {
      int imode = 0;
      if( mode.indexOf( 'r') >= 0) imode |= Constants.dlpOpenRead;
      if( mode.indexOf( 'w') >= 0) imode |= Constants.dlpOpenWrite;
      if( mode.indexOf( 's') >= 0) imode |= Constants.dlpOpenSecret;
      if( mode.indexOf( 'x') >= 0) imode |= Constants.dlpOpenExclusive;
      int handle = openDB( socket, card, imode, name);
      return new DB( this, handle, name, imode, card);
      }
   public DB create( String name, Char4 creator, Char4 type, int flags, int version) throws DlpException { 
      return create( name, creator, type, flags, version, 0); 
      }
   public DB create( String name, Char4 creator, Char4 type, int flags, int version, int card) throws DlpException {
      int handle = createDB( socket, creator, type, card, flags, version, name);
      return new DB( this, handle, name, 0x80 | 0x40 | 0x20, card);
      }
   public CardInfo getCardInfo( int card) throws DlpException {
      return readStorageInfo( socket, card);
      }
   public java.util.Date getTime( ) throws DlpException {
      return getSysDateTime( socket);
      }
   public UserInfo getUserInfo( ) throws DlpException {
      return readUserInfo(  socket);
      }
   public SysInfo getSysInfo( ) throws DlpException {
      return readSysInfo(  socket);
      }
   public void getStatus( ) throws DlpException, CancelSyncException {
      openConduit( socket);
      }
   public DBInfo getDBInfo( int start, boolean RAM, boolean ROM, int card) throws DlpException {
      return readDBList( socket,
                         card, 
                         ( RAM ? Constants.dlpDBListRAM : 0) |
                            ( ROM ? Constants.dlpDBListROM : 0),
                         start);
      }
   public int getFeature( Char4 type, int id) throws DlpException {
      return readFeature( socket, type, id);
      }
   public Pref getPref( Char4 creator, int id) throws DlpException {
      return getPref( creator, id, true); 
      }
   public Pref getPref( Char4 creator, int id, boolean backup) throws DlpException {
      Database dbClass = ( Database)Database.prefClasses.get( creator);
      if (dbClass == null) dbClass = Database.defaultPrefClass;
      return readAppPreference( socket, creator, id, backup, dbClass);
      }
   public Pref newPref( Char4 creator, int id) throws DlpException {
      return newPref( creator, id, 1); 
      }
   public Pref newPref( Char4 creator, int id, int version) throws DlpException	{
      return newPref( creator, id, version, true); 
      }
   public Pref newPref( Char4 creator, int id, int version, boolean backup) throws DlpException	{
      Database dbClass = ( Database)Database.prefClasses.get( creator);
      if( dbClass == null) dbClass = Database.defaultPrefClass;
      return dbClass.newPref( null, creator, id, version, backup);
      }
   public void setPref( Pref pref) throws DlpException {
      writeAppPreference( socket, pref);
      }
   public NetInfo getNetInfo( ) throws DlpException {
      return readNetSyncInfo( socket);
      }
   public void setNetInfo( NetInfo info) throws DlpException {
      writeNetSyncInfo( socket, info);
      }
   public void log( String message) throws DlpException {
      addSyncLogEntry( socket, message);
      }
   public byte[ ]callApplication( Char4 creator, 
                                  int type, 
                                  int action, 
                                  byte[ ]data, 
                                  int[ ]retcode) throws DlpException {
      return callApplication( socket, creator, type, action, data, retcode);
      }
   public void delete( String dbname) throws DlpException {
      delete( dbname, 0); 
      }
   public void delete( String dbname, int card) throws DlpException {
      deleteDB( socket, card, dbname);
      }
   // Some package private methods used for instance by DB to pass handles
   // I am not at all sure that these should be here, but for the time being
   // I will let them be.
   int closeDB( int handle) throws DlpException {
      return closeDB( socket, handle);
      }
   RecordID[ ]readRecordIDList( int handle, boolean sort, int start, int max) throws DlpException {
      return readRecordIDList( socket, handle, sort, start, max);
      }
   Pref readAppPreference( Char4 creator, int id, boolean backup, Database dbClass) throws DlpException {
      return readAppPreference( socket, creator, id, backup, dbClass);
      }
   int writeAppPreference( Pref pref) throws DlpException {
      return writeAppPreference( socket, pref);
      }
   int openDB( int card, int mode, String name) throws DlpException {
      return openDB( socket, card, mode, name);
      }
   int readOpenDBInfo( int handle) throws DlpException {
      return readOpenDBInfo( socket, handle);
      }
   int resetDBIndex( int handle) throws DlpException {
      return resetDBIndex( socket, handle);
      }
   int resetSyncFlags( int handle) throws DlpException {
      return resetSyncFlags( socket, handle);
      }
   int cleanUpDatabase( int handle) throws DlpException {
      return cleanUpDatabase( socket, handle);
      }
   int moveCategory( int handle, int from, int to) throws DlpException {
      return moveCategory( socket, handle, from, to);
      }
   int deleteCategory( int handle, int category) throws DlpException {
      return deleteCategory( socket, handle, category);
      }
   int deleteResource( int handle, boolean all, Char4 type, int id) throws DlpException {
      return deleteResource( socket, handle, all, type, id);
      }
   int deleteRecord( int handle, boolean all, RecordID id) throws DlpException {
      return deleteRecord( socket, handle, all, id);
      }
   int writeSortBlock( int handle, SortBlock sortblock) throws DlpException {
      return writeSortBlock( socket, handle, sortblock);
      }
   int writeAppBlock( int handle, AppBlock appblock) throws DlpException {
      return writeAppBlock( socket, handle, appblock);
      }
   int writeResource( int handle, Resource resource) throws DlpException {
      return writeResource( socket, handle, resource);
      }
   int writeRecord( int handle, Record record) throws DlpException {
      return writeRecord( socket, handle, record);
      }
   Resource readResourceByType( int handle, Char4 type, int id, Database dbClass) throws DlpException {
      return readResourceByType( socket, handle, type, id, dbClass);
      }
   Record readNextModifiedRecInCategory( int handle, int category, Database dbClass) throws DlpException {
      return readNextModifiedRecInCategory( socket, handle, category, dbClass);
      }
   Resource readResourceByIndex( int handle, int index, Database dbClass) throws DlpException {
      return readResourceByIndex( socket, handle, index, dbClass);
      }
   Record readNextModifiedRec( int handle, Database dbClass) throws DlpException {
      return readNextModifiedRec( socket, handle, dbClass);
      }
   Record readNextRecInCategory( int handle, int category, Database dbClass) throws DlpException {
      return readNextRecInCategory( socket, handle, category, dbClass);
      }
   AppBlock readAppBlock( int handle, Database dbClass) throws DlpException {
      return readAppBlock( socket, handle, dbClass);
      }
   SortBlock readSortBlock( int handle, Database dbClass) throws DlpException {
      return readSortBlock( socket, handle, dbClass);
      }
   Record readRecordByID( int handle, RecordID id, Database dbClass) throws DlpException {
      return readRecordByID( socket, handle, id, dbClass);
      }
   Record readRecordByIndex( int handle, int id, Database dbClass) throws DlpException {
      return readRecordByIndex( socket, handle, id, dbClass);
      }
   // Now for all the native methods.  
   synchronized private native int openDB(int socket, int card, int mode, String name) throws DlpException;
   synchronized private native int deleteDB(int socket, int card, String name) throws DlpException;
   synchronized private native int endOfSync(int socket, int status) throws DlpException;
   synchronized private native int deleteCategory(int socket, int handle, int category) throws DlpException;
   synchronized private native int closeDB(int socket, int handle) throws DlpException;
   synchronized private native int addSyncLogEntry(int socket, String entry) throws DlpException;
   synchronized native static String strerror(int error);
   synchronized private native java.util.Date getSysDateTime(int socket) throws DlpException;
   synchronized private native int setSysDateTime(int socket, java.util.Date date) throws DlpException;
   synchronized private native AppBlock readAppBlock(int socket, int handle, Database dbClass) throws DlpException;
   synchronized private native SortBlock readSortBlock(int socket, int handle, Database dbClass) throws DlpException;
   synchronized private native Pref readAppPreference(int socket, Char4 creator, int id, boolean backup, Database dbClass) throws DlpException;
   synchronized private native Record readRecordByIndex(int socket, int handle, int index, Database dbClass) throws DlpException;
   synchronized private native Record readNextModifiedRec(int socket, int handle, Database dbClass) throws DlpException;
   synchronized private native Record readNextModifiedRecInCategory(int socket, int handle, int category, Database dbClass) throws DlpException;
   synchronized private native Record readNextRecInCategory(int socket, int handle, int category, Database dbClass) throws DlpException;
   synchronized private native Record readRecordByID(int socket, int handle, RecordID id, Database dbClass) throws DlpException;
   synchronized private native Resource readResourceByType(int socket, int handle, Char4 type, int id, Database dbClass) throws DlpException;
   synchronized private native Resource readResourceByIndex(int socket, int handle, int index, Database dbClass) throws DlpException;
   synchronized private native int writeRecord(int socket, int handle, Record record) throws DlpException;
   synchronized private native int writeAppPreference(int socket, Pref pref) throws DlpException;
   synchronized private native int writeResource(int socket, int handle, Resource resource) throws DlpException;
   synchronized private native int writeAppBlock(int socket, int handle, AppBlock appblock) throws DlpException;
   synchronized private native int writeSortBlock(int socket, int handle, SortBlock sortblock) throws DlpException;
   synchronized private native CardInfo readStorageInfo(int socket, int card) throws DlpException;
   synchronized private native int createDB(int socket, Char4 creator, Char4 type, int card, int flags, int version, String name) throws DlpException;
   synchronized private native int resetSystem(int socket) throws DlpException;
   synchronized private native int openConduit(int socket) throws DlpException;
   synchronized private native UserInfo readUserInfo(int socket) throws DlpException;
   synchronized private native int writeUserInfo(int socket, UserInfo info) throws DlpException;
   synchronized private native SysInfo readSysInfo(int socket) throws DlpException;
   synchronized private native NetInfo readNetSyncInfo(int socket) throws DlpException;
   synchronized private native int writeNetSyncInfo(int socket, NetInfo info) throws DlpException;
   synchronized private native DBInfo readDBList(int socket, int card, int flags, int start) throws DlpException;
   synchronized private native int cleanUpDatabase(int socket, int handle) throws DlpException;
   synchronized private native int resetSyncFlags(int socket, int handle) throws DlpException;
   synchronized private native int moveCategory(int socket, int handle, int from, int to) throws DlpException;
   synchronized private native int deleteRecord(int socket, int handle, boolean all, RecordID id) throws DlpException;
   synchronized private native int deleteResource(int socket, int handle, boolean all, Char4 type, int id) throws DlpException;
   synchronized private native int readOpenDBInfo(int socket, int handle) throws DlpException;
   synchronized private native int readFeature(int socket, Char4 creator, int id) throws DlpException;
   synchronized private native RecordID[ ]readRecordIDList(int socket, int handle, boolean sort, int start, int max) throws DlpException;
   synchronized private native byte[ ]callApplication(int socket, Char4 creator, int type, int action, byte[] argument, int[] retcode) throws DlpException;
   synchronized private native int resetDBIndex(int socket, int handle) throws DlpException;
   }
