/* 
 * DBInfo.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
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

package Pdapilot;

public class DBInfo {
	public boolean flagReadOnly, flagResource, flagBackup, flagOpen;
   public boolean flagAppInfoDirty, flagNewer, flagReset, flagCopyPrevention, flagStream;
   public boolean flagExcludeFromSync;
	public Char4 creator, type;
	public int version;
	public int modnum;
	public java.util.Date createDate, modifyDate, backupDate;
	public int index;
	public String name;
	public int card;
	
	public boolean more;
	
	public String toString() {
		return "<Pdapilot.DBInfo name '"+name+"', version "+version+
			", flagReadOnly="+flagReadOnly+", flagResource="+flagResource+
			", flagOpen="+flagOpen+", flagBackup="+flagBackup+
			", flagAppInfoDirty="+flagAppInfoDirty+", flagNewer="+flagNewer+
         ", flagReset="+flagReset+", flagCopyPrevention="+flagCopyPrevention+  
         ", flagStream="+flagStream+", flagExcludeFromSync="+flagExcludeFromSync+
			", creator "+creator.toString()+
			", type "+type.toString()+", modnum "+modnum+
			", createDate " + ((createDate==null) ? "null" : createDate.toString())+
			", modifiyDate " + ((modifyDate==null) ? "null" : modifyDate.toString())+
			", backupDate " + ((backupDate==null) ? "null" : backupDate.toString())+
			", card "+card+
			", more "+more+">";
	}
}

