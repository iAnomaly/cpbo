/*
cpbo Copyright (C) 2006-2014 Keijo Ruotsalainen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License, version 2.1 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA  02110-1301  USA
*/

#include <stdint.h>
#include <time.h>
typedef int64_t filetime_t;

// PBO functionality
#include "pbo.h"
#include "sha1.h"


#ifndef _NIX
#define WITH_GUI
#else
typedef unsigned long DWORD;
typedef void *PVOID;
typedef PVOID HANDLE;
#endif

#ifdef WITH_GUI
#include <windows.h>
#include "resource.h"	
void hideWindow(bool state);
#endif

#ifdef WITH_GUI

#define WIN_REPOS(x,l,r,t,b) {\
  WINDOWPLACEMENT wp;GetWindowPlacement(x, &wp);\
  wp.rcNormalPosition.left += l;\
  wp.rcNormalPosition.right += r;\
  wp.rcNormalPosition.top += t;\
  wp.rcNormalPosition.bottom += b;\
  SetWindowPlacement(x, &wp);\
  InvalidateRect(x, NULL, TRUE);\
}

FTENTRY *ftg = NULL;
int ftgsize = 0;
char *doutdir = NULL;
char *dinfile = NULL;
INT_PTR CALLBACK dialogproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static int lw = 0;
  static int lh = 0;
  HWND lb = GetDlgItem(hwndDlg, IDC_FILELIST);
  HWND bok = GetDlgItem(hwndDlg, IDOK);
  HWND bcancel = GetDlgItem(hwndDlg, IDCANCEL);
  HWND frme = GetDlgItem(hwndDlg, IDC_BORDE);
  HWND frme2 = GetDlgItem(hwndDlg, IDC_BORDE2);
  HWND pathw = GetDlgItem(hwndDlg, IDC_PATH);
  HWND filtr = GetDlgItem(hwndDlg, IDC_FILTER);

  
  if(!lw || !lh) {
    RECT r;
    GetClientRect(hwndDlg, &r);
    lw = r.right - r.left;
    lh = r.bottom - r.top;
  }

  switch(uMsg) {
    case WM_SIZE: {
      int fwSizeType = wParam;  
      int w = LOWORD(lParam);
      int h = HIWORD(lParam); 
      if(fwSizeType != SIZE_RESTORED)
        break;
      
      int dx = w-lw;
      int dy = h-lh;

      WIN_REPOS(lb, 0, dx, 0, dy);
      WIN_REPOS(frme, 0, dx, dy, dy);
      WIN_REPOS(frme2, 0, dx, dy, dy);
      WIN_REPOS(pathw, 0, dx, dy, dy);
      WIN_REPOS(bok, 0, 0, dy, dy);
      WIN_REPOS(bcancel, dx, dx, dy, dy);
      WIN_REPOS(filtr, 0, dx, dy, dy);

      lw = w;
      lh = h;
      break;
    }

    case WM_INITDIALOG: {
      char filter[256];
      int c = 0;
      GetDlgItemText(hwndDlg, IDC_FILTER, filter, 256);
      SendMessage(lb, LB_RESETCONTENT, 0, 0); 
      for(int i=0;i<ftgsize;i++) {
        ftg[i].extract = false;
        if(strstr(ftg[i].fname, filter)) {
          SendMessage(lb, LB_ADDSTRING, 0, (LPARAM) ftg[i].fname);
          SendMessage(lb, LB_SETITEMDATA, c++, (LPARAM) i);
        }
      }
      SetDlgItemText(hwndDlg, IDC_PATH, doutdir);

      char title[512];
      sprintf(title, "cpbo: %s", dinfile);
      SetWindowText(hwndDlg, title);
      break;
    }

    case WM_COMMAND: {
      int wNotifyCode = HIWORD(wParam); // notification code 
      int wID = LOWORD(wParam);         // item, control, or accelerator identifier 
      HWND hwndCtl = (HWND) lParam;      // handle of control 

      switch(LOWORD(wParam)) {
        case IDOK: {
          int numsel = SendMessage(lb, LB_GETSELCOUNT, 0, 0);
          if(numsel == 0) {
            MessageBox(hwndDlg, "No files selected", "Cannot extract", MB_ICONWARNING);
            return FALSE;
          }
          int *sel = new int[numsel];
          SendMessage(lb, LB_GETSELITEMS, numsel, (LPARAM) sel);
          for(int i=0;i<numsel;i++) {
            DWORD id = SendMessage(lb, LB_GETITEMDATA, sel[i], 0);
            printf("EXTRACT: %d\n", id);
            ftg[id].extract = true;
            //ftg[sel[i]].extract = true;
          }
          printf("Selected %d files\n", numsel);
          GetDlgItemText(hwndDlg, IDC_PATH, doutdir, 256);
          EndDialog(hwndDlg, IDOK);
          break;
        }

        case IDCANCEL:
          EndDialog(hwndDlg, IDCANCEL);
          break;

        case IDC_FILTER:
          if(wNotifyCode == EN_CHANGE)
            SendMessage(hwndDlg, WM_INITDIALOG, NULL, NULL);
          
          break;
      }
      break;
    }
  }
  return FALSE;
}
#endif

