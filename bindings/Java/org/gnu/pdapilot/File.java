/* 
 * File.java:
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

/** A representation of an open database.
 */

public class File {
	private int _pf;
	private Database dbClass;
	
	private native void pi_file_create(String name, DBInfo info) throws java.io.IOException;
	private native void pi_file_open(String name) throws java.io.IOException;
	private native Record pi_file_read_record(int index, Database dbClass) throws java.io.IOException;
	private native Record pi_file_read_record_by_id(RecordID id, Database dbClass) throws java.io.IOException;
	private native Resource pi_file_read_resource(int index, Database dbClass) throws java.io.IOException;
	private native int pi_file_get_entries() throws java.io.IOException;
	private native DBInfo pi_file_get_info() throws java.io.IOException;
	private native AppBlock pi_file_get_app_info(Database dbClass) throws java.io.IOException;
	private native SortBlock pi_file_get_sort_info(Database dbClass) throws java.io.IOException;
	private native void pi_file_set_info(DBInfo info) throws java.io.IOException;
	private native void pi_file_set_app_info(AppBlock app) throws java.io.IOException;
	private native void pi_file_set_sort_info(SortBlock sort) throws java.io.IOException;
	private native void pi_file_append_record(Record rec) throws java.io.IOException;
	private native void pi_file_append_resource(Resource rsc) throws java.io.IOException;
	private native void pi_file_close() throws java.io.IOException;
	private native void pi_file_retrieve(int socket, int card) throws java.io.IOException;
	private native void pi_file_install(int socket, int card) throws java.io.IOException;
	private native void pi_file_merge(int socket, int card) throws java.io.IOException;
	
	private File() {
		_pf = 0;
	}

	private File(String name) throws java.io.IOException {
		_pf = 0;
		dbClass = (Database)Database.dbClasses.get(name);
		if (dbClass == null) {
			dbClass = Database.defaultDbClass;
		}
		System.out.println("dbClass = "+dbClass);
		pi_file_open(name);
	}
	
	private File(String name, DBInfo info)  throws java.io.IOException {
		_pf = 0;
		dbClass = (Database)Database.dbClasses.get(name);
		if (dbClass == null) {
			dbClass = Database.defaultDbClass;
		}
		if (info != null)
			pi_file_create(name, info);
		else
			pi_file_open(name);
	}
	
	public static File open(String name) throws java.io.IOException  {
		return new File(name);
	}

	public static File create(String name, DBInfo info) throws java.io.IOException  {
		return new File(name, info);
	}
	
	public void close() throws java.io.IOException  {
		/* This function must be idempotent */
		pi_file_close();
	}
	
	public void finalize() throws java.io.IOException  {
		close();
	}
	
	public Record getRecord(int index) throws java.io.IOException  {
		return pi_file_read_record(index, dbClass);
	}

	public Record getRecord(RecordID id) throws java.io.IOException  {
		return pi_file_read_record_by_id(id, dbClass);
	}

	public void addRecord(Record newRecord) throws java.io.IOException  {
		pi_file_append_record(newRecord);
	}

	public void addResource(Resource newResource) throws java.io.IOException  {
		pi_file_append_resource(newResource);
	}

	public Record newRecord() throws java.io.IOException  {
		return dbClass.newRecord();
	}

	public Record newRecord(RecordID id) throws java.io.IOException  {
		return dbClass.newRecord(id);
	}

	public Resource newResource() throws java.io.IOException  {
		return dbClass.newResource();
	}

	public Resource newResource(Char4 type, int id) throws java.io.IOException  {
		return dbClass.newResource(type, id);
	}
		
	public Resource getResource(int index) throws java.io.IOException  {
		return pi_file_read_resource(index, dbClass);
	}

	public void setAppBlock(AppBlock appblock) throws java.io.IOException  {
		pi_file_set_app_info(appblock);
	}

	public void setSortBlock(SortBlock sortblock) throws java.io.IOException  {
		pi_file_set_sort_info(sortblock);
	}

	public AppBlock getAppBlock() throws java.io.IOException  {
		return pi_file_get_app_info(dbClass);
	}

	public SortBlock getSortBlock() throws java.io.IOException  {
		return pi_file_get_sort_info(dbClass);
	}
	
	public DBInfo getDBInfo() throws java.io.IOException  {
		return pi_file_get_info();
	}
	
	public void setDBInfo(DBInfo info) throws java.io.IOException  {
		pi_file_set_info(info);
	}

	public int getRecords() throws java.io.IOException  {
		return pi_file_get_entries();
	}
	
	public void install(Dlp connection, int cardno) throws java.io.IOException  {
		pi_file_install(connection.socket, cardno);
	}

	public void retrieve(Dlp connection, int cardno) throws java.io.IOException  {
		pi_file_retrieve(connection.socket, cardno);
	}

	public void merge(Dlp connection, int cardno) throws java.io.IOException  {
		pi_file_merge(connection.socket, cardno);
	}
	
	static {
		System.loadLibrary("JavaPisock");
	}
}

  
