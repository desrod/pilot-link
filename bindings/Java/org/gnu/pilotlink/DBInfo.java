package org.gnu.pilotlink;
import java.util.Date;

public class DBInfo
{
    public String name;
    public Date createDate;
    public Date modifyDate;
    public Date backupDate;
    public int type;
    public int creator;
    public int modnum;
    public short flags;
    public short version;
    public short index;
    public byte miscFlags;
    public byte more;

    // No-argument constructor
    public DBInfo()
    {
        name = new String();
        createDate = new Date();
        modifyDate = new Date();
        backupDate = new Date();
        type = 0;
        creator = 0;
        modnum = 0;
        flags = 0;
        version = 0;
        index = 0;
        miscFlags = 0;
        more = 0;
    }

    // Complete-specification constructor (used in native code)
    public DBInfo(String n_name, int n_create_sec_epoch, int n_modify_sec_epoch,
        int n_backup_sec_epoch, int n_type, int n_creator, int n_modnum,
        short n_flags, short n_version, short n_index, byte n_miscFlags, byte n_more)
    {
        name = n_name;
        setCreationDate(n_create_sec_epoch);
        setModifyDate(n_modify_sec_epoch);
        setBackupDate(n_backup_sec_epoch);
        type = n_type;
        creator = n_creator;
        modnum = n_modnum;
        flags = n_flags;
        version = n_version;
        index = n_index;
        miscFlags = n_miscFlags;
        more = n_more;
    }

    public void setCreationDate(int sec_epoch)
    {
        createDate = new Date(sec_epoch * 1000L);
    }

    public void setModifyDate(int sec_epoch)
    {
        modifyDate = new Date(sec_epoch * 1000L);
    }

    public void setBackupDate(int sec_epoch)
    {
        backupDate = new Date(sec_epoch * 1000L);
    }
}

