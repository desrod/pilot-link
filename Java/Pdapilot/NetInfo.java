
package Pdapilot;

public class NetInfo {
	public boolean lanSync;
	public String hostName, hostAddress, hostSubnetMask;
	
	public String toString() {
		return "<Pdapilot.NetSyncInfo lanSnc "+lanSync+", hostName "+hostName+", hostAddress "+
			hostAddress+", hostSubnetMask "+hostSubnetMask+">";
	}
}

