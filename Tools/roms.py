import zlib
import zipfile
import glob
import os
import struct
import re
import hashlib

class Stats:
  crcs = {}
  microcodes = {}

  def __init__(self):
    self.crcs = {}
    self.microcodes = {}
    
  def addMicrocode(self, microcode, rom_name):
    if not self.microcodes.has_key(microcode):
      self.microcodes[microcode] = []
    self.microcodes[microcode].append(rom_name)
      
  def printMicrocodes(self):
    print '\n\n\n******* Microcodes *************\n\n'
    for microcode in sorted(self.microcodes):
      print microcode
      for rom_name in self.microcodes[microcode]:
        print rom_name
      print '' 
      print ''

    print ''
    print ''
    for microcode in sorted(self.microcodes):
      print microcode

def getCountryNameFromCountryID(country):
  countries = { '7' : "Beta",
                'A' : "NTSC",
                'D' : "Germany",
                'E' : "USA",
                'F' : "France",
                'I' : "Italy",
                'J' : "Japan",
                'P' : "Europe",
                'S' : "Spain",
                'U' : "Australia",
                'X' : "PAL",
                'Y' : "PAL" }
  if country in countries:
    return countries[country]
  return 'blank'

def getIconUrlFromCountryID(country):
  countries = { '7' : "unknown.png",
                'A' : "unknown.png",
                'D' : "de.png",
                'E' : "us.png",
                'F' : "fr.png",
                'I' : "it.png",
                'J' : "jp.png",
                'P' : "europeanunion.png",
                'S' : "es.png",
                'U' : "au.png",
                'X' : "unknown.png",
                'Y' : "unknown.png" }
                
  if country in countries:
    url = countries[country]
  else: url = 'unknown.png'

  return '/images/flags/' + url

def countryFromRomId(rom_id):
  m = re.search('{([a-fA-F0-9]+)-([a-fA-F0-9][a-fA-F0-9])}', rom_id)
  if not m: return '?'
  return chr(int(m.group(2), 16))

class RomDetails:
  data = {}
  rom_id = ''
  rom_name = ''
  cic_type = ''
  sha1_hash = ''
  
  def __init__(self, rom_id):
    self.data = {}
    self.rom_id = rom_id

  def getName(self):
    return self.rom_name
    
  def getSha1Hash(self):
    return self.sha1_hash
    
  def setSha1Hash(self, sha1_hash):
    self.sha1_hash = sha1_hash
    
  def setCicType(self, cic_type):
    self.cic_type = cic_type
    
  def __str__(self):
    return str(self.data)


def addRom(rom_id, attributes):

  if attributes.get('Comment') == 'Unknown':
    del attributes['Comment']

  current_rom = RomDetails(rom_id)
  current_rom.rom_name = attributes.get('Name')
  current_rom.rom_country = countryFromRomId(rom_id)
  current_rom.expansionPakUsage = attributes.get('ExpansionPakUsage')
  current_rom.preview = attributes.get('Preview')
  current_rom.saveType = attributes.get('SaveType')
  return current_rom


def parseRomsIni(lines):
  roms = []
  current_id = ''
  attributes = {}
  
  for line in lines:
    line = line.rstrip()
    if not line:
      continue
    elif line.startswith('{'):
      if current_id:
        roms.append(addRom(current_id, attributes))
      current_id = line
      attributes = {}
    elif current_id:
      vals = line.split('=')
      if len(vals) == 2:
        attributes[vals[0]] = vals[1]
      else:
        print 'invalid line ', line
    
  if current_id:
    roms.append(addRom(current_id, attributes))
  return roms


def parseRomsInifile(filename):
  f = open(filename, 'r')
  roms = parseRomsIni(f.readlines())
  f.close()
  return roms

def writeRomsIni(filename, roms):
  f = open(filename, 'w')
  
  # Sort roms based on Name field
  roms.sort(lambda x, y: cmp(x.getName(),y.getName())) 
    
  for rom in roms:
    f.write(rom.rom_id + '\n')
    f.write('Name=%s\n' % rom.getName())
    for kv in sorted(rom.data):
      f.write(kv + '=' + rom.data[kv] + '\n')
    if rom.cic_type:
      f.write('CicType=%s\n' % rom.cic_type)
    f.write('\n')

  f.close()

def all_files(pattern, search_path, pathsep=os.pathsep):
  for path in search_path.split(pathsep):
    for match in glob.glob(os.path.join(path, pattern)):
      yield match



def isRomHeader(buffer):
  magic = map(ord, buffer[:4])
  return magic in [[0x80, 0x37, 0x12, 0x40],    # 'Correct'
                    [0x40, 0x12, 0x37, 0x80],
                    [0x37, 0x80, 0x40, 0x12],
                    [0x12, 0x40, 0x80, 0x37]]
                    
                    
def fudgeByteswap(buffer):
  r = list(buffer)
  for i in range(0,len(buffer),4):
    r[i+0] = buffer[i+3]
    r[i+1] = buffer[i+2]
    r[i+2] = buffer[i+1]
    r[i+3] = buffer[i+0]
  return ''.join(r)
                
