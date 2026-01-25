#include "types.h"
#include <unistd.h>
#include "iso.h"
#include <fat.h>
#include <dirent.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <gccore.h> // needed, or VScode will show errors

// Global variables
struct FileEntry
{
  char name[256];
  char fullPath[512];
  bool isDirectory;
};

FileEntry fileList[256];
int fileCount = 0;
char selectedFilePath[512] = "";
char currentPath[512] = "sd:/discs/";
const int ITEMS_PER_PAGE = 10; // Can be up to 20 but bad UX unless lots of roms
int currentPage = 0;

// Function to check if file is a GDI / CDI / BIN / CUE / NRG / MDS / ELF / CHD
// Only GDI is supported, maybe NRG and MDS and BIN/CUE and ELF
bool hasValidExtension(const char *filename)
{
  size_t len = strlen(filename);
  if (len < 4)
    return false;

  const char *ext = &filename[len - 4];

  // Lowercase to make comparison not case sensible
  char extLower[5];
  for (int i = 0; i < 4; i++)
  {
    extLower[i] = tolower(ext[i]);
  }
  extLower[4] = '\0';

  // Check for valid extensions
  return (strcmp(extLower, ".gdi") == 0 ||
          strcmp(extLower, ".cdi") == 0 ||
          strcmp(extLower, ".bin") == 0 ||
          strcmp(extLower, ".cue") == 0 ||
          strcmp(extLower, ".nrg") == 0 ||
          strcmp(extLower, ".mds") == 0 ||
          strcmp(extLower, ".elf") == 0 ||
          strcmp(extLower, ".chd") == 0);
}

// Function to list files and directories in a folder
void listFilesInDirectory(const char *dirPath)
{
  DIR *dir;
  struct dirent *entry;
  struct stat statbuf;

  if ((dir = opendir(dirPath)) != NULL)
  {
    fileCount = 0;

    // Read all entries in directory
    while ((entry = readdir(dir)) != NULL && fileCount < 256)
    {
      // Ignore "." and ".." directories
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      {
        continue;
      }

      // Build full path
      char fullPath[512];
      snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, entry->d_name);

      // Get file stats
      if (stat(fullPath, &statbuf) == 0)
      {
        if (S_ISDIR(statbuf.st_mode))
        {
          // It's a directory, add it
          snprintf(fileList[fileCount].name, sizeof(fileList[fileCount].name), "[%s]", entry->d_name);
          strcpy(fileList[fileCount].fullPath, fullPath);
          fileList[fileCount].isDirectory = true;
          fileCount++;
        }
        else if (hasValidExtension(entry->d_name))
        {
          // It's a valid file, add it
          strcpy(fileList[fileCount].name, entry->d_name);
          strcpy(fileList[fileCount].fullPath, fullPath);
          fileList[fileCount].isDirectory = false;
          fileCount++;
        }
      }
    }
    // Sorting : Folder first, then file (alphabeticaly)
    for (int i = 0; i < fileCount - 1; i++)
    {
      for (int j = i + 1; j < fileCount; j++)
      {
        bool shouldSwap = false;

        // If i is a file and j a folder : invert
        if (!fileList[i].isDirectory && fileList[j].isDirectory)
        {
          shouldSwap = true;
        }
        // If same type, sorting alphabeticaly
        else if (fileList[i].isDirectory == fileList[j].isDirectory)
        {
          if (strcmp(fileList[i].name, fileList[j].name) > 0)
          {
            shouldSwap = true;
          }
        }

        if (shouldSwap)
        {
          FileEntry temp = fileList[i];
          fileList[i] = fileList[j];
          fileList[j] = temp;
        }
      }
    }
  }
  else
  {
    perror("Could not open directory");
  }
}

