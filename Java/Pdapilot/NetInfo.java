
package Pdapilot;

public class NetInfo {
	public boolean LANsync;
	public String hostName, hostAddress, hostSubnetMask;
	
	public String toString() {
		return "<Pdapilot.NetSyncInfo LANsync "+LANsync+", hostName "+hostName+", hostAddress "+
			hostAddress+", hostSubnetMask "+hostSubnetMask+">";
	}
}

