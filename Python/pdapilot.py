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
			self.attr= attr
		if category != None:
			self.category = category
		if contents != None:
			self.unpack(contents, dlpdb)
		else:
			self.fill()

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
DBClasses[Mail.name] = Mail
PrefClasses[Mail.creator] = {'': Mail.Pref}


