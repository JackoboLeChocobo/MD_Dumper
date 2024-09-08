// SDL Libraries for GUI
#include <SDL.h>				//Main Library file
#include <SDL_image.h>			//For PNG loading files

#include <libusb.h>				//Library for detecting the MD Dumper device

//Read CSV Files
#include "csv.h"
#include "csv.c"

//File Manager Library
#include "sfd.h"
#include "sfd.c"

// USB Special Command
#define WAKEUP             0x10	 // WakeUP for first STM32 Communication
#define READ_MD            0x11
#define READ_MD_SAVE       0x12
#define WRITE_MD_SAVE      0x13
#define WRITE_MD_FLASH 	   0x14
#define ERASE_MD_FLASH     0x15
#define READ_SMS           0x16
#define INFOS_ID           0x18
#define MAPPER_SSF2        0x20
#define CREATE_MX_BUFFER   0x60
#define WRITE_MX_BUFFER    0x61
#define FLASH_MX_BUFFER    0x62
#define SECTOR_ERASE       0x64
#define SEND_FLASH_ALGO    0x65


// Version
#define MAX_VERSION 	2
#define MIN_VERSION 	1

char * game_rom = NULL;
char * game_name = NULL;

// CSV Gamelist specific Variable
#define BUFFER_SIZE 1024
#define MAX_NON_EMPTY_CELLS 50 // Ajustez cette valeur selon vos besoins
#define CHKSM_TEXT_SIZE 16
#define ROM_TEXT_SIZE 3
#define COL3_TEXT_SIZE 4 // Taille maximale pour les valeurs texte de la colonne C
#define TEXT_SIZE 16

FILE *fp;
struct csv_parser p;
size_t bytes_read;
unsigned char options = 0;

char buffer[BUFFER_SIZE];
size_t bytes_read;
int current_row = 0;
int current_col = 0;
char cell_A2[BUFFER_SIZE] = ""; // Variable pour stocker la donnée de la cellule A2
int non_empty_cells_in_col_A = 0; // Compteur de cellules non vides en colonne A
int Index_chksm=0;

// Tableau pour stocker les valeurs non vides en colonne A CHKSM sous forme de texte
char chksm_text_values[MAX_NON_EMPTY_CELLS][CHKSM_TEXT_SIZE + 1];
int chksm_text_values_count = 0;
// Tableau pour stocker les valeurs de la troisième colonne rom_size
char rom_size_text_values[MAX_NON_EMPTY_CELLS][CHKSM_TEXT_SIZE + 1];
int rom_size_text_values_count = 0;
// Tableau pour stocker les différentes valeurs du fichier csv
char txt_csv_chksm[4];
unsigned short csv_chksm=0;
unsigned char txt_csv_game_size[4+1];
unsigned char csv_game_size=0;
char txt_mapper_number[1];
unsigned char csv_mapper_number=0;
char txt_save_type[1];
unsigned char csv_save_type=0;
char txt_save_size[2];
unsigned char csv_save_size=0;

//CSV Flashlist specific Variables

#define CHIPID_TEXT_SIZE 48 // taille de toute la chaine
#define TEXT_SIZE2 48

int current_row2 = 0;
int current_col2 = 0;
int non_empty_cells_in_col_A2 = 0; // Compteur de cellules non vides en colonne A
char chipid_text_values[MAX_NON_EMPTY_CELLS][CHIPID_TEXT_SIZE + 1];
int chipid_text_values_count = 0;

char txt_csv_deviceID[4];
unsigned short csv_deviceID=0;
unsigned char txt_csv_flash_size[3+1];
unsigned char csv_flash_size=0;
unsigned long flash_size=0;
unsigned char txt_csv_erase_algo[2+1];
unsigned char csv_erase_algo=0;
unsigned char txt_csv_write_algo[2+1];
unsigned char csv_write_algo=0;
unsigned char txt_csv_flash_name[11+1];
unsigned char txt_csv_man_name[18+1];
unsigned char txt_csv_voltage[2+1];
unsigned char csv_voltage=0;
unsigned char flash_algo=0;

FILE *fp2;
struct csv_parser p2;

// LibUSB Specific Var

