/* 
 * DB.java:
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

/** A representation of an open database.
 */

public class DB {
   Dlp dlp;
   int handle;
   String dbname;
   int dbmode;
   int dbcard;
   public Database dbClass;
   DB( Dlp dlp, int handle, String dbname, int dbmode, int dbcard) {
      this.dlp = dlp;
      this.handle = handle;
      this.dbname = dbname;
      this.dbmode = dbmode;
      this.dbcard = dbcard;
      System.out.println( "Opening database '"+dbname+"'");
      dbClass = ( Database)Database.dbClasses.get( dbname);
      if( dbClass == null) 
         {
            dbClass = Database.defaultDbClass;
            System.out.println( "Using default dbclass");
         }
      }
   DB( Dlp dlp, int handle, String dbname, int dbmode, int dbcard, Database dbClass) {
      this.dlp = dlp;
      this.handle = handle;
      this.dbname = dbname;
      this.dbmode = dbmode;
      this.dbcard = dbcard;
      this.dbClass = dbClass;
      System.out.println("Opening database '"+dbname+"'");
      }
   public void close( ) throws DlpException { 
      /* this method should be idempotent */
      if( handle != 0) dlp.closeDB( handle);
      handle = 0;
      }
   protected void finalize( ) throws DlpException {
      close( );
      }
   public AppBlock getAppBlock( ) throws DlpException {
      return dlp.readAppBlock( handle, dbClass);
      }
   public AppBlock newAppBlock( ) {
      return dbClass.newAppBlock( );
      }
   public SortBlock getSortBlock( ) throws DlpException {
      return dlp.readSortBlock( handle, dbClass);
      }
   public SortBlock newSortBlock( ) {
      return dbClass.newSortBlock( ); 
      }
   public Record getRecord( int index) throws DlpException {
      return dlp.readRecordByIndex( handle, index, dbClass);
      }
   public Record newRecord( ) {
      return dbClass.newRecord();
      }
   public Record newRecord( RecordID id) {
      return dbClass.newRecord( id);
      }
   public Resource newResource( ) {
      return dbClass.newResource( );
      }
   public Resource newResource( Char4 type, int id) {
      return dbClass.newResource( type, id);
      }
   public Record getRecord( RecordID id) throws DlpException {
      return dlp.readRecordByID( handle, id, dbClass);
      }
   public Record getNextRecord( int category) throws DlpException {
      return dlp.readNextRecInCategory( handle, category, dbClass);
      }
   public Record getNextModRecord( ) throws DlpException {
      return dlp.readNextModifiedRec( handle, dbClass);
      }
   public Record getNextModRecord( int category) throws DlpException {
      return dlp.readNextModifiedRecInCategory( handle, category, dbClass);
      }
   public Resource getResource( int index) throws DlpException {
      return dlp.readResourceByIndex( handle, index, dbClass);
      }
   public Resource getResource( Char4 type, int id) throws DlpException {
      return dlp.readResourceByType( handle, type, id, dbClass);
      }
   public long setRecord( Record record) throws DlpException {
      return dlp.writeRecord( handle, record);
      }
   public void setResource( Resource resource) throws DlpException {
      dlp.writeResource( handle, resource);
      }
   public void setAppBlock( AppBlock appblock) throws DlpException {
      dlp.writeAppBlock( handle, appblock);
      }
   public void setSortBlock( SortBlock sortblock) throws DlpException {
      dlp.writeSortBlock( handle, sortblock);
      }
   public void deleteRecord( RecordID id) throws DlpException {
      dlp.deleteRecord( handle, false, id);
      }
   public void deleteRecords( ) throws DlpException {
      dlp.deleteRecord( handle, true, null);
      }
   public void deleteResource( Char4 type, int id) throws DlpException {
      dlp.deleteResource( handle, false, type, id);
      }
   public void deleteResources( ) throws DlpException {
      dlp.deleteResource( handle, true, null, 0);
      }
   public void deleteCategory( int category) throws DlpException {
      dlp.deleteCategory( handle, category);
      }
   public void moveCategory( int from, int to) throws DlpException {
      dlp.moveCategory( handle, from, to);
      }
   public void purge( ) throws DlpException {
      dlp.cleanUpDatabase( handle);
      }
   public void resetFlags( ) throws DlpException {
      dlp.resetSyncFlags( handle);
      }
   public void resetNext( ) throws DlpException {
      dlp.resetDBIndex( handle);
      }
   public int getRecords( ) throws DlpException {
      return dlp.readOpenDBInfo( handle); 
      }
   public Pref getPref( int id) throws DlpException, IOException {	
      return getPref( id, true); 
      }
   public Pref getPref( int id, boolean backup) throws DlpException, NoCreatorException, IOException {
      if( dlp.version( ) < 0x101) dlp.closeDB( handle);
      if( dbClass.creator( ) == null) throw new NoCreatorException( );
      Pref result = dlp.readAppPreference( dbClass.creator( ), id, backup, dbClass);
      if( dlp.version( ) < 0x101) handle = dlp.openDB( dbcard, dbmode, dbname);
      return result;
      }
   public Pref newPref( int id) {
      return dbClass.newPref( null, dbClass.creator( ), id, 1, true);
      }
   public Pref newPref( int id, int version, boolean backup) {
      return dbClass.newPref( null, dbClass.creator( ), id, version, backup);
      }
   public void setPref( Pref pref) throws DlpException, IOException {
      if( dlp.version( ) < 0x101) dlp.closeDB( handle);
      dlp.writeAppPreference( pref);
      if( dlp.version( ) < 0x101) handle = dlp.openDB( dbcard, dbmode, dbname);
      }
   public RecordID[ ] getRecordIDs( ) throws DlpException {
      return getRecordIDs( false);	
      }
   public RecordID[ ] getRecordIDs( boolean sort) throws DlpException {
      return getRecordIDs( sort, 0);
      }
   public RecordID[ ] getRecordIDs( boolean sort, int start) throws DlpException {
      return getRecordIDs( sort, start, 0xffff);
      }
   public RecordID[ ] getRecordIDs( boolean sort, int start, int max) throws DlpException {
      return dlp.readRecordIDList( handle, sort, start, max);
      }
   }
