/*
  Implementation of the Yahoo! Messenger archive decoder.
  Copyright (C) 2008  Cristi Balas

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  http://www.gnu.org/licenses
  
  OS: Microsoft Windows
  Site: http://CristiBalas.wordpress.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>  // get directory contents
#include <dir.h>     // mkdir, chdir
#define ID_MAXLEN 255
#define DIR_MAXLEN 512
#define BUF_MAXLEN 1024

/*************************
 ******** Decoder ********
 *************************/

/* returns the length of a formatting token. 0 if no token here */
int token_length(const char *str) {
    int l;
    switch(*str) {
    case 0x1B:
        if(str[2] == '1'  // Bold
        || str[2] == '2'  // Italic
        || str[2] == '4'  // Underline
        || str[2] == 'l') // Link
            return 4;
        if(str[2] == '3' || str[2] == 'x') // predefined color or cancel Bold/Italic/Underline
            return 5;
        if(str[2] == '#') // custom color
            return 10;
    case '<':
        if(!strncmp(str, "<font ", 6)
        || !strncmp(str, "</font", 6)
        || !strncmp(str, "<ALT ", 5)
        || !strncmp(str, "</ALT", 5)
        || !strncmp(str, "<FADE ", 6)
        || !strncmp(str, "</FADE", 6)) {
            for(l = 0; str[l] && str[l] != '>'; l++)
                ; // nothing, just go to the '>'
            return l + 1;
        }
    }
    return 0;
}

int ym_decode_file(const char *id, const char *other_id, const char *strFin, const char *strFout) {
    int id_len = strlen(id);
    FILE *fin, *fout;
    time_t timestamp;
    int i, tmp;
    int msg_direction; // 0 = out, nonzero = in
    
    char cmsg[BUF_MAXLEN] = { 0 }; // coded message
    unsigned int cmsg_len;
    
    char msg[BUF_MAXLEN] = { 0 }; // decoded message
    unsigned int msg_len;

    /* Open files */
    if(!(fin = fopen(strFin, "rb"))) {
        fprintf(stderr, "Can't open input file '%s'\n", strFin);
        return 1;
    }
    if(!(fout = fopen(strFout, "wt"))) {
        fprintf(stderr, "Can't open output file '%s'\n", strFout);
        fclose(fin);
        return 1;        
    }
    
    /* Go */
    while( fread(&timestamp    , 4       , 1, fin)
        && fread(&tmp          , 4       , 1, fin)
        && fread(&msg_direction, 4       , 1, fin)
        && fread(&cmsg_len     , 4       , 1, fin)
        && (cmsg_len < BUF_MAXLEN)
        &&(fread(cmsg          , cmsg_len, 1, fin) || cmsg_len == 0)
        && fread(&tmp          , 4       , 1, fin))
    {
        if(cmsg_len == 0) // *almost* every time, first message is null
            continue;

        /* add time stamp */
        msg_len = strftime(msg, BUF_MAXLEN, "(%Y-%m-%d %H:%M:%S) ", localtime(&timestamp));
        /* add author */
        msg_len += sprintf(msg + msg_len, "%s: ", msg_direction ? other_id : id);
        /* add decoded message */
        for(i = 0; i < cmsg_len && msg_len < BUF_MAXLEN - 1; i++) {
            msg[msg_len] = cmsg[i] ^ id[i % id_len];
            msg_len++;
        }
        msg[msg_len] = 0;

        /* Remove formatting tokens */
        for(msg_len = 0, i = 0; msg[i]; i++) {
            tmp = token_length(msg + i);
            if(tmp) {
                i += tmp - 1;
            } else {
                msg[msg_len] = msg[i];
                msg_len++;
            }
        }
        msg[msg_len] = 0;

        /* save */
        fprintf(fout, "%s\n", msg);
    }
    fclose(fout);
    if(!feof(fin)) {
        fclose(fin);
        return 1;
    } else {
        fclose(fin);
        return 0;
    }
}


