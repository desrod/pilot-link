#
# Pilot.pm - Interface glue for the pilot-link libraries and Perl.
#
# Copyright (C) 1997, 1998, Kenneth Albanowski
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Library General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
# 
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
# License for more details.
# 
# You should have received a copy of the GNU Library General Public License
# along with this library; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
#

package PDA::Pilot;

require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@ EXPORT = qw(PI_AF_SLP
              PI_PF_LOOP
              PI_PF_PADP
              PI_PF_SLP
              PI_PilotSocketConsole
              PI_PilotSocketDLP
              PI_PilotSocketDebugger
              PI_PilotSocketRemoteUI
              PI_SOCK_DGRAM
              PI_SOCK_RAW
              PI_SOCK_SEQPACKET
              PI_SOCK_STREAM
              dlpOpenRead
              dlpOpenWrite
              dlpOpenExclusive
              dlpOpenSecret
              dlpOpenReadWrite
              dlpEndCodeNormal
              dlpEndCodeOutOfMemory
              dlpEndCodeUserCan
              dlpEndCodeOther
              dlpRecAttrDeleted
              dlpRecAttrDirty
              dlpRecAttrBusy
              dlpRecAttrSecret
              dlpRecAttrArchived
              dlpDBFlagResource
              dlpDBFlagReadOnly
              dlpDBFlagAppInfoDirty
              dlpDBFlagBackup
              dlpDBFlagOpen
              dlpDBFlagNewer
              dlpDBFlagReset
              dlpDBFlagCopyPrevention
              dlpDBFlagStream 
	      dlpDBListRAM
	      dlpDBListROM
);

# Other items we are prepared to export if requested
@EXPORT_OK = qw(
	CompareTm
);

# This AUTOLOAD is used to 'autoload' constants from the constant() XS
# function.  If a constant is not found then control is passed to the
# AUTOLOAD in AutoLoader.

# NOTE: THIS AUTOLOAD FUNCTION IS FLAWED (but is the best we can do for
# now). Avoid old-style ``&CONST'' usage. Either remove the ``&'' or add
# ``()''.
sub AUTOLOAD {
    if (@_ > 0) {
	$AutoLoader::AUTOLOAD = $AUTOLOAD;
	goto &AutoLoader::AUTOLOAD;
    }
    local($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
	    ($pack,$file,$line) = caller;
	    die "Your vendor has not defined PDA::Pilot macro $constname, used at $file line $line.";
	}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}

bootstrap PDA::Pilot;
package PDA::Pilot::Block;

#####################################################################
#
# Generic functions for "blocks", i.e., chunks of data from a Pilot.
#
#####################################################################
sub new {
	my($self,$data) = @_;
	$self = bless {}, (ref($self) || $self);
	if (defined($data)) {
		$self->Unpack($data);
	} else {
		$self->Fill();
	}
	return $self;
}

#####################################################################
#
# Translate a "packed" block of binary data into a decent Perl 
# representation
#
#####################################################################
sub Unpack {
	my($self,$data) = @_;
	$self->{raw} = $data;
}


#####################################################################
#
# Translate a Perl representation of a data block into a string of 
# binary data
#
#####################################################################
sub Pack {
	my($self) = @_;
	return $self->{raw};
}

#####################################################################
#
# Just copy the "raw" item straight through
#
#####################################################################
sub Raw {
	my($self) = @_;
	return $self->{raw};
}

#####################################################################
#
# Fill in a block with default Perl data
#
#####################################################################
sub Fill {

}

#####################################################################
#
# Copy a block
#
#####################################################################
sub Clone {
	my($self) = @_;
	my($k,$v);
	my($new) = bless {}, ref($_);
	for (($k,$v) = each %$self) { $new->{$k} = $v }
	$new;
}

package PDA::Pilot::Record;

#####################################################################
#
# A class to represent generic database records
#
#####################################################################
@ISA = qw(PDA::Pilot::Block);

sub new {
	my($self,$data,$id,$attr,$cat,$index) = @_;

      $index = 0 unless defined($index);
      $id = 0 unless defined($id);
      $attr = 0 unless defined($attr);
      $cat = "" unless defined($cat);

	$self = bless { 
		"index" => $index,
		id => $id,
		deleted => !!($attr & 0x80),
		modified => !!($attr & 0x40),
		busy => !!($attr & 0x20),
		secret => !!($attr & 0x10),
		archived => !!($attr & 0x08),
		category => $cat}, 
		(ref($self) || $self);
	if (defined($data)) {
		$self->Unpack($data);
	} else {
		$self->Fill();
	}
	return $self;
}

