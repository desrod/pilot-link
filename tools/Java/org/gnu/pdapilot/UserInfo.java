/* 
 * UserInfo.java:
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
import java.util.Date;
import org.gnu.pdapilot.Util;

public class UserInfo {
   public int userID, viewerID, lastSyncPC;
   public Date successfulSyncDate, lastSyncDate;
   public String username;
   public byte[ ] password;
   public String toString( ) {
      return "<org.gnu.pdapilot.UserInfo userID " + userID + 
             ", viewerID " + viewerID + 
             ", lastSyncPC " + lastSyncPC +
             ", successfulSyncDate " + ( ( successfulSyncDate == null) ? "null" : successfulSyncDate.toString( )) + 
             ", syncLast " + ( ( lastSyncDate == null) ? "null" : lastSyncDate.toString( )) +
             ", username '" + username + 
             "', password '" + Util.prettyPrint( password) + "'>";
      }
   }

