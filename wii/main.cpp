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

// Global variable
char* fileList[256]; // List of file
int fileCount = 0;   // Number of file
char selectedFilePath[260] = "";


// DIR List
void dirlist(char* path)
{
    DIR* pdir = opendir(path);

    if (pdir != NULL)
    {
        u32 sentinel = 0;

        while(true)
        {
            struct dirent* pent = readdir(pdir);
            if(pent == NULL) break;

            if(strcmp(".", pent->d_name) != 0 && strcmp("..", pent->d_name) != 0)
            {
                char dnbuf[260];
                sprintf(dnbuf, "%s/%s", path, pent->d_name);

                struct stat statbuf;
                stat(dnbuf, &statbuf);

                if(S_ISDIR(statbuf.st_mode))
                {
                    printf("%s <DIR>\n", dnbuf);
                    dirlist(dnbuf);
                }
                else
                {
                    printf("%s (%d)\n", dnbuf, (int)statbuf.st_size);
                }
                sentinel++;
            }
        }

        if (sentinel == 0)
            printf("empty\n");

        closedir(pdir);
        printf("\n");
    }
    else
    {
        printf("opendir() failure.\n");
    }
}



// Function to check if file is a GDI / CDI / BIN / CUE / NRG / MDS / ELF / CHD
// Only GDI, NRG and MDS should be supported for now
bool hasValidExtension(const char* filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    const char* ext = &filename[len - 4];
    
    // Lowercase to make comparison not case sensible
    char extLower[5];
    for (int i = 0; i < 4; i++) {
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

// Function to list file in a folder
void listFilesInDirectory(const char* dirPath) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(dirPath)) != NULL) {
        fileCount = 0;

        // "No disc (BIOS)" in first position
        fileList[fileCount] = strdup("[No disc - Boot to BIOS]");
        fileCount++;

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) { // If it's a regular file
                if (hasValidExtension(entry->d_name)) {
                    fileList[fileCount] = strdup(entry->d_name);
                    fileCount++;
                }
            }
        }
        closedir(dir);
    } else {
        perror("Could not open directory");
    }
}

// Function to display menu & allow selection with Wiimote
int displayMenuAndSelectFile() {
    int selectedIndex = 0;
    bool exitMenu = false;

    while (!exitMenu) {
        printf("\033[2J\033[H"); // Clear Screen
        printf("Select a game file: (GDI Work, CDI shouldn't work)\n");

        for (int i = 0; i < fileCount; i++) {
            if (i == selectedIndex) {
                printf("> %s\n", fileList[i]);
            } else {
                printf("  %s\n", fileList[i]);
            }
        }

        printf("\nUse UP/DOWN on the D-Pad to navigate, A to select, HOME to exit\n");

        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);

        if (pressed & WPAD_BUTTON_UP) {
            if (selectedIndex > 0) selectedIndex--;
        } else if (pressed & WPAD_BUTTON_DOWN) {
            if (selectedIndex < fileCount - 1) selectedIndex++;
        } else if (pressed & WPAD_BUTTON_A) {
            exitMenu = true;
        } else if (pressed & WPAD_BUTTON_HOME) {
            return -1; // Retour au menu principal
        }

        usleep(20000); // Wait a bit so GPU isn't overloaded (16667 = 1 frame @ 60 FPS)
        VIDEO_WaitVSync(); // Synchronization to avoid flickering (Wait for the next frame)
    }

    return selectedIndex;
}

void SetApplicationPath(wchar* path);

int main(int argc, wchar* argv[])
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
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    
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
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    /* // Claude AI */


    // Initialise SD Card
    if(fatInitDefault())
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
        while(1) {
            WPAD_ScanPads();
            if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
            VIDEO_WaitVSync();
        }
    }

    void SetApplicationPath(const wchar* path);

    // Call function to choose a file
    const char* dirPath = "sd:/discs/";
    printf(dirPath);
    listFilesInDirectory(dirPath);

    // If there is file (there always will be with the option "No Disc boot to BIOS")
    if (fileCount > 0) {
        int selectedIndex = displayMenuAndSelectFile();
        
        if (selectedIndex == 0) {
            // If "No Disc Option" is selected
            printf("\x1b[2J\x1b[H");
            printf("Booting to BIOS (no disc)...\n");
            strcpy(selectedFilePath, ""); // Pas de fichier
        }
        else if (selectedIndex > 0) {
            // A file has been selected
            sprintf(selectedFilePath, "%s%s", dirPath, fileList[selectedIndex]);
            printf("\x1b[2J\x1b[H");
            printf("Selected file: %s\n", selectedFilePath);
        }
        else {
            // HOME pressed
            printf("Exiting...\n");
            for (int i = 0; i < fileCount; i++) {
                free(fileList[i]);
            }
            return 0;
        }
    } else {
        // This part is never called because I added defaut bios boot option
        printf("No .gdi or .cdi files found in the discs directory.\n");
        usleep(100000); // Wait time to let user see message before booting to BIOS
        printf("Booting to BIOS...\n");
    }

    int rv = EmuMain(argc, argv);

    // Free memory for filename
    for (int i = 0; i < fileCount; i++) {
        free(fileList[i]);
    }

    return rv;
}

// Select file
int os_GetFile(char *szFileName, char *szParse, u32 flags)
{
    if (strlen(selectedFilePath) > 0) {
        strcpy(szFileName, selectedFilePath);
        return true;
    }
    return false; // No file (boot BIOS)
}

double os_GetSeconds()
{
    return clock()/(double)CLOCKS_PER_SEC;
}

int os_msgbox(const wchar* text, unsigned int type)
{
    printf("OS_MSGBOX: %s\n", text);
    return 0;
}