int res                      = 0;        /* return codes from libusb functions */
int kernelDriverDetached     = 0;        /* Set to 1 if kernel driver detached */
unsigned long len            = 0;        /* Number of bytes transferred. */
unsigned char usb_buffer_out[64] = {0};  /* 64 byte transfer buffer OUT */
libusb_device_handle* handle = 0;        /* handle for USB device */
int numBytes                 = 0;        /* Actual bytes transferred. */
unsigned char usb_buffer_in[64] = {0};   /* 64 byte transfer buffer IN */

// MD Dumper Var

unsigned long address=0;
unsigned long i=0;
unsigned long j=0;
unsigned long k=0;
unsigned char *buffer_header = NULL;
unsigned char *buffer_rom = NULL;
unsigned char md_dumper_type=0;
char dump_name[64];
unsigned char region[5];
char *game_region = NULL;
const char unk[] = {"unknown"};
int checksum_header = 0;
int use_gui=0;							/* 0=CLI Mode, 1=GUI Mode */
int opts_choice=0; 						/* 0=Read Mode / Game, 1=Read Mode / Save, 2=Write Mode / Game, 3=Write Mode / Save */
int dump_mode=0; 						/* 0=Auto, 1=Manual, 2=Bankswitch */
int dump_manual_size_opts=0; 			/* 0=32KB, 1=64KB, 2=128KB, 3=256KB, 4=512KB, 5=1024KB, 6=2048KB, 7=4096KB */
int dump_manual_cart_mode_opts=0; 		/* 0=MD MODE, 1=SMS MODE */
int write_flash=1;				 		/* 1=Write, 0=Erase */
int write_save=1;				 		/* 1=Write, 0=Erase */
int dump_sram_size_opts=0; 				/* 0=Automatic, 1=8192, 2=32768 */
int sram_type_opts=0; 					/* 0=serial_spi, 1=serial_i2c, 2=parallel_sram */
int game_size=0;
int manual_game_size=0;
int manual_game_cart_mode=0;
unsigned long save_size1 = 0;
unsigned long save_size2 = 0;
unsigned long save_size = 0;
unsigned long save_address = 0;
unsigned char *BufferROM;
unsigned char *BufferSAVE;
char empty_flash[512];
char dump_flash[512];
FILE *myfile;
unsigned char NumberOfBank=0;
unsigned char ActualBank=0;
unsigned long offset=0;
unsigned short rom_id=0;
unsigned short flash_id=0;
unsigned char chip_id=0;
unsigned char manufacturer_id=0;
const char * wheel[] = { "-","\\","|","/"}; //erase wheel

//Others Functions
void Display_Help(char *prog_name)
{
    SDL_Log("\n");
    SDL_Log("How to use the program:\n");
    SDL_Log("\n");
    SDL_Log("GUI Mode:\n");
    SDL_Log("  %s -gui\n", prog_name);
    SDL_Log("\n");
    SDL_Log("CLI Mode:\n");
    SDL_Log("\n");
    SDL_Log("  %s -read_rom auto  -  Auto Mode\n", prog_name);
    SDL_Log("  %s -read_rom bankswitch  -  Bankswitch Mode\n", prog_name);
    SDL_Log("  %s -read_rom manual (32|64|128|256|512|1024|2048|4096) (md|sms) -  Manual Mode\n", prog_name);
    SDL_Log("  %s -backup_memory (0|8192|32768) (serial_spi|serial_i2c|parallel_sram) -  Read Save/Memory Data\n", prog_name);
    SDL_Log("\n");
    SDL_Log("  %s -erase_flash -  Erase Flash Data\n", prog_name);
    SDL_Log("  %s -write_flash -  Write Flash Data\n", prog_name);
    SDL_Log("  %s -erase_memory (serial_spi|serial_i2c|parallel_sram) -  Erase Save/Memory Data\n", prog_name);
    SDL_Log("  %s -restore_memory (serial_spi|serial_i2c|parallel_sram) - Write Save/Memory Data\n", prog_name);
    SDL_Log("\n");
}