// From the internets
filetime_t UnixTimeToFileTime(time_t t) {
     filetime_t f;
     f = ((int64_t)t * 10000000) + 116444736000000000LL;
     return f;
}
time_t FileTimeToUnixTime(filetime_t f) {
  time_t t;
  t = (time_t) ((f - 116444736000000000LL) / 10000000);
  return t;
}


// Does file exist?
bool fileExists(char *filename) {
  FILE *f = fopen(filename, "rb");
  if(f) {
    fclose(f);
    return true;
  }
  return false;
}

// Read null terminated string from file
bool fgetsz(void *d, int maxlen, FILE *f) {
  int r = 0;
  char *dd = (char*) d;
  char c;
  do {
    c = fgetc(f);
    *dd++ = c;
    ++r;
  } while(c != 0x00 && c != EOF && r<maxlen);
  if(ferror(f))
    return false;
  return true;
}

// Create all subdirs in filename
void createDirs(char *fname) {
  path p(fname);
  try {
    create_directories(p.parent_path());
  }

  catch (const filesystem_error& ex) {
    cout << ex.what() << '\n';
  }
}

// Extract a PBO, sf = source filename, dd = target directory
bool pboEx(char *sf, char *dd, bool overwrite, bool gui) {
  FILE *i = fopen(sf, "rb");
  if(!i) 
    return false;	

  int status = STATUS_HEADER;
  int ftableptr = 0;
  int numFiles = 0;
  char str[FNAMELEN];

  FTENTRY *ft = NULL;
  int fi = 0;

  char prefix[FNAMELEN];
  char product[FNAMELEN];
  memset(prefix, 0, FNAMELEN);
  memset(product, 0, FNAMELEN);

  bool someFailed = false;

  fgetsz(str, FNAMELEN, i);

  if(strlen(str) != 0) {
    // No header block - just file table
    status = STATUS_FILECOUNT;
    ftableptr = 0;
    fseek(i, 0, SEEK_SET);
  }
  //printf("str: <%s> %d\n", str, strlen(str));

  // Read
  while(status != STATUS_DONE) {
    fgetsz(str, FNAMELEN, i);
    //printf("str: <%s>\n", str);
    switch(status) {
      case STATUS_HEADER:
        // Reading the first header blocks...
        if(strlen(str) != 0) {
          char *foo = str;

          if(!strcmp(foo, "sreV")) {
            // "Vers" block, nothing useful here...
            fseek(i, 15, SEEK_CUR);
          } else if(!strcmp(foo, "product")) {
            // "ProducT" block, nothing we need here either						
            fgetsz(product, FNAMELEN, i);
          } else if(!strcmp(foo, "prefix")) {
            // PBO filename prefix
            fgetsz(prefix, FNAMELEN, i);
          } else if(!strcmp(foo, "version")) {
            // PBO version?
            char version[FNAMELEN];
            fgetsz(version, FNAMELEN, i);
          } else if(!strcmp(foo, "svn")) {
            // SVN for vbs2 pbos
            char joop[FNAMELEN];
            fgetsz(joop, FNAMELEN, i);
          } else if(strlen(foo) != 0) {
            // Unknown block
            //printf("pboEx: Unknown block in header! aborting <%s>\n", foo);
            printf("pboEx: Warning: Unknown block in header: <%s>\n", foo);
            //return false;
            char joop[FNAMELEN];
            fgetsz(joop, FNAMELEN, i);
          }

          ftableptr = ftell(i)+1;
          break;
        }
        // str isn't null, we reached end of headers
        status = STATUS_FILECOUNT;
        break;

      case STATUS_FILECOUNT:
        // Count number of files
        if(strlen(str) != 0) {
          PBOENTRY e;
          fread(&e, sizeof(e), 1, i);
          ++numFiles;
        } else {
          // End of file table, seek back to beginning of table
          fseek(i, ftableptr, SEEK_SET);
          printf("Found %d files\n", numFiles);
          status = STATUS_FILETABLE;

          // Allocate memory for filetable
          ft = new FTENTRY[numFiles];
        }
        break;

      case STATUS_FILETABLE:
        if(strlen(str) != 0) {
          // Read file table block, filename is already read
          PBOENTRY e;
          fread(&e, sizeof(e), 1, i);

          //printf("File: %s, %d KB\n", str, e.DataSize/1024);
          /*if(e.PackingMethod == 0x43707273) {
            // Cant handle these (yet?)
            
            printf("%s:\n", str);
            printf("%d\n", e.DataSize);
            printf("%d\n", e.OriginalSize);

            printf("Compressed file found! aborting\n");
            return false;
          }*/
          
          if(e.PackingMethod == 0x43707273)
            ft[fi].origsize = e.OriginalSize;
          else
            ft[fi].origsize = 0;

          strcpy(ft[fi].fname, str);
          ft[fi].len = e.DataSize;
          ft[fi].timestamp = e.TimeStamp;
          ft[fi].extract = true;

          ++fi;

          break;
        }
        
        status = STATUS_DONE;

        // Seek over last entry
        PBOENTRY e;
        fread(&e, sizeof(e), 1, i);
        break;
    }
  }

  // Get output dir name
  char outdir[FNAMELEN];
  memset(outdir, 0, FNAMELEN);
  if(strlen(dd) == 0) {
    path p(sf);
    const char *p_str = p.string().c_str();
    if(!p.is_absolute()) { // is NOT an absolute path = is relative
      path currdir = current_path();
      p_str = (currdir /= path(sf)).string().c_str();
    }
    strncpy(outdir, p_str, strlen(p_str)-4);
  } else
    strcpy(outdir, dd);
#ifdef WITH_GUI
  if(gui) {
    hideWindow(true);
    ftg = ft;
    ftgsize = numFiles;
    doutdir = outdir;
    dinfile = sf;
    int r = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EXTRACTDIALOG), NULL, dialogproc);
    hideWindow(false);
    if(r == IDCANCEL) {
      printf("cancelled\n");
      return true;
    }
  }