/******************************
 ******** Main program ********
 ******************************/

const char *SZPNAME = "ym_decoder";

void die(char *msg, int code) {
    printf("%s", msg);
    system("PAUSE");
    exit(code);
}

void process_dir(const char *id, const char *buddy_id, const char *source_dir, const char *target_dir)
{
    DIR *dir_enc; // encoded DIR
    struct dirent* dent;
    char *fname;
    int y, m, d;
    char fpath[DIR_MAXLEN];
    char dfpath[DIR_MAXLEN];
    
    printf("%s ", buddy_id);
    
    mkdir(target_dir);
    if(chdir(target_dir)) {
        printf("[Can't make target dir]\n");
        return;
    }
    
    dir_enc = opendir(source_dir);
    while(dent = readdir(dir_enc)) {
        fname = dent->d_name;
        if(!strcmp(fname, ".") || !strcmp(fname, ".."))
            continue;
        if(strlen(fname) <= 10 || strcmp(fname + strlen(fname) - 4, ".dat"))
            continue;
        sprintf(fpath, "%s\\%s", source_dir, fname);
        sscanf(fname, "%4d%2d%2d", &y, &m, &d);
        sprintf(dfpath, "%s\\%04d-%02d-%02d.txt", target_dir, y, m, d);
        if(ym_decode_file(id, buddy_id, fpath, dfpath)) {
            printf("!");
        } else {
            printf("*");
        }
        fflush(stdout);
    }
    closedir(dir_enc);
    printf("\n");
}

int main(int argc, char *argv[])
{
    char id[ID_MAXLEN];
    char archive_dir[DIR_MAXLEN]; //  \Archive\Messages
    char decode_dir[DIR_MAXLEN]; //   \Decoded_Archive
    char buddy_dir[DIR_MAXLEN]; //    \Archive\Messages\buddy_id
    char d_buddy_dir[DIR_MAXLEN]; //  \Decoded_Archive\buddy_id

    DIR *dir_Messages;
    struct dirent* dent;
    
    
    char *p_tmp;
    /* Verify command line parameter */
    if(argc != 2) {
        printf("Usage: %s <user_profile_dir>\n\nExample: %s \"C:\\Program Files\\Yahoo!\\Messenger\\Profiles\\pufuletz_roz\"\n", SZPNAME, SZPNAME);
        die("", 0);
    }
    if(chdir(argv[1]))
        die("Invalid directory!\n", 1);

    /*** Set global parameters ***/
    sprintf(archive_dir, "%s\\Archive\\Messages", argv[1]);
    if(chdir(archive_dir))
        die("Can't find Archive\\Messages subdirectory!\n", 1);
    else
        printf("Archive directory is '%s'\n", archive_dir);

    sprintf(decode_dir, "%s\\Decoded_Archive", argv[1]);
    mkdir(decode_dir);
    if(chdir(decode_dir)) {
        printf("Error: Can't create directory '%s'\n", decode_dir);
        die("", 1);
    } else
        printf("\n*** Decoding in folder '%s'\n\n", decode_dir);

    p_tmp = argv[1] + strlen(argv[1]);
    while( p_tmp != argv[1]  &&  *(p_tmp - 1) != '\\')
        p_tmp--;
    strcpy(id, p_tmp);
    printf("MessengerID is '%s'\n", id);
    
    dir_Messages = opendir(archive_dir);
    while(dent = readdir(dir_Messages)) { // while
        if(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
            continue;
        sprintf(buddy_dir, "%s\\%s", archive_dir, dent->d_name);
        if(chdir(buddy_dir)) {
            printf("%s [CAN'T CHANGE DIR]\n", dent->d_name);
        } else {
            sprintf(d_buddy_dir, "%s\\%s", decode_dir, dent->d_name);
            process_dir(id, dent->d_name, buddy_dir, d_buddy_dir);
        }
    }
    closedir(dir_Messages);
    
    printf("\n\n");
    system("PAUSE");	
    return 0;
}
