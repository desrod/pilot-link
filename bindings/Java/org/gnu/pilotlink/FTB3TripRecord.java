/*
 * Created on 15.02.2004
 *
 * To change the template for this generated file go to
 * Window>Preferences>Java>Code Generation>Code and Comments
 */
package org.gnu.pilotlink;

import java.util.Date;

/**
 * @author stephan
 *
 * To change the template for this generated type comment go to
 * Window>Preferences>Java>Code Generation>Code and Comments
 */
public class FTB3TripRecord extends Record {
	private String startLocation;
	private String destLocation;
	private Date startTime;
	private Date arrivalTime;
	private boolean priv;
	private boolean storno;
	private String note;
	private long startMileage;
	private long endMileage;
	private long carID;
	
	private boolean isCost;
	private double sum;
	private double fuel;
	private double fuelCost;
	private long[] cat;
	private double[] cost;
	/**
	 * 
	 */
	public FTB3TripRecord() {
		super();

	}

	/**
	 * @param i
	 * @param att
	 * @param cat
	 * @param sz
	 */
	public FTB3TripRecord(int i, int att, int cat, int sz) {
		super(i, att, cat, sz);
	
	}

	/**
	 * @param r
	 */
	public FTB3TripRecord(Record r) {
		super(r);
	
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#getBuffer()
	 */
	public byte[] getBuffer() {
		byte buffer[]=new byte[65535];
		
		Record.setStringAt(buffer,startLocation,0x28);
		Record.setStringAt(buffer,destLocation,0x68);
		Record.setDateTimeAt(buffer,startTime,0x12,0x16);
		Record.setDateTimeAt(buffer,arrivalTime,0x14,0x18);
		Record.setStringAt(buffer,note,0xa8);
		Record.setLongAt(buffer,startMileage*10,0x1a);
		
		Record.setLongAt(buffer,carID,0x0a);
		if (!priv) buffer[0x101]=1;
		if (storno) buffer[0x102]=1;
		
		if (isCost) {
			Record.setLongAt(buffer,0,0x1e);
			//Cost
			isCost=true;
			Record.setDoubleAt(buffer,sum,0x28);
			Record.setDoubleAt(buffer,fuel,0x30);
			Record.setDoubleAt(buffer,fuelCost,0x38);
			for (int i=0; i<4; i++) {
				Record.setLongAt(buffer,cat[i],0x40+i*4);
				setDoubleAt(buffer,cost[i],0x50+i*8);
			}
			
		} else {
			Record.setLongAt(buffer,endMileage*10,0x1e);
		}
		
		return buffer;
	}

	/* (non-Javadoc)
	 * @see org.gnu.pilotlink.Record#setBuffer(byte[])
	 */
	public void setBuffer(byte[] buf) {
		this.cat=new long[4];
		cost=new double[4];
		
		startLocation=Record.getStringAt(buf,0x28);
		destLocation=Record.getStringAt(buf,0x68);
		startTime=Record.getDateTimeAt(buf,0x12,0x16);
		arrivalTime=Record.getDateTimeAt(buf,0x14,0x18);
		note=Record.getStringAt(buf,0xa8);
		startMileage=Record.getLongAt(buf,0x1a)/10;
		endMileage=Record.getLongAt(buf,0x1e)/10;
		carID=Record.getLongAt(buf,0x0a);
		priv=(buf[0x101]==0);
		storno=(buf[0x102]==1);
		
		if (endMileage==0) {
			//Cost
			isCost=true;
			sum=Record.getDoubleAt(buf,0x28);
			fuel=Record.getDoubleAt(buf,0x30);
			fuelCost=Record.getDoubleAt(buf,0x38);
			for (int i=0; i<4; i++) {
				cat[i]=Record.getLongAt(buf,0x40+i*4);
				cost[i]=getDoubleAt(buf,0x50+i*8);
			}
			
		}
		setSize(buf.length);
	}

	/**
	 * @return Returns the arrivalTime.
	 */
	public Date getArrivalTime() {
		return arrivalTime;
	}

	/**
	 * @param arrivalTime The arrivalTime to set.
	 */
	public void setArrivalTime(Date arrivalTime) {
		this.arrivalTime= arrivalTime;
	}

	/**
	 * @return Returns the carID.
	 */
	public long getCarID() {
		return carID;
	}

	/**
	 * @param carID The carID to set.
	 */
	public void setCarID(long carID) {
		this.carID= carID;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the cat.
	 */
	public long[] getCat() {
		return cat;
	}

	/**
	 * @param cat The cat to set.
	 */
	public void setCat(long[] cat) {
		this.cat= cat;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the cost.
	 */
	public double[] getCost() {
		return cost;
	}

	/**
	 * @param cost The cost to set.
	 */
	public void setCost(double[] cost) {
		this.cost= cost;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the destLocation.
	 */
	public String getDestLocation() {
		return destLocation;
	}

	/**
	 * @param destLocation The destLocation to set.
	 */
	public void setDestLocation(String destLocation) {
		this.destLocation= destLocation;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the endMileage.
	 */
	public long getEndMileage() {
		return endMileage;
	}

	/**
	 * @param endMileage The endMileage to set.
	 */
	public void setEndMileage(long endMileage) {
		this.endMileage= endMileage;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the fuel.
	 */
	public double getFuel() {
		return fuel;
	}

	/**
	 * @param fuel The fuel to set.
	 */
	public void setFuel(double fuel) {
		this.fuel= fuel;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the fuelCost.
	 */
	public double getFuelCost() {
		return fuelCost;
	}

	/**
	 * @param fuelCost The fuelCost to set.
	 */
	public void setFuelCost(float fuelCost) {
		this.fuelCost= fuelCost;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the isCost.
	 */
	public boolean isCost() {
		return isCost;
	}

	/**
	 * @param isCost The isCost to set.
	 */
	public void setCost(boolean isCost) {
		this.isCost= isCost;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the note.
	 */
	public String getNote() {
		return note;
	}

	/**
	 * @param note The note to set.
	 */
	public void setNote(String note) {
		this.note= note;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the priv.
	 */
	public boolean isPriv() {
		return priv;
	}

	/**
	 * @param priv The priv to set.
	 */
	public void setPriv(boolean priv) {
		this.priv= priv;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the startLocation.
	 */
	public String getStartLocation() {
		return startLocation;
	}

	/**
	 * @param startLocation The startLocation to set.
	 */
	public void setStartLocation(String startLocation) {
		this.startLocation= startLocation;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the startMileage.
	 */
	public long getStartMileage() {
		return startMileage;
	}

	/**
	 * @param startMileage The startMileage to set.
	 */
	public void setStartMileage(long startMileage) {
		this.startMileage= startMileage;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the startTime.
	 */
	public Date getStartTime() {
		return startTime;
	}

	/**
	 * @param startTime The startTime to set.
	 */
	public void setStartTime(Date startTime) {
		this.startTime= startTime;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the storno.
	 */
	public boolean isStorno() {
		return storno;
	}

	/**
	 * @param storno The storno to set.
	 */
	public void setStorno(boolean storno) {
		this.storno= storno;
		setSize(getBuffer().length);
	}

	/**
	 * @return Returns the sum.
	 */
	public double getSum() {
		return sum;
	}

	/**
	 * @param sum The sum to set.
	 */
	public void setSum(double sum) {
		this.sum= sum;
		setSize(getBuffer().length);
	}
	
	public String toString() {
		String out="";
		if (isCost) {
			out="cost "+getSum();
		} else {
			out="trip "+getStartLocation()+" ("+getStartMileage()+"km) ->"+getDestLocation()+"("+getEndMileage()+")";
		}
		return out;
	}

}