package PDA::Pilot::Resource;

#####################################################################
#
# A class to represent generic database resources
#
#####################################################################
@ISA = qw(PDA::Pilot::Block);

sub new {
	my($self,$data,$index,$type,$id) = @_;
	$self = bless { "index" => $index, type => $type, id => $id}, 
		(ref($self) || $self);
	if (defined($data)) {
		$self->Unpack($data);
	} else {
		$self->Fill();
	}
	return $self;
}

package PDA::Pilot::AppBlock;

#####################################################################
# A class to represent generic application-information blocks
#####################################################################
@ISA = qw(PDA::Pilot::Block);

package PDA::Pilot::SortBlock;

#####################################################################
# A class to represent generic sort-information blocks
#####################################################################
@ISA = qw(PDA::Pilot::Block);

package PDA::Pilot::Pref;

#####################################################################
# A class to represent generic preference blocks
#####################################################################
@ISA = qw(PDA::Pilot::Block);

sub new {
	my($self,$data,$creator,$id,$version,$backup) = @_;
	$self = bless { creator => $creator, id => $id, version => $version, backup => $backup}, 
		(ref($self) || $self);
	if (defined($data)) {
		$self->Unpack($data);
	} else {
		$self->Fill();
	}
	return $self;
}

package PDA::Pilot::MemoRecord;

#####################################################################
# A class to represent records of the Memo application
#####################################################################
@ISA = qw(PDA::Pilot::Record);