#endif

  // Ask for overwriting
  if(!overwrite && fileExists(outdir)) {
    char str[256];
    sprintf(str, "Directory '%s' already exists, overwrite files?", outdir);
    #ifdef WITH_GUI
      int ret = MessageBox(NULL, str, VERSIONSTRING, MB_ICONQUESTION|MB_YESNO);
      if(ret != IDYES)
    #else
      char in;
      cout << str << endl;
      cin >> in;
      if(in != 'Y' && in != 'y')
    #endif
      return true; // Abort cleanly
  }

  // Create prefix store file
  if(strlen(prefix) > 0) {
    char oname[FNAMELEN];
    sprintf(oname, "%s/%s", outdir, PREFIXFILE); // FIX THIS FOR WINDOWS
    createDirs(oname);
    FILE *fo = fopen(oname, "wb");
    if (fo) {
      fputs(prefix, fo);
      fclose(fo);
    } else
      cout << "Cannot open: " << oname << endl;
  }

  // Extract files
  for(int o=0;o<numFiles;o++) {
    char oname[FNAMELEN];
    string filename(ft[o].fname);
    sprintf(oname, "%s/%s", outdir, replace_all_copy(filename, "\\", "/").c_str()); // FIX THIS FOR WINDOWS

    if(!ft[o].extract)  {
      fseek(i, ft[o].len, SEEK_CUR); 
      continue;
    }

    int l = ft[o].len;
    printf("Extracting: %s (%d KB)\n", ft[o].fname, ft[o].len/1024);
    createDirs(oname);
    FILE *fo = fopen(oname, "wb");

    if(ft[o].origsize == 0) {
      // Read from pbo, write to file				
      #define BUFSIZE	1024*1024*4
      char *buf = new char[BUFSIZE];
      int w = l;
      while(w > 0) {
        int read = fread(buf, 1, w>BUFSIZE?BUFSIZE:w, i);
        fwrite(buf, read, 1, fo);
        w-=BUFSIZE;			
      }
      delete[] buf;
    } else {
      // Compressed file, read it all to memory & decompress
      // TODO: with large files this will use alot of memory...
      BYTE *buf = new BYTE[l];
      BYTE *outbuf = new BYTE[ft[o].origsize+32];
      fread(buf, 1, l, i);
      bool ret = pboDecompress(buf, outbuf, l, ft[o].origsize);

      fwrite(outbuf, ft[o].origsize, 1, fo);
      
      delete[] buf;
      delete[] outbuf;

      if(!ret) {
        // Checksum failed
        //return false;
        someFailed = true;
      }
      printf("Decompressed %dKB -> %dKB\n", l/1024, ft[o].origsize/1024);
    }

    fclose(fo);
/*
    // Set file time
    if(ft[o].timestamp != 0) {
      HANDLE tf = CreateFile(oname, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
      if(tf != INVALID_HANDLE_VALUE) {
        filetime_t filet = UnixTimeToFileTime(ft[o].timestamp);
        SetFileTime(tf, &filet, NULL, NULL);
        CloseHandle(tf);
      }
    } else {
      printf("Warning! File creation time not set (Invalid time)\n");
    }*/
  }

  fclose(i);
  delete[] ft;

  return !someFailed;
}

