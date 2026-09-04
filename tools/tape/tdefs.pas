unit tdefs;

{$mode objfpc}{$H+}

// definitions for M/A/I/ basic four tsave tape format
// A.Diehl 11/2011

interface

type
tHeaderDate = packed Record
  day,month,year : array[0..1] of char;
end;
pHeaderDate = ^tHeaderDate;
tHeaderTime = packed Record
  hour,minute : array[0..1] of char;
end;
pHeaderTime = ^tHeaderTime;

tTapeHeader = packed record
  volBlank   : array [0..3] of char;  // "VOL "
  volType    : array [0..1] of char;  // Boottape: "33", backup tape: "30"
  tapeSetName: array [0..7] of char;
  tapeId     : array [0..7] of char;
  unknown01  : array [0..1] of char;  // '01' on all tapes i have, may be 01=BOSS/IX
  MachineSSN : array [0..11] of char;  // '300081000' or '200097894'
  createDate : tHeaderDate;
  writeDate  : tHeaderDate;
  writeTime  : tHeaderTime;
  zero       : byte;
  unknown02  : array [0..18] of char;
  tapeSerial : array [0..7] of char;
  unknown    : array [0..431] of byte;  // 1126051126052337
end;
pTapeHeader = ^tTapeHeader;
tsavePrefix=packed record
  unknownL01: longint;
  unknownL02: longint;
  unknownL03: longint;
  filetype  : longint;  // /sys/bass,/util/fl: 00 8A 00 01, /util/fl/EBS, /util/fl/EBA: 00 82 00 01
  //unknown02: array[1..16] of byte;
  unknownL11 : longint;  // 7F FF FF FF for /bin/fichk,csave.cpl
  FileSize   : longint;  // 00 00 12 4C = 4684 for /bin/fichk
                         // 00 02 11 A4 = 135588 for /bin/csave.cpl
  unknownL13 : longint;
  unknownL14 : longint;
  unknown03: array[1..16] of byte;
  unknown04: array[1..16] of byte;
  unknown05: array[1..16] of byte;
  unknown2 : array[1..12] of byte;
  fileName : array[1..4+(7*16)+14] of char;
  xtype    : array [0..20] of char;  // **FILE** or **SAVESET** \0
  unknown3 : array [1..269] of byte;
end;
psavePrefix = ^tsavePrefix;

const
  ft_unknown    = 0;
  ft_saveset    = 1;
  ft_filesystem = 2;
  ft_file       = 3;
  ft_keyfile    = 4;

  ft_directory  = $01008A00;


function ts_getFileType(dataPtr : pchar) : integer;
procedure checkStructs;
function longSwap (l:longint):longint;

implementation

const
  tsFileTypes : array [1..4] of string =
  ('**SAVESET**',
   '**FILESYSTEM**',
   '**FILE**',
   '**KEYFILE**');

function ts_getFileType(dataPtr : pchar) : integer;
var
  i : integer;
begin
  result := 0;
  if dataPtr = nil then exit;
  for i := low(tsFileTypes) to high(tsFileTypes) do
    if tsFileTypes[i] = psavePrefix(dataPtr)^.xtype then
      begin
        result := i;
        exit;
      end;
end;

function longSwap (l:longint):longint;
var l2 : packed array [0..3] of byte absolute l; // 00 02 11 A4 = 135588 for /bin/csave.cpl
begin
  result := l2[3] + (l2[2] shl 8) + (l2[1] shl 16) + (l2[0] shl 24);
end;

procedure checkStructs;
begin
  if sizeof(tTapeHeader) <> 512 then
    begin
      writeln ('sizeof(tTapeHeader) is ',sizeof(TTapeHeader),', instead of 512, diff: ',512-sizeof(TTapeHeader));
      halt(1);
    end;
  if sizeof(tSavePrefix) <> 512 then
    begin
      writeln ('sizeof(tSavePrefix) is ',sizeof(tSavePrefix),', instead of 512, diff: ',512-sizeof(tSavePrefix));
      halt(1);
    end;
end;

end.