sub Pack {
	my($self) = @_;
	return $self->{raw} = PDA::Pilot::Memo::Pack($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Memo::Unpack($self);
}

package PDA::Pilot::MemoAppBlock;

#####################################################################
# A class to represent records of the Memo application
#####################################################################
@ISA = qw(PDA::Pilot::AppBlock);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Memo::PackAppBlock($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Memo::UnpackAppBlock($self);
}

package PDA::Pilot::ToDoRecord;

#####################################################################
# A class to represent records of the ToDo application
#####################################################################
@ISA = qw(PDA::Pilot::Record);

sub Pack {
	my($self) = @_;
	return $self->{raw} = PDA::Pilot::ToDo::Pack($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::ToDo::Unpack($self);
}

package PDA::Pilot::ToDoAppBlock;

#####################################################################
# A class to represent the app block of the ToDo application
#####################################################################
@ISA = qw(PDA::Pilot::AppBlock);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::ToDo::PackAppBlock($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::ToDo::UnpackAppBlock($self);
}

package PDA::Pilot::AddressRecord;

#####################################################################
# A class to represent records of the Address application
#####################################################################
@ISA = qw(PDA::Pilot::Record);

sub Pack {
	my($self) = @_;
	return $self->{raw} = PDA::Pilot::Address::Pack($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Address::Unpack($self);
}

package PDA::Pilot::AddressAppBlock;

#####################################################################
# A class to represent the app block of the Address application
#####################################################################
@ISA = qw(PDA::Pilot::AppBlock);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Address::PackAppBlock($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Address::UnpackAppBlock($self);
}

package PDA::Pilot::AppointmentRecord;

#####################################################################
# A class to represent records of the Appointment application
#####################################################################
@ISA = qw(PDA::Pilot::Record);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Appointment::Pack($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Appointment::Unpack($self);
}

package PDA::Pilot::AppointmentAppBlock;

#####################################################################
# A class to represent the app block of the Appointment application
#####################################################################
@ISA = qw(PDA::Pilot::AppBlock);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Appointment::PackAppBlock($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Appointment::UnpackAppBlock($self);
}

package PDA::Pilot::MailRecord;

#####################################################################
# A class to represent records of the Mail application
#####################################################################
@ISA = qw(PDA::Pilot::Record);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Mail::Pack($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Mail::Unpack($self);
}

package PDA::Pilot::MailAppBlock;

#####################################################################
# A class to represent the app block of the Mail application
#####################################################################
@ISA = qw(PDA::Pilot::AppBlock);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Mail::PackAppBlock($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Mail::UnpackAppBlock($self);
}

package PDA::Pilot::MailSyncPref;

#####################################################################
# A class to represent the sync pref of the Mail application
#####################################################################
@ISA = qw(PDA::Pilot::Pref);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Mail::PackSyncPref($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Mail::UnpackSyncPref($self);
}

package PDA::Pilot::MailSignaturePref;

#####################################################################
# A class to represent the signature pref of the Mail application
#####################################################################
@ISA = qw(PDA::Pilot::Pref);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Mail::PackSignaturePref($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Mail::UnpackSignaturePref($self);
}

package PDA::Pilot::ExpenseRecord;

#####################################################################
# A class to represent records of the Expense application
#####################################################################
@ISA = qw(PDA::Pilot::Record);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Expense::Pack($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Expense::Unpack($self);
}

package PDA::Pilot::ExpenseAppBlock;

#####################################################################
# A class to represent the app block of the Expense application
#####################################################################
@ISA = qw(PDA::Pilot::AppBlock);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Expense::PackAppBlock($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Expense::UnpackAppBlock($self);
}

package PDA::Pilot::ExpensePref;

#####################################################################
# A class to represent the pref of the Expense application
#####################################################################
@ISA = qw(PDA::Pilot::Pref);

sub Pack {
	my($self) = @_;
	return PDA::Pilot::Expense::PackPref($self);
}

sub Unpack {
	my($self, $data) = @_;
	$self->{raw} = $data;
	PDA::Pilot::Expense::UnpackPref($self);
}

package PDA::Pilot::Database;

#####################################################################
# A class to specify which classes are used for generic database entries
#####################################################################
sub record { shift @_; PDA::Pilot::Record->new(@_) }
sub resource { shift @_; PDA::Pilot::Resource->new(@_) }
sub pref { shift @_; PDA::Pilot::Pref->new(@_) }
sub appblock { shift @_; PDA::Pilot::AppBlock->new(@_) }
sub sortblock { shift @_; PDA::Pilot::SortBlock->new(@_) }

package PDA::Pilot::MemoDatabase;

#####################################################################
# A class to specify which classes are used for Memo database entries
#####################################################################
@ISA=qw(PDA::Pilot::Database);

sub record { shift @_; PDA::Pilot::MemoRecord->new(@_) }
sub appblock { shift @_; PDA::Pilot::MemoAppBlock->new(@_) }
sub creator { 'memo' }
sub dbname { 'MemoDB' }

package PDA::Pilot::ToDoDatabase;

@ISA=qw(PDA::Pilot::Database);

sub record { shift @_; PDA::Pilot::ToDoRecord->new(@_) }
sub appblock { shift @_; PDA::Pilot::ToDoAppBlock->new(@_) }
sub creator { 'todo' }
sub dbname { 'ToDoDB' }

package PDA::Pilot::AppointmentDatabase;

@ISA=qw(PDA::Pilot::Database);

sub record { shift @_; PDA::Pilot::AppointmentRecord->new(@_) }
sub appblock { shift @_; PDA::Pilot::AppointmentAppBlock->new(@_) }
sub creator { 'date' }
sub dbname { 'DatebookDB' }

package PDA::Pilot::AddressDatabase;

@ISA=qw(PDA::Pilot::Database);

sub record { shift @_; PDA::Pilot::AddressRecord->new(@_) }
sub appblock { shift @_; PDA::Pilot::AddressAppBlock->new(@_) }
sub creator { 'addr' }
sub dbname { 'AddressDB' }

package PDA::Pilot::MailDatabase;

@ISA=qw(PDA::Pilot::Database);

sub record { shift @_; PDA::Pilot::MailRecord->new(@_) }
sub appblock { shift @_; PDA::Pilot::MailAppBlock->new(@_) }
sub pref {
	shift @_;
	my($data,$creator,$id) = @_;
	if (($id == 1) || ($id == 2)) {
		PDA::Pilot::MailSyncPref->new(@_);
	} elsif ($id == 3) {
		PDA::Pilot::MailSignaturePref->new(@_);
	} else {
		PDA::Pilot::Database->pref(@_);
	}
}
sub creator { 'mail' }
sub dbname { 'MailDB' }

package PDA::Pilot::ExpenseDatabase;

@ISA=qw(PDA::Pilot::Database);

sub record { shift @_; PDA::Pilot::ExpenseRecord->new(@_) }
sub appblock { shift @_; PDA::Pilot::ExpenseAppBlock->new(@_) }
sub pref {
	shift @_;
	my($data,$creator,$id) = @_;
	if ($id == 1) {
		PDA::Pilot::ExpensePref->new(@_);
	} else {
		PDA::Pilot::Database->pref(@_);
	}
}
sub creator { 'exps' }
sub dbname { 'ExpenseDB' }

package PDA::Pilot;

%DBClasses = (	MemoDB => 'PDA::Pilot::MemoDatabase',
				ToDoDB => 'PDA::Pilot::ToDoDatabase',
				AddressDB => 'PDA::Pilot::AddressDatabase',
				DatebookDB => 'PDA::Pilot::AppointmentDatabase',
				MailDB => 'PDA::Pilot::MailDatabase',
				ExpenseDB => 'PDA::Pilot::ExpenseDatabase'
				);
%PrefClasses = (	memo => 'PDA::Pilot::MemoDatabase',
				todo => 'PDA::Pilot::ToDoDatabase',
				mail => 'PDA::Pilot::MailDatabase',
				date => 'PDA::Pilot::AppointmentDatabase',
				addr => 'PDA::Pilot::AddressDatabase',
				exps => 'PDA::Pilot::ExpenseDatabase'
				);

#####################################################################
# Default classes
#####################################################################
$DBClasses{''} = 'PDA::Pilot::Database';
$PrefClasses{''} = 'PDA::Pilot::Database';

sub CompareTm {
	my(@a) = @{$_[0]};
	my(@b) = @{$_[1]};
	return ($a[5] <=> $b[5]) || ($a[4] <=> $b[4]) || ($a[3] <=> $b[3]) ||
	       ($a[2] <=> $b[2]) || ($a[1] <=> $b[1]) || ($a[0] <=> $b[0]);
}

#####################################################################
# Autoload methods go after __END__, and are processed by the 
# autosplit program.
#####################################################################

1;
__END__

=head1

Commands include:

B<Notice!> This information is out of date, and potentially quite
misleading.

=over 4

=item PDA::Pilot::Appointment::Unpack(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given a record from a .pdb file or a Pilot database.

=item PDA::Pilot::Appointment::Pack(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database.

=item PDA::Pilot::Appointment::UnpackAppInfo(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given the AppInfo record from a .pdb file or a Pilot database.

=item PDA::Pilot::Appointment::PackAppInfo(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database AppInfo block.

=item PDA::Pilot::Memo::Unpack(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given a record from a .pdb file or a Pilot database.

=item PDA::Pilot::Memo::Pack(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database.

=item PDA::Pilot::Memo::UnpackAppInfo(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given the AppInfo record from a .pdb file or a Pilot database.

=item PDA::Pilot::Memo::PackAppInfo(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database AppInfo block.

=item PDA::Pilot::ToDo::Unpack(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given a record from a .pdb file or a Pilot database.

=item PDA::Pilot::ToDo::Pack(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database.

=item PDA::Pilot::ToDo::UnpackAppInfo(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given the AppInfo record from a .pdb file or a Pilot database.

=item PDA::Pilot::ToDo::PackAppInfo(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database AppInfo block.

=item PDA::Pilot::Address::Unpack(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given a record from a .pdb file or a Pilot database.

=item PDA::Pilot::Address::Pack(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database.

=item PDA::Pilot::Address::UnpackAppInfo(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given the AppInfo record from a .pdb file or a Pilot database.

=item PDA::Pilot::Address::PackAppInfo(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database AppInfo block.

=item PDA::Pilot::Mail::Unpack(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given a record from a .pdb file or a Pilot database.

=item PDA::Pilot::Mail::Pack(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database.

=item PDA::Pilot::Mail::UnpackAppInfo(buffer) 

Returns hash reference containing appointment (datebook entry) in a usable
format, given the AppInfo record from a .pdb file or a Pilot database.

=item PDA::Pilot::Mail::PackAppInfo(buffer) 

Given a hash reference in the form that the previous call generates, returns 
a single string suitable for storing in a Pilot's database AppInfo block.

=item PDA::Pilot::Socket::socket(domain, type, protocol)

Same as pi-link routine called pi_socket.

=item PDA::Pilot::Socket::close(socket)

Same as pi-link routine called pi_close.

=item PDA::Pilot::Socket::write(socket, string)

Same as pi-link routine called pi_write.

=item PDA::Pilot::Socket::read(socket, len)

Same as pi-link routine called pi_write (returns read data as result.)

=item PDA::Pilot::Socket::listen(socket, backlog)

Same as pi-link routine called pi_listen.

=item PDA::Pilot::Socket::bind(socket, sockaddr)

Same as pi-link routine called pi_bind. Sockaddr may either be a packed
string containing a pi_sockaddr structure, or a hash reference containing
"device", "family", and "port" keys.

=item PDA::Pilot::Socket::accept(socket)

Same as pi-link routine called pi_accept. If connection is successfull, returns
reference to hash containing remote address, as described above. If failed, returns
undef.

=item PDA::Pilot::DLP::errno()

Returns last DLP error, resetting error to zero.

=item PDA::Pilot::DLP::GetSysDateTime(socket)

Same as DLP call dlp_GetSysDateTime. If successfull, returns time, otherwise
returns undef.

=item PDA::Pilot::DLP::SetSysDateTime(socket, time)

Same as DLP call dlp_SetSysDateTime. time must be a time_t value.

=item PDA::Pilot::DLP::ReadSysInfo(socket)

Same as DLP call dlp_ReadSysInfo. If successfull, returns reference to hash
containing system information.

=item PDA::Pilot::DLP::ReadStorageInfo(socket, cardno)

Same as DLP call dlp_ReadStorageInfo. If successfull, returns reference to hash
containing information on given memory card.

=item PDA::Pilot::DLP::ReadUserInfo(socket)

Same as DLP call dlp_ReadUserInfo. If successfull, returns reference to hash
containing information about user settings.

=item PDA::Pilot::DLP::WriteUserInfo(socket, info)

Same as DLP call dlp_WriteUserInfo. info must be a reference to a hash
containing data similar to that returned by ReadUserInfo (Note: the password
can not be set through this call.)

=item PDA::Pilot::DLP::OpenDB(socket, cardno, mode, name)

Same as DLP call dlp_OpenDB. If successfull returns database handle,
otherwise returns undef.

=item PDA::Pilot::DLP::CloseDB(socket, handle)

Same as DLP call dlp_CloseDB. 

=item PDA::Pilot::DLP::EndOfSync(socket, status)

Same as DLP call dlp_EndOfSync. 

=item PDA::Pilot::DLP::AbortSync(socket)

Same as DLP call dlp_AbortSync. 

=item PDA::Pilot::DLP::MoveCategory(socket, handle, fromcat, tocat)

Same as DLP call dlp_MoveCategory. 

=item PDA::Pilot::DLP::ResetSystem(socket)

Same as DLP call dlp_ResetSystem. 

=item PDA::Pilot::DLP::OpenConduit(socket)

Same as DLP call dlp_OpenConduit. 

=item PDA::Pilot::DLP::AddSyncLogEntry(socket, message)

Same as DLP call dlp_AddSyncLogEntry 

=item PDA::Pilot::DLP::CleanUpDatabase(socket, handle)

Same as DLP call dlp_CleanUpDatabase. 

=item PDA::Pilot::DLP::ResetSyncFlags(socket, handle)

Same as DLP call dlp_ResetSyncFlags. 

=item PDA::Pilot::DLP::ResetDBIndex(socket, handle)

Same as DLP call dlp_ResetDBIndex. 

=item PDA::Pilot::DLP::ResetLastSyncPC(socket)

Same as DLP call dlp_ResetLastSyncPC. 

=item PDA::Pilot::DLP::DeleteCategory(socket, handle, category)

Same as DLP call dlp_DeleteCategory. 

=item PDA::Pilot::DLP::DeleteRecord(socket, handle, all, id)

Same as DLP call dlp_DeleteRecord. 

=item PDA::Pilot::DLP::ReadDBList(socket, cardno, flags, start)

Same as DLP call dlp_ReadDBList. If successfull, returns reference
to hash containing DB information. If failed, returns undef.

=item PDA::Pilot::DLP::FindDBInfo(socket, cardno, flags, name, type, creator)

Same as DLP call dlp_FindDBInfo. If successfull, returns reference
to hash containing DB information. If failed, returns undef.

=item PDA::Pilot::DLP::ReadFeature(socket, creator, number)

Same as DLP call dlp_ReadFeature. If successfull, returns feature value. If
failed, returns undef.

=item PDA::Pilot::DLP::ReadAppBlock(socket, handle)

Same as DLP call dlp_ReadAppBlock. If successfull, returns app block. If
failed, returns undef.

=item PDA::Pilot::DLP::ReadSortBlock(socket, handle)

Same as DLP call dlp_ReadSortBlock. If successfull, returns app block. If
failed, returns undef.

=item PDA::Pilot::DLP::WriteAppBlock(socket, handle, block)

Same as DLP call dlp_WriteAppBlock.

=item PDA::Pilot::DLP::WriteSortBlock(socket, handle, block)

Same as DLP call dlp_WriteSortBlock.

=item PDA::Pilot::DLP::ReadOpenDBInfo(socket, handle)

Same as DLP call dlp_ReadOpenDBInfo.

=item PDA::Pilot::DLP::ReadRecordByIndex(socket, handle, index)

Same as DLP call dlp_ReadRecordByIndex. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, id, index, attr, and category, in that order.

=item PDA::Pilot::DLP::ReadRecordById(socket, handle, id)

Same as DLP call dlp_ReadRecordById. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, id, index, attr, and category, in that order.

=item PDA::Pilot::DLP::ReadNextModifiedRec(socket, handle)

Same as DLP call dlp_ReadNextModifiedRec. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, id, index, attr, and category, in that order.

=item PDA::Pilot::DLP::ReadNextRecInCategory(socket, handle, category)

Same as DLP call dlp_ReadNextRecInCategory. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, id, index, attr, and category, in that order.

=item PDA::Pilot::DLP::ReadNextModifiedRecInCategory(socket, handle, category)

Same as DLP call dlp_ReadNextModifiedRecInCategory. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, id, index, attr, and category, in that order.

=item PDA::Pilot::DLP::WriteRecord(socket, handle, record, id, attr, category)

Same as DLP call dlp_WriteRecord.

=item PDA::Pilot::DLP::ReadResourceByType(socket, handle, type, id)

Same as DLP call dlp_ReadResourceByType. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, type, id, and index, in that order.

=item PDA::Pilot::DLP::ReadResourceByIndex(socket, handle, index)

Same as DLP call dlp_ReadResourceByIndex. If call fails, it returns undef.
Otherwise, in scalar context it returns the read record, in array it returns
the record, type, id, and index, in that order.

=item PDA::Pilot::DLP::WriteResource(socket, handle, record, type, id)

Same as DLP call dlp_WriteResource.

=item PDA::Pilot::DLP::DeleteResource(socket, handle, all, type, id)

Same as DLP call dlp_DeleteResource.

=item PDA::Pilot::DLP::CallApplication(socket, creator, type, action, data)

Same as DLP call dlp_CallApplication.

=item PDA::Pilot::File::open(name)

Same as pi_file_open. Returns a PDA::Pilot::File object on success.

=item PDA::Pilot::File::close(file)

Same as pi_file_close.

=item PDA::Pilot::File::get_app_info(file)

Same as pi_file_get_app_info.

=item PDA::Pilot::File::get_sort_info(file)

Same as pi_file_get_sort_info.

=item PDA::Pilot::File::get_entries(file)

Same as pi_file_get_entries.

=item PDA::Pilot::File::read_resource(file, index)

Same as pi_file_read_resource. Returns (record, type, id, index).

=item PDA::Pilot::File::read_record(file, index)

Same as pi_file_read_record. Returns (record, id, index, attr, category).

=item PDA::Pilot::File::read_record_by_id(file, type, id)

Same as pi_file_read_record_by_id. Returns (record, id, index, attr, category).

=item PDA::Pilot::File::create(name, info)

Same as pi_file_create. Info is reference to hash containg dbinfo data.

=item PDA::Pilot::File::get_info(file)

Same as pi_file_get_info.

=item PDA::Pilot::File::set_info(file, info)

Same as pi_file_set_info.

=item PDA::Pilot::File::set_app_info(file, data)

Same as pi_file_set_app_info.

=item PDA::Pilot::File::set_sort_info(file, data)

Same as pi_file_set_sort_info.

=item PDA::Pilot::File::append_resource(file, data, type, id)

Same as pi_file_append_resource.

=item PDA::Pilot::File::append_record(file, data, attr, category, id)

Same as pi_file_append_record.

=item PDA::Pilot::File::install(file, socket, cardno)

Same as pi_file_install.

=item PDA::Pilot::File::retrieve(file, socket, cardno)

Same as pi_file_retrieve.

=back

=cut