int getDirFiles(char *sd, FTENTRY *ftable, int *fti, char excludes[EX_NUM][EX_LEN]) {
  int count = 0;
  path p (sd);

  try {
    if (exists(p)) {
      if (is_directory(p)) {
        for(recursive_directory_iterator i = recursive_directory_iterator(p); i != recursive_directory_iterator(); ++i) {

          char filename[FNAMELEN];
          memset(filename, 0, FNAMELEN);
          const char *rel = i->path().c_str(); // Path relative to sd (starting directory)
          rel += strlen(sd) + 1;
          strcpy(filename, rel);
          if(iequals(filename, PREFIXFILE)) // Do not pack prefix file
            continue;
          if(iequals(filename, EXCLUDEFILE))
            continue;

          if(is_directory(i->path())) { // Skip directories; we only want files
            continue;
          }

          // Check for exclude
          bool skip = false;
          for(int i=0;i<EX_NUM;i++) {
            if(strlen(excludes[i]) > 1 && !strcasecmp(excludes[i], &filename[strlen(filename)-strlen(excludes[i])])) {
              // printf("Skipping: %s - %s\n", fd.cFileName, excludes[i]);
              skip = true;
              break;
            }
          }
          if(skip)
            continue; // File extension is excluded

          ++count;
          if(ftable != NULL) {
            // Fill table. filename...
            static char foo[1024];
            sprintf(foo, "%s/%s", sd, filename);
            strcpy(ftable[*fti].fname, foo);

            // Modification time
            //ftable[*fti].timestamp = (DWORD) FileTimetoUnixTime(fd.ftCreationTime);

            // Size, TODO: 4GB limit check?
            ftable[*fti].len = file_size(i->path());

            ++(*fti);
          }
        }
      } else
        cout << p << " is not a directory\n";
    } else
      cout << p << " does not exist\n";
  }

  catch (const filesystem_error& ex) {
    cout << ex.what() << '\n';
  }

  return count;
}