// Function to display menu and allow selection with Wiimote
int displayMenuAndSelectFile()
{
  int selectedIndex = 0;
  currentPage = 0;

  while (true)
  {
    printf("\033[2J\033[H"); // Clear Screen
    printf("Current directory: %s\n", currentPath);
    printf("Select a game file: (Only GDI Works, Maybe NRG/MDS. (Try Bin/CUE and ELF)\n\n");

    // Calculate pagination
    int totalPages = (fileCount + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (totalPages < 1)
      totalPages = 1;
    int startIndex = currentPage * ITEMS_PER_PAGE;
    int endIndex = startIndex + ITEMS_PER_PAGE;
    if (endIndex > fileCount)
      endIndex = fileCount;

    // Display files for current page
    for (int i = startIndex; i < endIndex; i++)
    {
      if (i == selectedIndex)
      {
        printf("> %s\n", fileList[i].name);
      }
      else
      {
        printf("  %s\n", fileList[i].name);
      }
    }

    // Display page info
    printf("\n--- Page %02d/%02d ---\n", currentPage + 1, totalPages);
    printf("UP/DOWN/LEFT/RIGHT: Navigate | A: Select | B: Back | 1: BIOS | HOME: Exit\n");

    WPAD_ScanPads();
    u32 pressed = WPAD_ButtonsDown(0);

    if (pressed & WPAD_BUTTON_1)
    {
      return -2; // Boot to BIOS
    }
    if (pressed & WPAD_BUTTON_UP)
    {
      if (selectedIndex > 0)
      {
        selectedIndex--;
        // Change page if necessary
        if (selectedIndex < startIndex)
        {
          currentPage--;
        }
      }
    }
    else if (pressed & WPAD_BUTTON_DOWN)
    {
      if (selectedIndex < fileCount - 1)
      {
        selectedIndex++;
        // Change page if necessary
        if (selectedIndex >= endIndex)
        {
          currentPage++;
        }
      }
    }
    else if (pressed & WPAD_BUTTON_LEFT)
    {
      // Previous page
      if (currentPage > 0)
      {
        currentPage--;
        selectedIndex = currentPage * ITEMS_PER_PAGE;
      }
    }
    else if (pressed & WPAD_BUTTON_RIGHT)
    {
      // Next page
      if (currentPage < totalPages - 1)
      {
        currentPage++;
        selectedIndex = currentPage * ITEMS_PER_PAGE;
      }
    }
    else if (pressed & WPAD_BUTTON_A)
    {
      // Check if it's a directory
      if (fileList[selectedIndex].isDirectory)
      {
        // Enter directory
        strcpy(currentPath, fileList[selectedIndex].fullPath);
        listFilesInDirectory(currentPath);
        selectedIndex = 0;
        currentPage = 0;
      }
      else
      {
        // It's a file or "No disc" option
        return selectedIndex;
      }
    }
    else if (pressed & WPAD_BUTTON_B)
    {
      // Go back to parent directory
      if (strcmp(currentPath, "sd:/discs/") != 0 && strcmp(currentPath, "sd:/discs") != 0)
      {
        char *lastSlash = strrchr(currentPath, '/');
        if (lastSlash != NULL && lastSlash != currentPath)
        {
          *lastSlash = '\0';
        }
        listFilesInDirectory(currentPath);
        selectedIndex = 0;
        currentPage = 0;
      }
    }
    else if (pressed & WPAD_BUTTON_HOME)
    {
      return -1; // Exit to main menu
    }

    usleep(20000);     // Wait a bit so GPU isn't overloaded (16667 = 1 frame @ 60 FPS)
    VIDEO_WaitVSync(); // Synchronization to avoid flickering (Wait for the next frame)
  }

  return selectedIndex;
}

void SetApplicationPath(wchar *path);

int main(int argc, wchar *argv[])
{
  /* Claude AI */
  // Initialize the video system
  // (yes, right now before gxrend, otherwise no game selector)
  VIDEO_Init();
  /* // Claude AI */

  // This function initialises the attached controllers (devkit wii-example)
  PAD_Init();
  WPAD_Init();

  /* Claude AI */
  // Obtain the preferred video mode from the system
  GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);

  // Allocate memory for the display in the uncached region
  void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

  // ** ADD THIS LINE TO INITIALIZE THE CONSOLE **
  console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);

  // Set up the video registers with the chosen mode
  VIDEO_Configure(rmode);

  // Tell the video hardware where our display memory is
  VIDEO_SetNextFramebuffer(xfb);

  // Make the display visible
  VIDEO_SetBlack(false);

  // Flush the video register changes to the hardware
  VIDEO_Flush();

  // Wait for Video setup to complete
  VIDEO_WaitVSync();
  if (rmode->viTVMode & VI_NON_INTERLACE)
    VIDEO_WaitVSync();
  /* // Claude AI */

  // Initialise SD Card
  if (fatInitDefault())
  {
    printf("SD card mounted!\n");
    // This part export every printf to a txt file and disable every printf afterwards
    /*
    if (!fopen("/dolphin", "r"))
        freopen("/ndclog.txt", "w", stdout);
        */
  }
  else
  {
    // If no SD card :
    printf("ERROR: Failed to mount SD card!\n");
    printf("Press HOME to exit.\n");
    while (1)
    {
      WPAD_ScanPads();
      if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
        exit(0);
      usleep(100000); // Wait time to save GPU
      VIDEO_WaitVSync();
    }
  }

  void SetApplicationPath(const wchar *path);

  // List files in initial directory
  listFilesInDirectory(currentPath);

  // If there is file (there always will be with the option "No Disc boot to BIOS")
  if (fileCount > 0)
  {
    int selectedIndex = displayMenuAndSelectFile();

    if (selectedIndex == -2)
    {
      // Boot to BIOS (button 1 pressed)
      printf("\x1b[2J\x1b[H"); // Clear Screen
      printf("Booting to BIOS (no disc)...\n");
      strcpy(selectedFilePath, ""); // No File
    }
    else if (selectedIndex >= 0)
    {
      // A file has been selected
      strcpy(selectedFilePath, fileList[selectedIndex].fullPath);
      printf("\x1b[2J\x1b[H"); // Clear Screen
      printf("Selected file: %s\n", selectedFilePath);
    }
    else
    {
      // HOME pressed
      printf("Exiting...\n");
      return 0;
    }
  }
  else
  {
    // If no valid disc file found
    printf("No valid disc files found. Booting to BIOS\n");
    usleep(100000); // Wait time to let user see message before booting to BIOS
    printf("Booting to BIOS...\n");
  }

  int rv = EmuMain(argc, argv);

  return rv;
}

// Select file
int os_GetFile(char *szFileName, char *szParse, u32 flags)
{
  if (strlen(selectedFilePath) > 0)
  {
    strcpy(szFileName, selectedFilePath);
    return true;
  }
  return false; // No file (boot BIOS)
}

double os_GetSeconds()
{
  return clock() / (double)CLOCKS_PER_SEC;
}

int os_msgbox(const wchar *text, unsigned int type)
{
  printf("OS_MSGBOX: %s\n", text);
  return 0;
}
