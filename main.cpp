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

#ifndef _NIX
#else
typedef void *PVOID;
typedef PVOID HANDLE;
typedef HANDLE HWND;
#endif
#include "pbo.h"

#ifndef _NIX
HWND win = NULL;

void hideWindow(bool state) {
  ShowWindow(win, !state);
}
#endif

void usage(void) {
  printf("\nUsage:\n");
  printf("Extract a pbo:\n");
  printf("  cpbo.exe [-y] -e (filename.pbo) [directory]\n");
  printf("  directory name is optional, PBO file name used if undefined\n");
  printf("  optional -y parameter overwrites directory without asking\n\n");
  printf("Make a pbo:\n");
  printf("  cpbo.exe [-y] -p (directory) [filename.pbo]\n");
  printf("  pbo name is optional, directory name used if undefined\n");
  printf("  optional -y parameter overwrites pbo without asking\n\n");
  #ifndef _NIX
  printf("Associate cpbo with PBO files and directories:\n");
  printf("  cpbo.exe -a\n");
  #endif
  exit(0);
}

int main(int argc, char* argv[]) {
  printf("%s <http://www.kegetys.net>\n", VERSIONSTRING);

  #ifndef _NIX
  // Compare console title & try to find out if we were run from terminal or directly
  char ctit[256];
  char cmod[256];
  GetModuleFileName(NULL, cmod, 256);
  GetConsoleTitle(ctit, 256);
  SetConsoleTitle(VERSIONSTRING);
  bool runDirectly = !strcasecmp(ctit, cmod);

  char rnd[256];
  sprintf(rnd, "_cpbo_tmp__%d__", GetTickCount());
  SetConsoleTitle(rnd);
  win = FindWindow(NULL, rnd);
  SetConsoleTitle(VERSIONSTRING);
  #endif

  if(argc < 2) {
    #ifndef _NIX
    // If title & module name match we were propably run directly
    if(runDirectly) {
      // Double clicked exe, display query for file association
      int ret = MessageBox(NULL, "No parameters\n\nDo you wish to associate cpbo with PBO files\nand directories (Explorer right click menu)?", VERSIONSTRING, MB_ICONQUESTION|MB_YESNO);
      if(ret == IDYES)
        goto assign;
      else
        exit(0);
    } else
    #else
      usage(); // Ran from console
    #endif
  }

  // Parse parameters
  bool gui = false;
  bool overwrite = false;
  for(int ai=1;ai<argc;ai++) {
    if(!strcasecmp("-y", argv[ai])) {
      // Overwrite all files
      overwrite = true;
    }

    if(!strcasecmp("-gui", argv[ai]))
      gui = true;

    if(!strcasecmp("-e", argv[ai])) {
      if(argc-ai < 2)
        usage();

      // Extract...
      char *odir = "";
      if(argc >= ai+3) {
        char *last = argv[ai+2] + strlen(argv[ai+2]) - 1;
        if (*last == '/' || *last == '\\') // If directory path has trailing slash then remove it (replace with null terminator)
          *last = '\0';
        odir = argv[ai+2];
      }

      printf("Extracting %s\n", argv[ai+1]);
      if(pboEx(argv[ai+1], odir, overwrite, gui)) {
        printf("Done.\n");
        return 1;
      } else {
        printf("Failed!\n");
        #ifndef _NIX
        //MessageBox(NULL, "PBO extract failed", "cpbo", MB_ICONSTOP);
        MessageBox(NULL, "Extract of one or more files failed", "cpbo", MB_ICONSTOP);
        #endif
        return -1;
      }
    }

    if(!strcasecmp("-p", argv[ai])) {
      if(argc-ai < 2)
        usage();

      char *last = argv[ai+1] + strlen(argv[ai+1]) - 1;
      if (*last == '/' || *last == '\\') // If directory path has trailing slash then remove it (replace with null terminator)
        *last = '\0';

      // Create PBO.
      char *ofile = "";
      if(argc >= ai+3)
        ofile = argv[ai+2];

      printf("Creating %s\n", argv[ai+1]);
      if(pboPack(argv[ai+1], ofile, overwrite)) {
        printf("Done.\n");
        return 1;
      } else {
        printf("Failed!\n");
        #ifndef _NIX
        MessageBox(NULL, "PBO creation failed", "cpbo", MB_ICONSTOP);
        #endif
        return -1;
      }
    }

    #ifndef _NIX
    if(!strcasecmp("-a", argv[ai])) {
  assign:
      // Create file associations
      char foo[1024];
      GetModuleFileName(NULL, foo, 1024);
      printf("%s\n", foo);

      // for PBO...
      HKEY hKey;
      DWORD dwDisp = 0;
      LPDWORD lpdwDisp = &dwDisp;
      DWORD dwVal = 100;
      LONG ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, ".pbo\\shell\\Extract\\command", 0L,NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,lpdwDisp);
      if(ret == ERROR_SUCCESS) {
        char foo2[1024];
        sprintf(foo2, "\"%s\" -e \"%%1\"", foo);

        RegSetValueEx(hKey, NULL, 0L, REG_SZ, (const BYTE *) foo2, strlen(foo2));
        RegCloseKey(hKey);
      } else
        printf("PBO association failed! Verify registry permissions\n");

      ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, ".pbo\\shell\\extract PBO...\\command", 0L,NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,lpdwDisp);
      if(ret == ERROR_SUCCESS) {
        char foo2[1024];
        sprintf(foo2, "\"%s\" -gui -e \"%%1\"", foo);

        RegSetValueEx(hKey, NULL, 0L, REG_SZ, (const BYTE *) foo2, strlen(foo2));
        RegCloseKey(hKey);
      } else
        printf("PBO association failed! Verify registry permissions\n");

      // For directories
      ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, "Folder\\shell\\create PBO\\command", 0L,NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,lpdwDisp);
      if(ret == ERROR_SUCCESS) {
        char foo2[1024];
        sprintf(foo2, "\"%s\" -p \"%%1\"", foo);

        RegSetValueEx(hKey, NULL, 0L, REG_SZ, (const BYTE *) foo2, strlen(foo2));
        RegCloseKey(hKey);
      } else
        printf("Directory association failed! Verify registry permissions\n");

      // PBO Icon
      ret = RegCreateKeyEx(HKEY_CLASSES_ROOT, ".pbo\\DefaultIcon", 0L,NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, lpdwDisp);
      if(ret == ERROR_SUCCESS) {
        if(dwDisp == REG_CREATED_NEW_KEY)  {
          // Set new icon
          char foo2[1024];
          sprintf(foo2, "%s,0", foo);

          RegSetValueEx(hKey, NULL, 0L, REG_SZ, (const BYTE *) foo2, strlen(foo2));
        } else 
          printf("Default PBO icon already exists, not overwritten\n");
        RegCloseKey(hKey);
      }

      printf("Done.\n");

      if(runDirectly)
        MessageBox(NULL, "Done", "cpbo", MB_ICONINFORMATION);
      return 1;
    }
    #endif
  }
}