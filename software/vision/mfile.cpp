#include "mgui.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

//#define FILE_DEBUG
#ifdef FILE_DEBUG
    #define DEBUG_PRINT(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(format, ...)
#endif

bool str_isdigit (const char str[])
// Helper function for read_mv_setting. Returns true if string is a float 
{
    unsigned len = strlen (str);
    for (unsigned i = 0; i < len; i++)
        if ((str[i] < '0' || str[i] > '9') && str[i] != '.') 
            return false;
        
    return true;
}

std::string settings_filename(const char *filename)
{
    std::string settings_file;
    char * env_mda_settings_dir = getenv(MDA_SETTINGS_DIR_ENV_VAR);
    if (env_mda_settings_dir != NULL)
        settings_file = env_mda_settings_dir;
    else
        settings_file = MDA_BACKUP_SETTINGS_DIR;
    
    settings_file += filename;
    return settings_file;
}

template <typename TYPE>
void read_mv_setting (const char filename[], const char setting_name[], TYPE &data) 
/* Reads a single setting from a file. searches through file for the setting
   and puts it into &data. Returns 0 iff successful. Settings file must have
   lines consisting of either comments (beings with '#') or in the format
   SETTING_NAME, SETTING_VALUE */

{
    std::string settings_file = settings_filename(filename);

    FILE* fh = fopen (settings_file.c_str(), "r");
    if (fh == NULL) {
        fprintf (stderr, "**** Error: read_mv_setting failed to open %s\n", settings_file.c_str()); 
        exit (1);
    }

    DEBUG_PRINT ("Reading Settings File: %s\n", filename);    
    char line[LINE_LEN+1];
    char* token;
    
    // read the file line by line
    while (fgets(line, LINE_LEN, fh)) {
        // extract first token and match to setting_name
        token = strtok (line, " ,");
        if (token[0] == COMMENT_CHAR) 
            continue;

        if (!strcmp(token, setting_name)) { // found the right line
            token = strtok (NULL, " ,\n"); // get next token           
            if (!str_isdigit(token)) {
                fprintf (stderr, "**** ERROR: read_mv_setting: invalid value for %s\n", setting_name);
                exit (1);
            }
            
            double temp = atof (token);
            data = TYPE(temp);
            fclose (fh);
            return;
        }
    }
    
    fprintf (stderr, "**** ERROR: read_mv_setting: setting %s not found in file %s\n", setting_name, settings_file.c_str());
    exit (1);
}
/* these lines declare the use of bool,int,unsigned,float of read_mv_setting */
template void read_mv_setting<bool>(const char filename[], const char setting_name[], bool &data);
template void read_mv_setting<int>(const char filename[], const char setting_name[], int &data);
template void read_mv_setting<unsigned>(const char filename[], const char setting_name[], unsigned &data);
template void read_mv_setting<unsigned char>(const char filename[], const char setting_name[], unsigned char &data);
template void read_mv_setting<float>(const char filename[], const char setting_name[], float &data);
template void read_mv_setting<double>(const char filename[], const char setting_name[], double &data);

/* overloaded for string type */
void read_mv_setting(const char filename[], const char setting_name[], std::string &data)
{
    std::string settings_file = settings_filename(filename);

    FILE* fh = fopen (settings_file.c_str(), "r");
    if (fh == NULL) {
        fprintf (stderr, "**** Error: read_mv_setting failed to open %s\n", settings_file.c_str()); 
        exit (1);
    }
    
    DEBUG_PRINT ("Reading Settings File: %s\n", filename);
    char line[LINE_LEN+1];
    char* token;
    
    // read the file line by line
    while (fgets(line, LINE_LEN, fh)) {
        // extract first token and match to setting_name
        token = strtok (line, " ,");
        if (token[0] == COMMENT_CHAR) 
            continue;

        if (!strcmp(token, setting_name)) { // found the right line
            data = strtok (NULL, " ,\n"); // get next token           
            fclose (fh);
            return;
        }
    }
    
    fprintf (stderr, "**** ERROR: read_mv_setting: setting %s not found in file %s\n", setting_name, settings_file.c_str());
    exit (1);
}
