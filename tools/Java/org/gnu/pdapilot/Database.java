/* 
 * Database.java:
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

public class Database {
   public Record newRecord( ) { return new Record( ); }
   public Record newRecord( RecordID id) { return new Record( null, id, 0, 0, 0); }
   public Record newRecord( byte[ ] contents, RecordID id, int index, int attr, int cat) { 
      return new Record( contents, id, index, attr, cat); 
      }
   public Resource newResource( ) { return new Resource( ); }
   public Resource newResource( Char4 type, int id) { return newResource( null, type, id, 0); }
   public Resource newResource( byte[ ] contents, Char4 type, int id, int index) { 
      return new Resource( contents, type, id, index); 
      }
   public Pref newPref( ) { return new Pref( ); }
   public Pref newPref( byte[ ] contents, Char4 creator, int id, int version, boolean backup) { 
      return new Pref( contents, creator, id, version, backup); 
      }
   public AppBlock newAppBlock( ) { return new AppBlock( ); } 
   public SortBlock newSortBlock( ) { return new SortBlock( ); }
   public AppBlock newAppBlock( byte[ ] contents) { return new AppBlock( contents); } 
   public SortBlock newSortBlock( byte[ ] contents) { return new SortBlock( contents); }
   public Char4 creator( ) { return null; }
   public String dbname( ) { return null; }
   static public java.util.Hashtable dbClasses = new java.util.Hashtable( 10);
   static public java.util.Hashtable prefClasses = new java.util.Hashtable( 10);
   static public Database defaultDbClass;
   static public Database defaultPrefClass;
   static {
      defaultDbClass = defaultPrefClass = new Database( );
      org.gnu.pdapilot.memo.Database memo = new org.gnu.pdapilot.memo.Database();
      org.gnu.pdapilot.Database.dbClasses.put(memo.dbname(), memo);
      org.gnu.pdapilot.Database.prefClasses.put(memo.creator(), memo);
      org.gnu.pdapilot.todo.Database todo = new org.gnu.pdapilot.todo.Database();
      org.gnu.pdapilot.Database.dbClasses.put(todo.dbname(), todo);
      org.gnu.pdapilot.Database.prefClasses.put(todo.creator(), todo);
      org.gnu.pdapilot.mail.Database mail = new org.gnu.pdapilot.mail.Database();
      org.gnu.pdapilot.Database.dbClasses.put(mail.dbname(), mail);
      org.gnu.pdapilot.Database.prefClasses.put(mail.creator(), mail);
      org.gnu.pdapilot.appointment.Database appointment = new org.gnu.pdapilot.appointment.Database();
      org.gnu.pdapilot.Database.dbClasses.put(appointment.dbname(), appointment);
      org.gnu.pdapilot.Database.prefClasses.put(appointment.creator(), appointment);
      org.gnu.pdapilot.expense.Database expense = new org.gnu.pdapilot.expense.Database();
      org.gnu.pdapilot.Database.dbClasses.put(expense.dbname(), expense);
      org.gnu.pdapilot.Database.prefClasses.put(expense.creator(), expense);
      org.gnu.pdapilot.address.Database address = new org.gnu.pdapilot.address.Database();
      org.gnu.pdapilot.Database.dbClasses.put(address.dbname(), address);
      org.gnu.pdapilot.Database.prefClasses.put(address.creator(), address);
      }
   }
