#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>

#define hash_init 5381

uint32_t hash_djb2(const uint8_t * str, uint32_t hash) 
{
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) ^ c;

    return hash;
}

void usage(const char * binname) 
{
    printf("Usage: %s [-d <dir>] [outfile]\n", binname);
    exit(-1);
}

void processdir(DIR * dirp, const char * curpath, FILE * outfile, const char * prefix) 
{
    char fullpath[1024];
    char buf[16 * 1024]; // the file should be less than 16KByte

    struct dirent * ent;
    DIR * rec_dirp;

    uint32_t cur_hash = hash_djb2((const uint8_t *) curpath, hash_init);
    uint32_t size, w, hash, sl;
    uint8_t b;

    FILE * infile;

    while ((ent = readdir(dirp))) 
    {
        strcpy(fullpath, prefix);
        strcat(fullpath, "/");
        strcat(fullpath, curpath);
        strcat(fullpath, ent->d_name);//!!file name, that's the key!!
    
    #ifdef _WIN32
        if (GetFileAttributes(fullpath) & FILE_ATTRIBUTE_DIRECTORY) 
        {
    #else
        if (ent->d_type == DT_DIR) 
        {
    #endif
            if (strcmp(ent->d_name, ".") == 0)
                continue;
            if (strcmp(ent->d_name, "..") == 0)
                continue;
        
            strcat(fullpath, "/");
            rec_dirp = opendir(fullpath);
            processdir(rec_dirp, fullpath + strlen(prefix) + 1, outfile, prefix);
            closedir(rec_dirp);
        } 
        else 
        {
            hash = hash_djb2((const uint8_t *) ent->d_name, cur_hash);
            infile = fopen(fullpath, "rb");
            
            sl = strlen(ent->d_name);// get the length of that file name
            printf("deds1014 testing: %s, %s, %d, %c\n", fullpath, ent->d_name, sl, (char)sl);//test
        
            if (!infile) 
            {
                perror("opening input file");
                exit(-1);
            }
            b = (hash >>  0) & 0xff; fwrite(&b, 1, 1, outfile);
            b = (hash >>  8) & 0xff; fwrite(&b, 1, 1, outfile);
            b = (hash >> 16) & 0xff; fwrite(&b, 1, 1, outfile);
            b = (hash >> 24) & 0xff; fwrite(&b, 1, 1, outfile);
            
            fseek(infile, 0, SEEK_END);
            size = ftell(infile);//caution! , non-portable
            fseek(infile, 0, SEEK_SET);
            
            b = ((size + 14) >>  0) & 0xff; fwrite(&b, 1, 1, outfile); // section of magic
            b = ((size + 14) >>  8) & 0xff; fwrite(&b, 1, 1, outfile);
            b = ((size + 14) >> 16) & 0xff; fwrite(&b, 1, 1, outfile); 
            b = ((size + 14) >> 24) & 0xff; fwrite(&b, 1, 1, outfile);
            
            //fwrite(ent->d_name, 1, sl, outfile); //add the file name section

            while (size) //why while?
            {
                w = size > 16 * 1024 ? 16 * 1024 : size;//only store 16KB data
                fread(buf, 1, w, infile);
                fwrite(buf, 1, w, outfile);
                size -= w;
            }
            //buf[w] = (char)sl;//this will cause memory mapping error. I want to add the length of file name at the end, but the output will be weird.
            //printf ("%c\n", (unsigned char)sl); // testing
            //ent->d_name[sl] = (char)sl; // add the length of file name at the end of this memory section
            fwrite(ent->d_name, 1, 14, outfile);//write file name

            printf("fs end loop\n");
            fclose(infile);
        }
    }
}

int main(int argc, char ** argv) 
{
    char * binname = *argv++;
    char * o;
    char * outname = NULL;
    char * dirname = ".";
    uint64_t z = 0;
    FILE * outfile;
    DIR * dirp;

    while ((o = *argv++)) 
    {
        if (*o == '-') 
        {
            o++;
            switch (*o) 
            {
            case 'd':
                dirname = *argv++;
                break;
            default:
                usage(binname);
                break;
            }
        } 
        else 
        {
            if (outname)
                usage(binname);
            outname = o;
        }
    }

    if (!outname)
        outfile = stdout;
    else
        outfile = fopen(outname, "wb");

    if (!outfile) 
    {
        perror("opening output file");
        exit(-1);
    }

    dirp = opendir(dirname);
    
    if (!dirp) 
    {
        perror("opening directory");
        exit(-1);
    }

    processdir(dirp, "", outfile, dirname);
    fwrite(&z, 1, 8, outfile);
    
    if (outname)
        fclose(outfile);
    closedir(dirp);
    
    return 0;
}