def correctByteswap(buffer):
  magic = map(ord, buffer[:4])
  if magic == [0x80, 0x37, 0x12, 0x40]: 
    return buffer
    
  r = list(buffer)
    
  if magic == [0x40, 0x12, 0x37, 0x80]:
    for i in range(0,len(buffer),4):
      r[i+0] = buffer[i+3]
      r[i+1] = buffer[i+2]
      r[i+2] = buffer[i+1]
      r[i+3] = buffer[i+0]
  elif magic == [0x37, 0x80, 0x40, 0x12]:
    for i in range(0,len(buffer),4):
      r[i+0] = buffer[i+1]
      r[i+1] = buffer[i+0]
      r[i+2] = buffer[i+3]
      r[i+3] = buffer[i+2]
  elif magic == [0x12, 0x40, 0x80, 0x37]:
    for i in range(0,len(buffer),4):
      r[i+0] = buffer[i+2]
      r[i+1] = buffer[i+3]
      r[i+2] = buffer[i+0]
      r[i+3] = buffer[i+1]

  return ''.join(r)

def find_rom(roms, rom_id):
  for rom in roms:
    if rom.rom_id == rom_id:
      return rom
  return None
  
def getCicNameFromCrc(crc):
  name_map = { 0x6170a4a1 : 'CIC-6101',
               0x90bb6cb5 : 'CIC-6102',
               0x0b050ee0 : 'CIC-6103',
               0x009e9ea3 : 'CIC-6104',
               0x98bc2c86 : 'CIC-6105',
               0xacc8580a : 'CIC-6106' } 
  if crc in name_map:
    return name_map[crc]
  return '?'

def findMicrocodes(buffer):
  codes = {}
  idx = buffer.find('RSP')
  while idx != -1:
    has_space = False
    for i in range(idx, idx+200):
      c = ord(buffer[i])
      if c > 128 or c == ord('=') or c == ord('@'):
        break
      if c < 32:
        microcode = buffer[idx:i]
        if len(microcode) > 8 and has_space:
          if not codes.has_key(microcode):
            codes[microcode] = 1
        break
      elif c == ord(' '):
        has_space = True
    idx = buffer.find('RSP', idx+1)
  return codes.keys()

def find_roms_in_zipfile(zip_filename, roms, stats):
  z = zipfile.ZipFile(zip_filename, 'r')
  
  for filename in z.namelist():
    f = z.open(filename, 'r')
    if not f: continue

    buffer = f.read()
    processBuffer(filename, buffer)
  
def processFile(filename, roms, stats):
		buffer = open(filename,'rb').read()
		processBuffer(filename, buffer)
    
def processBuffer(filename, buffer):

    if len(buffer) < header_size+bootcode_size: return
    if not isRomHeader(buffer): return
    
    
    len_mod = len(buffer) % 4
    if len_mod > 0:
      print '%s / %s not multiple of 4 - %d' % (zip_filename, filename, len(buffer))
      padding = chr(0) * (4 - len_mod)
      buffer = buffer + padding
      return
    
    buffer = correctByteswap(buffer)
    
    header_buf = buffer[:header_size]
    bootcode_buf = buffer[header_size:game_offset]
    
    fields = list(struct.unpack(header_string, header_buf))
       
    #crc = long(zlib.crc32(bootcode_buf) & 0xffffffff)
    #new_count = 1
    #if stats.crcs.has_key(crc): new_count = stats.crcs[crc] + 1
    #stats.crcs[crc] = new_count
        
    rom_name = fields[11].rstrip()
    rom_id = '{%08x%08x-%02x}' % (fields[7], fields[8], fields[17])
    
    rom = find_rom(roms, rom_id)
    if rom:
      #rom.setCicType(getCicNameFromCrc(crc))
      #if not rom.getSha1Hash():
      #  sha1_hash = hashlib.sha1(buffer).hexdigest()
      #  print 'sha1 - %s' % sha1_hash
      #  rom.setSha1Hash(sha1_hash)
      print '%s - "%s"' % (filename, rom.getName())

      for s in findMicrocodes(buffer): 
        stats.addMicrocode(s, rom.getName())
        print '  ' + s
    else:
      print '%s - %s - %s' % (filename, rom_name, rom_id)
    
  
#struct ROMHeader
#{
#	u8		x1, x2, x3, x4;   0
#	u32		ClockRate;        4
#	u32		BootAddress;      5
#	u32		Release;          6
#	u32		CRC1;             7
#	u32		CRC2;             8
#	u32		Unknown0;         9
#	u32		Unknown1;         10
#	s8		Name[20];         11
#	u32		Unknown2;         12
#	u16		Unknown3;         13
#	u8		Unknown4;         14
#	u8		Manufacturer;     15
#	u16		CartID;           16
#	s8		CountryID;        17
#	u8		Unknown5;         18
#};
header_string = 'BBBBLLLLLLL20sLHBBHbB'
header_size = struct.calcsize(header_string)
game_offset = 0x1000
bootcode_size = 0x1000 - 0x40

    
stats = Stats()
roms = parseRomsInifile('roms.ini')

count = 0
for match in all_files('*.z64', r'm:\rom_dump'):
  #print match
  #find_roms_in_zipfile(match, roms, stats)
  processFile(match, roms, stats)
  count = count + 1
  #if count >= 30: break
stats.printMicrocodes()
#for crc in sorted(crcs): print 'CRC %s -> %s' % (getCicNameFromCrc(crc), crcs[crc])

writeRomsIni('roms2.ini', roms)
