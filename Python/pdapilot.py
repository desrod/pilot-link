import _pdapilot
from _pdapilot import *

class Block:
	def __init__(self, contents=None, dlpdb=None):
		if contents != None:
			self.unpack(contents, dlpdb)
		else:
			self.fill()

	def __repr__(self):
		return '<' + self.__class__.__name__ +' instance, '+ repr(self.__dict__) + '>'

	def pack(self, dlpdb=None):
		return self.raw

	def unpack(self, block, dlpdb=None):
		self.raw = block

	def fill(self):
		0

class PrefBlock (Block):
	def __init__(self, contents=None, dlp=None, creator=None, id=None, version=None, backup=None):
		if creator != None:
			self.creator = creator
		if id != None:
			self.id = id
		if version != None:
			self.version = version
		if backup != None:
			self.backup = backup
		if contents != None:
			self.unpack(contents, dlp)
		else:
			self.fill()

class ResourceBlock (Block):
	def __init__(self, contents=None, dlpdb=None, type=None, id=None):
		if type != None:
			self.type = type
		if id != None:
			self.id = id
		if contents != None:
			self.unpack(contents, dlpdb)
		else:
			self.fill()

class RecordBlock (Block):
	def __init__(self, contents=None, dlpdb=None, index=None, id=None, attr=None, category=None):
		if index != None:
			self.index = index
		if id != None:
			self.id = id
		if attr != None:
			if attr & 0x80:
				self.deleted = 1
			else:
				self.deleted = 0
			if attr & 0x40:
				self.modified = 1
			else:
				self.mofified = 0
			if attr & 0x20:
				self.busy = 1
			else:
				self.busy = 0
			if attr & 0x10:
				self.secret = 1
			else:
				self.secret = 0
			if attr & 0x08:
				self.archived = 1
			else:
				self.archived = 0
		if category != None:
			self.category = category
		if (contents != None):
		    if not self.deleted:
				self.unpack(contents, dlpdb)
		else:
			self.fill()
	def fill(self):
		self.deleted = 0
		self.modified = 0
		self.busy = 0
		self.secret = 0
		self.archived = 0

class AppBlock (Block):
	""" AppBlock is intended to keep any methods common to most app blocks
	such as category manipulation routines, etc.
	"""

class RecordDatabase:
	Record = RecordBlock
	AppBlock = Block
	SortBlock = Block
	Pref = PrefBlock

class ResourceDatabase:
	Resource = ResourceBlock
	AppBlock = Block
	SortBlock = Block
	Pref = PrefBlock

class Database:
	Resource = ResourceBlock
	Record = RecordBlock
	AppBlock = Block
	SortBlock = Block
	Pref = PrefBlock

class Memo (RecordDatabase):
	creator = 'memo'
	name = 'MemoDB'
	class Record (RecordBlock):
		def fill(self):
			self.text = ''
			RecordBlock.fill(self)
		
		def pack(self, dlpdb=None):
			self.raw = _pdapilot.MemoPack(self.__dict__)
			return self.raw
		
		def unpack(self, block, dlpdb=None):
			self.raw = block
			_pdapilot.MemoUnpack(self.__dict__, block)
			
	class AppBlock (AppBlock):
		def pack(self, dlpdb=None):
			self.raw = _pdapilot.MemoPackAppBlock(self.__dict__)
			return self.raw
		
		def unpack(self, block, dlpdb=None):
			self.raw = block
			_pdapilot.MemoUnpackAppBlock(self.__dict__, block)

class ToDo (RecordDatabase):
	creator = 'todo'
	name = 'ToDoDB'
	class Record (RecordBlock):
		def fill(self):
			self.indefinite = 0
			self.priority = 0
			self.complete = 0
			self.note = None
			self.description = None
			self.due = None
			RecordBlock.fill(self)
		
		def pack(self, dlpdb=None):
			self.raw = _pdapilot.ToDoPack(self.__dict__)
			return self.raw
		
		def unpack(self, block, dlpdb=None):
			self.raw = block
			_pdapilot.ToDoUnpack(self.__dict__, block)
			
	class AppBlock (AppBlock):
		def pack(self, dlpdb=None):
			self.raw = _pdapilot.ToDoPackAppBlock(self.__dict__)
			return self.raw
		
		def unpack(self, block, dlpdb=None):
			self.raw = block
			_pdapilot.ToDoUnpackAppBlock(self.__dict__, block)

class Datebook (RecordDatabase):
	creator = 'date'
	name = 'DatebookDB'
	class Record (RecordBlock):
		def fill(self):
			self.begin = self.end = None
			self.alarm = None
			self.repeat = None;
			self.repeatEnd = None;
			self.repeatWeekstart = 0;
			self.exceptions = None
			self.description = None
			self.note = None
			RecordBlock.fill(self)
		
		def pack(self, dlpdb=None):
			self.raw = _pdapilot.DatebookPack(self.__dict__)
			return self.raw
		
		def unpack(self, block, dlpdb=None):
			self.raw = block
			_pdapilot.DatebookUnpack(self.__dict__, block)
			
	class AppBlock (AppBlock):
		def pack(self, dlpdb=None):
			self.raw = _pdapilot.DatebookPackAppBlock(self.__dict__)
			return self.raw
		
		def unpack(self, block, dlpdb=None):
			self.raw = block
			_pdapilot.DatebookUnpackAppBlock(self.__dict__, block)


class Mail (RecordDatabase):
	creator = 'mail'
	name = 'MailDB'
	class Pref (PrefBlock):
		def fill(self):
			if (self.id == 1) or (self.id == 2):
				self.syncType = None;
				self.getHigh = None;
				self.getContaining = None;
				self.truncate = None;
				self.filterTo = None;
				self.filterFrom = None;
				self.filterSubject = None;
			elif self.id == 3:
				self.signature = None;
			
		def pack(self, dlp=None):
			self.raw = _pdapilot.MailPackPref(self.__dict__, self.id)
			return self.raw
		
		def unpack(self, block, dlp=None):
			self.raw = block
			_pdapilot.MailUnpackPref(self.__dict__, block, self.id)

DBClasses[''] = Database
PrefClasses[''] = PrefBlock

DBClasses[Memo.name] = Memo
DBClasses[ToDo.name] = ToDo
DBClasses[Datebook.name] = Datebook
DBClasses[Mail.name] = Mail
PrefClasses[Mail.creator] = {'': Mail.Pref}