void cb1(void *s, size_t len, void *data)
{
    int target_row = 0; // Index basé sur 0 (deuxième ligne a l'index 1)
    int col_A = 0;     // Index basé sur 0 (première colonne a l'index 0)
    int col_C = 1;     // Index basé sur 0 (troisième colonne a l'index 2)

    /* if (current_row == target_row && current_col == col_A) {
         // Copier la donnée dans la variable cell_A2
         strncpy(cell_A2, (char *)s, len);
         cell_A2[len] = '\0'; // Ajouter le caractère de fin de chaîne
     }*/

    // Compter les cellules non vides en colonne A et stocker les valeurs sous forme de texte
    if (current_row > 0 && current_col == col_A && len > 0)   // Ignorer la première ligne
    {
        non_empty_cells_in_col_A++;
        if (chksm_text_values_count < MAX_NON_EMPTY_CELLS)
        {
            // Assurer que la chaîne est de taille TEXT_SIZE
            strncpy(chksm_text_values[chksm_text_values_count], (char *)s, TEXT_SIZE);
            chksm_text_values[chksm_text_values_count][TEXT_SIZE] = '\0'; // Ajouter le caractère de fin de chaîne
            chksm_text_values_count++;
        }
    }
}

// Fonction de rappel pour traiter la fin de chaque ligne
void cb2(int c, void *data)
{
    current_row++;
    current_col = 0; // Réinitialiser le compteur de colonnes à la fin de chaque ligne
}

void cb3(void *s, size_t len, void *data)
{
    int target_row = 0; // Index basé sur 0 (deuxième ligne a l'index 1)
    int col_A = 0;     // Index basé sur 0 (première colonne a l'index 0)
    int col_C = 1;     // Index basé sur 0 (troisième colonne a l'index 2)

    /* if (current_row == target_row && current_col == col_A) {
         // Copier la donnée dans la variable cell_A2
         strncpy(cell_A2, (char *)s, len);
         cell_A2[len] = '\0'; // Ajouter le caractère de fin de chaîne
     }*/

    // Compter les cellules non vides en colonne A et stocker les valeurs sous forme de texte
    if (current_row2 > 0 && current_col2 == col_A && len > 0)   // Ignorer la première ligne
    {
        non_empty_cells_in_col_A2++;
        if (chipid_text_values_count < MAX_NON_EMPTY_CELLS)
        {
            // Assurer que la chaîne est de taille TEXT_SIZE
            strncpy(chipid_text_values[chipid_text_values_count], (char *)s, TEXT_SIZE2);
            chipid_text_values[chipid_text_values_count][TEXT_SIZE2] = '\0'; // Ajouter le caractère de fin de chaîne
            chipid_text_values_count++;
        }
    }
}


// Fonction de rappel pour traiter la fin de chaque ligne
void cb4(int c, void *data)
{
    current_row2++;
    current_col2 = 0; // Réinitialiser le compteur de colonnes à la fin de chaque ligne
}



//Timer functions according to Operating Systems
#if defined(_WIN32)		//Windows
clock_t microsec_start;
clock_t microsec_end;

void timer_start()
{
    microsec_start = clock();
}
void timer_end()
{
    microsec_end = clock();
}
void timer_show()
{
    SDL_Log("~ Elapsed time: %lds", (microsec_end - microsec_start)/1000);
    SDL_Log(" (%ldms)\n", (microsec_end - microsec_start));
}
#else 				//Others
struct timeval ostime;
long microsec_start = 0;
long microsec_end = 0;

void timer_start()
{
    gettimeofday(&ostime, NULL);
    microsec_start = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
}

void timer_end()
{
    gettimeofday(&ostime, NULL);
    microsec_end = ((unsigned long long)ostime.tv_sec * 1000000) + ostime.tv_usec;
}

void timer_show()
{
    SDL_Log("~ Elapsed time: %lds", (microsec_end - microsec_start)/1000000);
    SDL_Log(" (%ldms)\n", (microsec_end - microsec_start)/1000);
}
#endif


