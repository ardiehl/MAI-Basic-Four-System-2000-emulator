uses sysutils,
     classes,
     tdefs;

// AD 03 March 2021: directories seem to have the flag $01008A00

(*$I-*)

const //fn='S????*';
      fn='F????*';
      outPrefix = 'out/';

type

  barray = array [0..$FFFFFF] of byte;

var sr : TSearchRec;
  err : longint;
  listMode : boolean = false;
  fileList : TStringList;
  i : integer;

procedure processFile(fn : string);
var p : ^barray;
    f : file;
    s,fname,fdir : string;
    fm:integer;
    fsize:longint;
    po : integer;
begin
  fm := filemode; filemode:=0;  //readonly
  assign(f,fn); reset(f,1); fsize := fileSize(f);
  getmem(p,fsize); blockread(f,p^,fsize); close(f);
  filemode := fm;
  if ioresult <> 0 then
   begin
     writeln('unable to read ',fn); exit;
   end;
  if listMode then
    with psavePrefix(p)^ do
      begin
        write('L01: ',IntToHex(unknownL01,8),
              ' L02: ',IntToHex(unknownL02,8),
              ' L03: ',IntToHex(unknownL03,8),
              ' FileType: ',IntToHex(FileType,8),
              ' L11: ',IntToHex(unknownL11,8));
      end;
  s := psavePrefix(p)^.xtype;
  write(' "',s,'" ');
  if (s = '**FILE**') then
  begin
    //fname := ExtractFileName(strpas(@psavePrefix(p)^.fileName));
    fname := strpas(@psavePrefix(p)^.fileName);
    fdir := outPrefix+ExtractFileDir(fname);
    po := pos ('//',fdir);
    while (po > 0) do
    begin
        delete(fdir,po,1);
        po := pos ('//',fdir);
    end;
    writeln('"',fname,'" size: ',longSwap(psavePrefix(p)^.FileSize),' (fdir: "',fdir,'" ');
    if listMode then
      exit;
    if psavePrefix(p)^.FileType = $01008A00 then
    begin
      forceDirectories(fdir+fname);
    end else
    begin
      forceDirectories(fdir);
      assign(f,outPrefix+fname); rewrite(f,1);
      if ioresult <> 0 then
       begin
         writeln('unable to create ',outPrefix+fname); exit;
       end;
      blockWrite(f,p^[sizeof(tsavePrefix)],fsize-sizeof(tsavePrefix));
      close(f);
      if ioresult <> 0 then
       begin
         writeln('unable to write ',outPrefix+fname); exit;
       end;
     end;
  end else writeln;
  freeMem(p,fsize);
end;

var dir,fname : string;

begin
  checkStructs;
  dir :=paramStr(1);
  if dir = '-l' then
    begin
      listMode := true;
      dir := paramStr(2);
    end;
  if dir <> '' then
    dir := IncludeTrailingBackslash(dir);
  writeln ('src: ',dir+fn);
  fileList := TStringList.create;

  if findFirst(dir+fn,faAnyFile,sr) = 0 then
  begin
    repeat
      //write (sr.name,' ');
      fileList.add(dir+sr.name);
      err := findNext(sr);
    until err <> 0;
    findClose(sr);
    fileList.sort;
    for i := 0 to fileList.count-1 do
      begin
        fname := fileList[i];
        write(extractFileName(fname),': ');
        processFile(fname);
      end;
  end;
end.
