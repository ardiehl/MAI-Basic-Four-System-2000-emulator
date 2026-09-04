{Change diskparams in mai 2000 dutil (from diagnostic tape)
 Armin Diehl <diehl@nordrhein.de> 5 Dec 2005
 Compiler: FreePascal
}
{$MODE objfpc}
var f : file;

type
 THdr =
   packed record
     hdr1 : array [0..3] of char;
     unknown : array [0..7] of byte;
     name     : array [0..35] of char;
     unknown1 : array [1..29*16] of byte;
   end;
 LineRec = record
             addr : longint;
             bytes: byte;
           end;
 TDiskNames = array [0..8,0..15] of char;
 PDiskNames = ^TDiskNames;
 TDiskTypeEntry = packed record
                    Cylinders : word;
                    rwc       : word;
                    wpc       : word;
                    unknown1  : byte;
                    unknown2  : byte;
                    heads     : byte;
                    unknown3  : byte;
                  end;
 TDiskEntries = array [0..8] of TDiskTypeEntry;
 PDiskEntries = ^TDiskEntries;
const maxLines = 1000;
      DiskNamesOfs = $933a;
      DiskEntriesOfs = $6b4;
{these offsets are for the menu of disk types, the menu
 is text only and not linked to the names in PDiskNames}
const
  DiskNamesMenuOfs : array [0..7] of longint = ($aba9,$abc5,$abde,$abff,$ac22,$ac45,$ac5e,$ac84);
  DiskNamesMenuLen : array [0..7] of byte    = (   22,   19,   27,   29,   29,   19,   32,   28);
var
 Header : THdr;
 Buf    : array [0..65535] of byte;
 BufEnd : word;
 StartAddr : longint;
 Date : array [0..$11] of char;
 Version : array [0..7] of char;
 lastLine : longint = 0;
 Lines : array [1..maxLines] of LineRec;  // to save addresses
 RestLen : longint;
 Rest : pointer;
 DiskNames : PDiskNames;
 DiskEntries : PDiskEntries;


{41 to 42    Contain a checksum for the record. To calculate this, take the
             sum of the values of all bytes from the byte count up to the
             last data byte, inclusive, modulo 256. Subtract this result
             from 255.

checksum  A char[2] field.  These characters when paired and
		    interpreted as a hexadecimal value display the
		    least significant byte of the ones complement of
		    the sum of the byte values represented by the
		    pairs of characters making up the count, the
                    address, and the data fields.
}

function swapWord (w : word) : word;
begin
  swapWord := hi(w)+(lo(w) shl 8);
end;

procedure readDutil (var f : file);
var
    c : char;
    cmd : char;
    len,i,lastlen : byte;
    b : byte;
    a1,a2,a3,chkF,chk : byte;
    byteSum : longint;
    lastaddr,addr  : longint;
    p : pchar;
    st : string [2];

  procedure skip (bytes : longint);
  begin
    seek (f,filepos(f)+bytes);
  end;

  function nextCh : char;
  var c : char;
  begin
    blockread(f,c,1);
    nextCh := C;
  end;

  function hexNib2b (c : char) : byte;
  begin
    if (c >= '0') and (c <= '9') then
      result := byte(c) - byte('0')
    else
    if (c >= 'A') and (c <= 'F') then
      result := byte(c) - byte('A') + 10
    else begin
      writeln ('Illegal Hex char ',c);
      halt;
    end;
  end;

  function getbyte : byte;
  begin
    result := (hexNib2b (nextCh) shl 4) + hexNib2b (nextCh);
  end;

begin
  blockread(f,header,sizeof(header));
  startaddr := -1;
  bufEnd := 0;
  c := nextch;
  while C = 'S' do
  begin
    cmd := nextCh;
    len := getbyte;

    case cmd of
      '2' : begin        // bin data
              byteSum := len;
              a1 := getbyte; byteSum := byteSum + a1;
              a2 := getbyte; byteSum := byteSum + a2;
              a3 := getbyte; byteSum := byteSum + a3; // address
              addr := (a1 shl 16) + (a2 shl 8) + a3;
              if startaddr = -1 then startaddr := addr;
              inc(lastline);
              lines[lastline].addr := addr;
              lines[lastline].bytes:= len -4;
              lastlen := len;
              for i := 4 to len-1 do
              begin
                b := getbyte;
                byteSum := byteSum + b;
                buf[bufEnd] := b;
                inc(bufEnd);
              end;
              chkF := getbyte;
              chk := 255 - (byteSum mod 256);
              if chk <> chkF then
                writeln ('WARNING: Checksum Error');
              //writeln ('a1,2,3: ',hexStr(a1,2)+' '+hexStr(Addr,6)+' '+hexStr(a2,2)+' '+hexStr(a3,2)+' len: 0x'+hexStr(Len-1-3,2)+
              //          '  chk: 0x',hexStr(chk,2),' chkF: 0x',hexStr(chkF,2));
            end;
      '4' : begin  // date
              for i := 0 to len-1 do
                date[i] := nextch;
            end;
      '5' : begin
              // version
              for i := 0 to len-1 do
                version[i] := nextch;
            end;
      '8' : begin // eof ??, copy remaining 1:1 to destination file, dont know what it is, may be garbage
              RestLen := FileSize(f)-FilePos(f)+4;
              getMem (Rest,RestLen);
              p := Rest;
              p^ := 'S'; inc(p);
              p^ := '8'; inc(p);
              st := HexStr(len,2);
              p^ := st[1]; inc(p);
              p^ := st[2]; inc(p);

              blockRead(f,p^,RestLen-4);
              exit;
            end else
              writeln ('Unknown Record S'+c);
    end;
    if eof(f) then
      exit;
    c := nextCh;
  end;