unsigned int trim(unsigned char * buf, unsigned char is_out)
{
    unsigned char i=0, j=0;
    unsigned char tmp[49] = {0}; //max
    unsigned char tmp2[49] = {0}; //max
    unsigned char next = 1;

    /*check ascii remove unwanted ones and transform upper to lowercase*/
    if(is_out)
    {
        while(i<48)
        {
            if(buf[i]<0x30 || buf[i]>0x7A || (buf[i]>0x29 && buf[i]<0x41) || (buf[i]>0x5A && buf[i]<0x61))
                buf[i] = 0x20; //remove shiiit
            if(buf[i]>0x40 && buf[i]<0x5B)
                buf[i] += 0x20; //to lower case A => a
            i++;
        }
        i=0;
    }

    while(i<48)
    {
        if(buf[i]!=0x20)
        {
            if(buf[i]==0x2F)
                buf[i] = '-';
            tmp[j]=buf[i];
            tmp2[j]=buf[i];
            next = 1;
            j++;
        }
        else
        {
            if(next)
            {
                tmp[j]=0x20;
                tmp2[j]='_';
                next = 0;
                j++;
            }
        }
        i++;
    }

    next=0;
    if(tmp2[0]=='_')
    {
        next=1;    //offset
    }
    if(tmp[(j-1)]==0x20)
    {
        tmp[(j-1)] = tmp2[(j-1)]='\0';
    }
    else
    {
        tmp[j] = tmp2[j]='\0';
    }

    if(is_out)  //+4 for extension
    {
        game_rom = (char *)malloc(j-next +4);
        memcpy((unsigned char *)game_rom, (unsigned char *)tmp2 +next, j-next); //stringed file
    }

    game_name = (char *)malloc(j-next);
    memcpy((unsigned char *)game_name, (unsigned char *)tmp +next, j-next); //trimmed
    return 0;
}


int Detect_Device(void)
{
    /* Initialise libusb. */
    SDL_Log("Init LibUSB... \n");
    res = libusb_init(0);
    if (res != 0)
    {
        SDL_Log("Error initialising libusb.\n");
        return 1;
    }

    SDL_Log("LibUSB Init Sucessfully ! \n");

    SDL_Log("Detecting MD Dumper... \n");

    /* Get the first device with the matching Vendor ID and Product ID. If
     * intending to allow multiple demo boards to be connected at once, you
     * will need to use libusb_get_device_list() instead. Refer to the libusb
     * documentation for details. */

    handle = libusb_open_device_with_vid_pid(0, 0x0483, 0x5740);

    if (!handle)
    {
        SDL_Log("Unable to open device.\n");
        return 1;
    }

    /* Claim interface #0. */

    if(libusb_kernel_driver_active(handle, 0) == 1)
    {
        SDL_Log("Kernel Driver Active");
        if(libusb_detach_kernel_driver(handle, 0) == 0)
            SDL_Log("Kernel Driver Detached!");
        else
        {
            SDL_Log("Couldn't detach kernel driver!\n");
            libusb_close(handle);
        }
    }
    res = libusb_claim_interface(handle, 0);
    //SDL_Log("Test 1 : %d\n",res);
    if (res != 0)
    {
        if(libusb_kernel_driver_active(handle, 1) == 1)
        {
            SDL_Log("Kernel Driver Active");
            if(libusb_detach_kernel_driver(handle, 1) == 0)
                SDL_Log("Kernel Driver Detached!");
            else
            {
                SDL_Log("Couldn't detach kernel driver!\n");
                libusb_close(handle);
            }
        }
        res = libusb_claim_interface(handle, 1);
        //SDL_Log("Test 2 : %d\n",res);
        if (res != 0)
        {
            SDL_Log("Error claiming interface.\n");
            return 1;
        }
    }

    // Clean Buffer
    for (i = 0; i < 64; i++)
    {
        usb_buffer_in[i]=0x00;
        usb_buffer_out[i]=0x00;
    }
    i=0;

    usb_buffer_out[0] = WAKEUP;// Affect request to  WakeUP Command
    libusb_bulk_transfer(handle, 0x01,usb_buffer_out, sizeof(usb_buffer_out), &numBytes, 0); // Send Packets to Sega Dumper
    libusb_bulk_transfer(handle, 0x82, usb_buffer_in, sizeof(usb_buffer_in), &numBytes, 0);
    SDL_Log("\n");
    SDL_Log("MD Dumper %.*s",6, (char *)usb_buffer_in);
    SDL_Log("\n");
    md_dumper_type = usb_buffer_in[24];
    if ( md_dumper_type == 0 )
    {
        SDL_Log("MD Dumper type : Old Version \n");
    }
    if ( md_dumper_type == 1 )
    {
        SDL_Log("MD Dumper type : BluePill non aligned \n");
    }
    if ( md_dumper_type == 2 )
    {
        SDL_Log("MD Dumper type : BluePill aligned \n");
    }
    if ( md_dumper_type == 3 )
    {
        SDL_Log("MD Dumper type : SMD ARM TQFP100 Aligned  \n");
    }
    if ( md_dumper_type == 4 )
    {
        SDL_Log("MD Dumper type : Marv17 aligned tqfp44  \n");
    }
    SDL_Log("Hardware Firmware version : %d.%d\n", usb_buffer_in[20],usb_buffer_in[21]);
return 0;
}