// Create a PBO, sd = source directory, df = target file
bool pboPack(char *sd, char *df, bool overwrite) {
  // Check for excludes file
  char excludes[EX_NUM][EX_LEN];
  memset(excludes, 0, EX_NUM*EX_LEN);
  char exname[FNAMELEN];
  sprintf(exname, "%s/%s", sd, EXCLUDEFILE); // FIX FOR WINDOWS
  cout << path(sd) << endl;
  FILE *ef = fopen(exname, "rb");
  int eidx = 0;
  if(ef) {
    printf("Excluded: ");
    while(fgets(excludes[eidx], EX_LEN, ef)) {			
      // Strip line feed
      if(excludes[eidx][strlen(excludes[eidx])-2] == 0x0D) // DOS
        excludes[eidx][strlen(excludes[eidx])-2] = 0x00;
      if(excludes[eidx][strlen(excludes[eidx])-1] == 0x0A) // Unix
        excludes[eidx][strlen(excludes[eidx])-1] = 0x00;
      printf("<%s> ", excludes[eidx]);
      ++eidx;
    }
    printf("\n");
    fclose(ef);
  }

  int c = getDirFiles(sd, NULL, NULL, excludes); // Get number of files
  if(c == 0)
    return false;
  printf("%d files\n", c);

  // Allocate file table & fill it
  FTENTRY *ft = new FTENTRY[c];
  static int fti;
  fti = 0;
  getDirFiles(sd, ft, &fti, excludes); // Get files

  // Open output file and create header
  char outname[FNAMELEN];
  if(strlen(df) != 0)
    sprintf(outname, "%s", df);
  else {
    sprintf(outname, "%s.pbo", sd);
  }

  // Ask for overwriting
  if(!overwrite && fileExists(outname)) {
    char str[256];
    sprintf(str, "File %s already exists, overwrite?", outname);
    #ifdef WITH_GUI
      int ret = MessageBox(NULL, str, VERSIONSTRING, MB_ICONQUESTION|MB_YESNO);
      if(ret != IDYES)
    #else
      char in;
      cout << str << endl;
      cin >> in;
      if(in != 'Y' && in != 'y')
    #endif
      return true; // Abort cleanly
  }

  FILE *o = fopen(outname, "w+b");
  if(!o) {
    printf("Unable to open %s for writing!\n", outname);
    return false;
  }

  // Prepare SHA-1
  sha1_context ctx;
  sha1_starts(&ctx);

  // "sreV" Header
  char hdrb[21];
  memset(hdrb, 0, 21);
  strcpy(hdrb+1, "sreV");
  fwrite(hdrb, 21, 1, o);

  // Check for prefix file & write it
  char foo[FNAMELEN];
  sprintf(foo, "%s/%s", sd, PREFIXFILE); // FIX FOR WINDOWS
  FILE *hf = fopen(foo, "rb");
  if(hf) {
    char prefix[FNAMELEN];
    fgets(prefix, FNAMELEN, hf);
    fclose(hf);
    fputs("prefix", o);
    fputc(0x00, o);
    fputs(prefix, o);
    fputc(0x00, o);
    fputc(0x00, o);
    printf("prefix: %s\n", prefix);
  } else {
    fputc(0x00, o); // Header terminator
  }

  // Write file table
  for(int i=0;i<fti;i++) {
    fputs(ft[i].fname+strlen(sd)+1, o);
    fputc(0x00, o);

    PBOENTRY e;
    e.PackingMethod = 0;
    e.OriginalSize = 0;
    e.Reserved = 0;
    e.TimeStamp = ft[i].timestamp;
    e.DataSize = ft[i].len;
    fwrite(&e, sizeof(PBOENTRY), 1, o);
    //printf("file %d: %s\n", i, ft[i].fname);
  }
  
  // Write blank separator block
  memset(hdrb, 0, 21);
  fwrite(hdrb, 21, 1, o);

  // Seek back & calculate hash for current data
  DWORD fooptr = ftell(o);
  BYTE *food = new BYTE[fooptr];
  fseek(o, 0, SEEK_SET);
  fread(food, fooptr, 1, o);
  fseek(o, fooptr, SEEK_SET);
  sha1_update(&ctx, (uchar*) food, fooptr);
  delete[] food;

  // Write file data
  for(int i=0;i<fti;i++) {
    printf("file %d/%d: %s (%d KB)\n", i, fti, ft[i].fname, ft[i].len/1024);

    string filename(ft[i].fname);
    FILE *inp = fopen(replace_all_copy(filename, "\\", "").c_str(), "rb");
    if(!inp) {
      printf("Warning! Cannot open file for reading!\n");
      continue;
    }

    // Read from file, write to pbo
    #define BUFSIZE	1024*1024*4
    char *buf = new char[BUFSIZE];
    int w = ft[i].len;
    while(w > 0) {
      int read = fread(buf, 1, w>BUFSIZE?BUFSIZE:w, inp);
      fwrite(buf, read, 1, o);
      w-=BUFSIZE;			
      sha1_update(&ctx, (uchar*) buf, read);
    }

    delete[] buf; 
    fclose(inp);
  }

  // Write 0x00 + SHA-1 hash
  fputc(0x00, o);
  BYTE sha1sum[20];
  sha1_finish(&ctx, sha1sum);
  fwrite(sha1sum, 20, 1, o);
    
  fclose(o);
  delete[] ft;

  return true;
}