end;

procedure writeDutil (fn : string);
var f : file;
    i,j : longint;
    s : ansistring;
    bufPos : longint;
    byteSum : longint;
    chk : byte;
    s1 : string [50];
begin
  assign (f,fn); rewrite (f,1);
  blockwrite (f,header,sizeof(header));
  s1 := 'S412'+Date+'S508'+Version;
  BlockWrite (f,s1[1],length(s1));
  bufPos := 0;
  for i := 1 to lastline do
  begin
    byteSum := (lines[i].addr and $ff) +
               ((lines[i].addr and $ff00) shr 8) +
               ((lines[i].addr and $ff0000) shr 16) + lines[i].bytes + 4;
    s := 'S2'+HexStr(Lines[i].bytes+4,2)+HexStr(Lines[i].Addr,6);
    for j := 1 to lines[i].bytes do
    begin
      byteSum := byteSum + Buf[bufPos];
      s := s + HexStr(Buf[bufPos],2);
      inc(bufPos);
    end;
    chk := 255 - (byteSum mod 256);
    s := s + HexStr(chk,2);
    BlockWrite(f,s[1],length(s));
  end;
  BlockWrite (f,Rest^,RestLen);
  close(f);

end;

procedure edit;
var c : char;
    n : byte;
    name : string[50];
    w : word;

  procedure show;
  var i : byte;
  begin
    for i := 0 to 8 do
    begin
      write (i,': ',DiskNames^[i]);
      write (' Cyl:',SwapWord(DiskEntries^[i].Cylinders):5);
      write (' rwc:',SwapWord(DiskEntries^[i].rwc):5);
      write (' wpc:',SwapWord(DiskEntries^[i].wpc):5);
      write (' ?1:0x',HexStr(DiskEntries^[i].unknown1,2));
      write (' ?2:0x',HexStr(DiskEntries^[i].unknown2,2));
      write (' hds:',DiskEntries^[i].heads:2);
      write (' ?3:0x',HexStr(DiskEntries^[i].unknown3,2));
      writeln;
    end;
    writeln;
  end;

begin
  DiskNames := @Buf[DiskNamesOfs];
  DiskEntries := @Buf[DiskEntriesOfs];
  repeat
    show;
    write ('Enter Number to change or (S)ave, (Q)uit: '); readln (c);
    c := upcase(c);
    if c = 'Q' then halt(1);
    if (c >= '0') and (C <= '7') then
    begin
      n := byte(c)-byte('0');
      byte(name[0]) := DiskNamesMenuLen[n];
      move(buf[DiskNamesMenuOfs[n]],name[1],length(name));
      writeln ('Menu Entry: "',name,'"');

      write ('name (',DiskNames^[n],'): ');
      ioresult;
      readln(name);
      if name <> '' then
      begin
        fillchar(DiskNames^[n],15,' ');
        if length(name)>15 then name := copy(name,1,15);
        move(name[1],DiskNames^[n],length(name));

        {patch menu used for disk type selection}
        while length(name) < DiskNamesMenuLen[n] do
          name := name + ' ';
        move (name[1],buf[DiskNamesMenuOfs[n]],length(name));
      end;

      write ('Cylinders (Enter 0 for ',SwapWord(DiskEntries^[n].Cylinders),'): ');
      readln (w);
      if w > 0 then
        DiskEntries^[n].Cylinders := SwapWord(w);

      write ('rwc (Enter 0 for ',SwapWord(DiskEntries^[n].rwc),'): ');
      readln (w);
      if w > 0 then
        DiskEntries^[n].rwc := SwapWord(w);

      write ('wpc (Enter 0 for ',SwapWord(DiskEntries^[n].wpc),'): ');
      readln (w);
      if w > 0 then
        DiskEntries^[n].wpc := SwapWord(w);

      write ('Heads (Enter 0 for ',DiskEntries^[n].Heads,'): ');
      readln (w);
      if w > 0 then
        DiskEntries^[n].Heads := w;

      writeln;
    end;
  until c = 'S';
end;


var fn : string;
begin
  fn := paramstr(1);
  if fn = '' then fn := 'dutil.2000';
  write ('Reading ',fn);
  assign (f,fn);
  reset (f,1);
  readDutil(f);
  writeln;
  writeln ('Dutil Date: "',Date,'"');
  Writeln ('Dutil Version: "',Version,'"');
  writeln ('-------------------------------------------------------------------------------');
  close(f);
  edit;
  if (pos('.new',fn) = 0) then fn := fn + '.new';
  write ('Saving to ',fn);
  writeDutil (fn);
  writeln;
end.