int Open_CSV_Files(void)
{
if (csv_init(&p, options) != 0)
    {
        SDL_Log("\n");
        SDL_Log("\n");
        SDL_Log("ERROR Failed to init CSV Parser for Gamelist ...\n");
        exit(EXIT_FAILURE);
    }
    csv_set_quote(&p,';');

    FILE *fp = fopen("gameslist.csv", "r");
    if (!fp)
    {
        SDL_Log("\n");
        SDL_Log("\n");
        SDL_Log("ERROR Can't find gamelist.csv ...\n");
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
    {
        if (csv_parse(&p, buffer, bytes_read, cb1, cb2, NULL) != bytes_read)
        {
            buffer_header[i]=0x00;
            SDL_Log("\n");
            SDL_Log("\n");
            SDL_Log("ERROR while parsing file ...\n");
            return EXIT_FAILURE;
        }
    }
    i = 0;

    csv_fini(&p, cb1, cb2, NULL);
    csv_free(&p);
    fclose(fp);

    SDL_Log("\n");
    SDL_Log("CSV Gamelist file opened sucessfully\n");
    //Afficher le nombre de cellules non vides en colonne A
    SDL_Log("Add : %d Special Games into MD Dumper Database \n", non_empty_cells_in_col_A);

    // open csv flash list File

    if (csv_init(&p2, options) != 0)
    {
        SDL_Log("\n");
        SDL_Log("\n");
        SDL_Log(" ERROR Failed to init CSV Parser for Flashlist ...\n");
        exit(EXIT_FAILURE);
    }
    csv_set_quote(&p2,';');


    FILE *fp2 = fopen("flashlist.csv", "r");
    if (!fp)
    {
        SDL_Log("\n");
        SDL_Log("\n");
        SDL_Log(" ERROR Can't find flashlist.csv ...\n");
        return EXIT_FAILURE;
    }

    char buffer2[BUFFER_SIZE];
    size_t bytes_read2;
    while ((bytes_read2 = fread(buffer2, 1, BUFFER_SIZE, fp)) > 0)
    {
        if (csv_parse(&p2, buffer2, bytes_read2, cb3, cb4, NULL) != bytes_read2)
        {
            SDL_Log("\n");
            SDL_Log("\n");
            SDL_Log(" ERROR while parsing file ...\n");
            return EXIT_FAILURE;
        }
    }

    csv_fini(&p, cb1, cb2, NULL);
    csv_free(&p);
    fclose(fp);

    SDL_Log("CSV Flashlist file opened sucessfully\n");
    // Afficher le nombre de cellules non vides en colonne A
    SDL_Log("Add : %d Flash ID into MD Dumper Database \n", non_empty_cells_in_col_A2);
    return 0;
}

void Game_Header_Infos(void)
{
//First try to read ROM MD Header

    buffer_header = (unsigned char *)malloc(0x200);
    i = 0;
    address = 0x80;

    // Cleaning header Buffer
    for (i=0; i<512; i++)
    {
        buffer_header[i]=0x00;
    }

    i = 0;

    while (i<8)
    {
        usb_buffer_out[0] = READ_MD;
        usb_buffer_out[1] = address&0xFF ;
        usb_buffer_out[2] = (address&0xFF00)>>8;
        usb_buffer_out[3]=(address & 0xFF0000)>>16;
        usb_buffer_out[4] = 0; // Slow Mode

        libusb_bulk_transfer(handle, 0x01,usb_buffer_out, sizeof(usb_buffer_out), &numBytes, 60000);
        libusb_bulk_transfer(handle, 0x82,buffer_header+(64*i),64, &numBytes, 60000);
        address+=32;
        i++;
    }

    if(memcmp((unsigned char *)buffer_header,"SEGA",4) == 0)
    {
        SDL_Log("\n");
        SDL_Log("Megadrive/Genesis/32X cartridge detected!\n");
        SDL_Log("\n");
        SDL_Log(" --- HEADER ---\n");
        memcpy((unsigned char *)dump_name, (unsigned char *)buffer_header+32, 48);
        trim((unsigned char *)dump_name, 0);
        SDL_Log(" Domestic: %.*s\n", 48, (char *)game_name);
        memcpy((unsigned char *)dump_name, (unsigned char *)buffer_header+80, 48);
        trim((unsigned char *)dump_name, 0);

        SDL_Log(" International: %.*s\n", 48, game_name);
        SDL_Log(" Release date: %.*s\n", 16, buffer_header+0x10);
        SDL_Log(" Version: %.*s\n", 14, buffer_header+0x80);
        memcpy((unsigned char *)region, (unsigned char *)buffer_header +0xF0, 4);
        for(i=0; i<4; i++)
        {
            if(region[i]==0x20)
            {
                game_region = (char *)malloc(i);
                memcpy((unsigned char *)game_region, (unsigned char *)buffer_header +0xF0, i);
                game_region[i] = '\0';
                break;
            }
        }

        if(game_region[0]=='0')
        {
            game_region = (char *)malloc(4);
            memcpy((char *)game_region, (char *)unk, 3);
            game_region[3] = '\0';
        }

        SDL_Log(" Region: %s\n", game_region);

        checksum_header = (buffer_header[0x8E]<<8) | buffer_header[0x8F];
        SDL_Log(" Checksum: %X\n", checksum_header);

        game_size = 1 + ((buffer_header[0xA4]<<24) | (buffer_header[0xA5]<<16) | (buffer_header[0xA6]<<8) | buffer_header[0xA7])/1024;
        SDL_Log(" Game size: %dKB\n", game_size);

        if((buffer_header[0xB0] + buffer_header[0xB1])!=0x93)
        {
            SDL_Log(" Extra Memory : No\n");
        }
        else
        {
            SDL_Log(" Extra Memory : Yes ");
            switch(buffer_header[0xB2])
            {
            case 0xF0:
                SDL_Log(" 8bit backup SRAM (even addressing)\n");
                break;
            case 0xF8:
                SDL_Log(" 8bit backup SRAM (odd addressing)\n");
                break;
            case 0xB8:
                SDL_Log(" 8bit volatile SRAM (odd addressing)\n");
                break;
            case 0xB0:
                SDL_Log(" 8bit volatile SRAM (even addressing)\n");
                break;
            case 0xE0:
                SDL_Log(" 16bit backup SRAM\n");
                break;
            case 0xA0:
                SDL_Log(" 16bit volatile SRAM\n");
                break;
            case 0xE8:
                SDL_Log(" Serial EEPROM\n");
                break;
            }
            if ( buffer_header[0xB2] != 0xE0 | buffer_header[0xB2] != 0xA0 ) // 8 bit SRAM
            {
                save_size2 = (buffer_header[0xB8]<<24) | (buffer_header[0xB9]<<16) | (buffer_header[0xBA] << 8) | buffer_header[0xBB];
                save_size1 = (buffer_header[0xB4]<<24) | (buffer_header[0xB5]<<16) | (buffer_header[0xB6] << 8) | buffer_header[0xB7];

                save_size = save_size2 - save_size1;
                save_size = (save_size/1024); // Kb format
                save_size=(save_size/2) + 1; // 8bit size
            }
            save_address = (buffer_header[0xB4]<<24) | (buffer_header[0xB5]<<16) | (buffer_header[0xB6] << 8) | buffer_header[0xB7];
            SDL_Log(" Save size: %dKb\n", save_size);
            SDL_Log(" Save address: %lX\n", save_address);

            if(usb_buffer_in[0xB2]==0xE8) // EEPROM Game
            {
                SDL_Log(" No information on this game!\n");
            }
        }
    }
}