// PBO decompressor
bool pboDecompress(BYTE *buf, BYTE *out, int size, int outSize) {
  size = size-4;
  DWORD *checksumCorrect = (DWORD*) &buf[size];

  memset(out, 0, outSize);

  BYTE *ptr = buf;
  BYTE *optr = out;
  // Read format bit
  while ((ptr - buf) < size) {
    char f = *ptr++;
    for(int i=0;i<8;i++) {
      if ((ptr - buf) > size) break;
      if (optr-out > outSize) break;
      if (f & 0x01) {
        // Direct write byte
        *optr++ = *ptr++;
      } else {
        BYTE *endofwindow = optr;
        int data = 0;
        data = (*(unsigned short*)ptr);
        ptr += 2;
        unsigned int rpos = (data & 0xff) + ((data & 0xf000) >> 4);
        unsigned int rlen = ((data >> 8) & 0x0f) + 3;

        BYTE *windowptr = optr - rpos;
        BYTE *begofwindow;

        if(rpos == 0)
          begofwindow = windowptr-4096;
        else
          begofwindow = optr - rpos;

        for (DWORD j = 0; j < rlen; j++) {
          if(windowptr >= endofwindow) windowptr = begofwindow;
          if(windowptr < out)
            *optr++ = 0x20;
          else
            *optr++ = *windowptr;
          ++windowptr;
        }
      }
      f = f >> 1;
    }
  }
  //printf("Decompressed size: %db", optr-out);

  // Calculate checksum
  int checksum = 0;
  int dlen = outSize;
  for(DWORD i=0;i<(DWORD)dlen;i++)
    checksum += (char)out[i];
  
  //printf("Checksum: %X - %X\n", checksum, checksumCorrect);
  if(checksum != *checksumCorrect) {
    printf("Decompression checksum mismatch! %08X != %08X\n", checksum, *checksumCorrect);
    return false;
  }

  return true;
}