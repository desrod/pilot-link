/* NetInfo.java:
 *
 * Copyright (C) 1997, 1998, Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 */

package Pdapilot;

public class NetInfo {
	public boolean lanSync;
	public String hostName, hostAddress, hostSubnetMask;
	
	public String toString() {
		return "<Pdapilot.NetSyncInfo lanSnc "+lanSync+", hostName "+hostName+", hostAddress "+
			hostAddress+", hostSubnetMask "+hostSubnetMask+">";
	}
}

