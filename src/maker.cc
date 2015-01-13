//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

/*
 * WARNING: Extensions are removed from files using hard-coded offsets.
 */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

#ifndef EXTRA_MAKE_OPTION
#define EXTRA_MAKE_OPTION  ""
// #define EXTRA_MAKE_OPTION  "-j 20"
#endif

struct FilesToBuildFrom {

    char **fName;
    int numNames;
    char target[256];

    FilesToBuildFrom ();
    ~FilesToBuildFrom ();
    void PrintDirs (FILE *toMe);
    void PrintObjs (FILE *toMe);
    void PrintLibs (FILE *toMe);

};

void FilesToBuildFrom :: PrintDirs (FILE *toMe) {

    for (int i = 0; i < numNames; i++) {

        if (strlen(fName[i]) < 2) {
            fprintf(toMe, "%s\n", fName[i]);
            continue;
        }

        if ((fName[i][strlen(fName[i]) - 1] != 'o' || fName[i][strlen(fName[i]) - 2] != '.') &&
                (fName[i][0] != '-' /*Alin: || fName[i][1] != 'l' */))
            fprintf(toMe, "%s\n", fName[i]);
    }
}

void FilesToBuildFrom :: PrintObjs (FILE *toMe) {

    for (int i = 0; i < numNames; i++) {

        if (strlen(fName[i]) < 2)
            continue;

        if (fName[i][strlen(fName[i]) - 1] == 'o' && fName[i][strlen(fName[i]) - 2] == '.')
            fprintf(toMe, " %s", fName[i]);
    }
}

void FilesToBuildFrom :: PrintLibs (FILE *toMe) {

    for (int i = 0; i < numNames; i++) {

        if (strlen(fName[i]) < 2)
            continue;

        if (fName[i][0] == '-' /**Alin: && fName[i][1] == 'l'*/)
            fprintf(toMe, " %s", fName[i]);
    }
}

FilesToBuildFrom *ReadExecsFromFile (FILE *fromMe, int &curPosInOutput) {

    char buffer[1000];
    FilesToBuildFrom *output = new FilesToBuildFrom[256];

    // this is the return value
    curPosInOutput = -1;

    while (fscanf(fromMe, "%s", buffer) != EOF) {

        // see if we got a new target
        if (buffer[strlen(buffer) - 1] == ':') {
            buffer[strlen(buffer) - 1] = 0;
            curPosInOutput++;
            if (curPosInOutput == 256) {
                cerr << "Error: max 256 targets reached." << endl;
                exit(1);
            }
            strcpy (output[curPosInOutput].target, buffer);

        } else {
            output[curPosInOutput].fName[output[curPosInOutput].numNames] = strdup(buffer);
            output[curPosInOutput].numNames++;
            if (output[curPosInOutput].numNames == 256) {
                cerr << "Error: max 256 directories for target reached." << endl;
                exit(1);
            }

        }
    }

    curPosInOutput++;
    return output;
}

FilesToBuildFrom :: FilesToBuildFrom () {
    fName = new char *[1000];
    numNames = 0;
}

FilesToBuildFrom :: ~FilesToBuildFrom () {

    for (int i = 0; i < numNames; i++) {
        free (fName[i]);
    }
    delete [] fName;
}

char *replace(char *input, const char *findMe, const char *useMeInstead) {
    char buffer[4096];
    char *p;

    if(!(p = strstr(input, findMe)))  // Is 'orig' even in 'str'?
        return strdup (input);

    strncpy(buffer, input, p - input); // Copy characters from 'str' start to 'orig'
    buffer[p - input] = 0;

    sprintf(buffer + (p - input), "%s%s", useMeInstead, p + strlen(findMe));

    return strdup(buffer);
}

void WriteExecs (FILE *output, FilesToBuildFrom *execList, int listLen) {
    for (int i = 0; i < listLen; i++) {
        fprintf (output, "%s ", execList[i].target);
        int j;
        for (j = strlen (execList[i].target); j >= 0; j--) {
            if (execList[i].target[j] == '/')
                break;
        }
        j++;
        fprintf (output, "Headers%s ", execList[i].target + j);
    }
}

// this writes the names of all of the directories in execList to the file called outFileName
void WriteAll (FilesToBuildFrom *execList, int listLen, const char *outFileName) {

    FILE *output = fopen (outFileName, "w");
    fprintf (output, "# if you just type `make' by default it will build everything.\n");
    fprintf (output, "all: ");
    for (int i = 0; i < listLen; i++) {
        fprintf (output, " %s", execList[i].target);
    }
    fprintf (output, "\n\n");
    for (int i = 0; i < listLen; i++) {
        for (int j = strlen(execList[i].target); j >= 0; j--) {
            if (execList[i].target[j] == '/') {
                fprintf (output, "%s: %s\n\n", &(execList[i].target[j + 1]), execList[i].target);
                break;
            }
        }
    }
    fclose (output);
}

// this writes the names of all of the directories in execList to the file called outFileName
void WriteAllDirectoriesToFile (FilesToBuildFrom *execList, int listLen, const char *outFileName) {

    FILE *output = fopen (outFileName, "w");
    for (int i = 0; i < listLen; i++) {
        execList[i].PrintDirs (output);
    }
    fclose (output);

}

// removes all duplicate lines in the file
void RemDups (const char *inFileName, const char *outFileNAme) {
    char buffer[1000];
    sprintf (buffer, "sort -o %s -u %s", outFileNAme, inFileName);
    system (buffer);
}

// this gets one line from a file
int GetLine (char *inMe, FILE *fromMe) {

    for (int i = 0; 1; i++) {
        if (i >= 512) {
            cerr << "Weird.  I read more than 512 bytes on one line of a text file." << endl;
            exit(1);
        }
        inMe[i] = getc (fromMe);
        if (inMe[i] == '\n') {
            inMe[i] = 0;
            return 1;
        }
        else if (inMe[i] == EOF) {
            inMe[i] = 0;
            return 0;
        }
    }

}

int Cmpstring (const void *p1, const void *p2) {
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}

// this goes through the input file.  For every line, it counts the number of slashes.  It finds the
// nth one from the end, and strips off everything after that.
void Strip (const char *inputFile, int n, const char *outputFile) {

    FILE *inFile = fopen (inputFile, "r");
    FILE *outFile = fopen (outputFile, "w");
    char buffer[512];
    while (GetLine (buffer, inFile)) {
        int i = 0, slashCount = 0;
        for (; buffer[i] != 0; i++) {
            if (buffer[i] == '/')
                slashCount++;
        }
        slashCount -= n;
        for (i = 0; buffer[i] != 0 && slashCount != 0; i++) {
            if (buffer[i] == '/')
                slashCount--;
        }
        buffer[i] = 0;
        fprintf (outFile, "%s\n", buffer);
    }
    fclose (inFile);
    fclose (outFile);
}

// this sorts all of the lines in the file.  I don't use the Unix sort because
// it uses a DIFFERENT sort order than strcmp does!
void SortFile (const char *inputFile, const char *outputFile) {

    // first go thru and count the lines in the file
    int numLines = 0;
    FILE *inFile = fopen (inputFile, "r");
    char buffer[512];
    while (GetLine (buffer, inFile))
        numLines++;
    fclose (inFile);

    // now, we load it up again...
    char **allLines = new char *[numLines];
    FILE *inFile2 = fopen (inputFile, "r");
    for (int i = 0; i < numLines; i++) {
        GetLine (buffer, inFile2);
        allLines[i] = strdup (buffer);
    }
    fclose (inFile2);

    // sort it...
    qsort (allLines, numLines, sizeof(char *), Cmpstring);

    // write it out
    FILE *outFile = fopen (outputFile, "w");
    for (int i = 0; i < numLines; i++) {
        fprintf (outFile, "%s\n", allLines[i]);
        free (allLines[i]);
    }
    delete [] allLines;
    fclose (outFile);


}

// this creates an output file that has everything in leftInput where some prefix does NOT appear in
// rightInput.
void Antijoin (const char *leftInput, const char *rightInput, const char *outputFile) {

    // sort the two files
    SortFile (leftInput, leftInput);
    SortFile (rightInput, rightInput);

    // now do a merge
    FILE *linput = fopen(leftInput, "r");
    FILE *rinput = fopen(rightInput, "r");
    FILE *output = fopen(outputFile, "w");

    char leftBuffer[1000];
    char rightBuffer[1000];

    // prime the two buffers
    if (fscanf (linput, "%s", leftBuffer) == EOF ||
            fscanf (rinput, "%s", rightBuffer) == EOF) {
        fclose (linput);
        fclose (rinput);
        fclose (output);
        char buffer[1000];
        sprintf(buffer, "cp %s %s", leftInput, outputFile);
        system (buffer);
        return;
    }

    while (1) {

        // if they are equal, write out the LHS
        int lengthL = strlen(leftBuffer);
        int lengthR = strlen(rightBuffer);
        int min  = lengthL;
        if (lengthR < min)
            min = lengthR;
        int result = strncmp(leftBuffer, rightBuffer, min);

        // got a match! so output it, and move to next guy in left...
        if (result == 0) {
            if (fscanf (linput, "%s", leftBuffer) == EOF)
                break;

        } else if (result < 0) {
            fprintf (output, "%s\n", leftBuffer);
            if (fscanf (linput, "%s", leftBuffer) == EOF)
                break;
        } else {
            if (fscanf (rinput, "%s", rightBuffer) == EOF) {
                fprintf (output, "%s\n", leftBuffer);
                while (fscanf (linput, "%s", leftBuffer) != EOF) {
                    fprintf (output, "%s\n", leftBuffer);
                }
                break;
            }
        }
    }

    fclose (linput);
    fclose (rinput);
    fclose (output);
    return;
}


// this creates an output file that has everything in leftInput where some prefix also appears in
// rightInput.
void Semijoin (const char *leftInput, const char *rightInput, const char *outputFile) {

    // sort the two files
    SortFile (leftInput, leftInput);
    SortFile (rightInput, rightInput);

    // now do a merge
    FILE *linput = fopen(leftInput, "r");
    FILE *rinput = fopen(rightInput, "r");
    FILE *output = fopen(outputFile, "w");

    char leftBuffer[1000];
    char rightBuffer[1000];

    // prime the two buffers
    if (fscanf (linput, "%s", leftBuffer) == EOF ||
            fscanf (rinput, "%s", rightBuffer) == EOF) {
        fclose (linput);
        fclose (rinput);
        fclose (output);
        return;
    }

    while (1) {

        // if they are equal, write out the LHS
        int lengthL = strlen(leftBuffer);
        int lengthR = strlen(rightBuffer);
        int min  = lengthL;
        if (lengthR < min)
            min = lengthR;
        int result = strncmp(leftBuffer, rightBuffer, min);

        // got a match! so output it, and move to next guy in left...
        if (result == 0) {
            fprintf (output, "%s\n", leftBuffer);
            if (fscanf (linput, "%s", leftBuffer) == EOF)
                break;

        } else if (result < 0) {
            if (fscanf (linput, "%s", leftBuffer) == EOF)
                break;
        } else {
            if (fscanf (rinput, "%s", rightBuffer) == EOF)
                break;
        }
    }

    fclose (linput);
    fclose (rinput);
    fclose (output);
    return;
}

void BuildMakefile (const char *makefileName, const char *prelude, const char *headerIncludeDirectories,
        const char *phpIncludeDirectories, const char *phpFilesForHeaders, const char *phpFilesForCPlusPlus,
        const char *filesToBuildDependenciesFor, const char *dependencyFile, const char *rulesForObjectFiles,
        int numProjects, FilesToBuildFrom *myProjects) {

    char buffer[256];
    sprintf (buffer, "rm %s 2> /dev/null", makefileName);
    system (buffer);

    FILE *suffix = fopen("suffix", "w");

    // add all of the header include directories to the 2nd half of the makefile
    if (headerIncludeDirectories != 0) {
        fprintf (suffix, "# here we add additional include directories\n");
        FILE *temp = fopen(headerIncludeDirectories, "r");
        char buffer[512];
        while (GetLine (buffer, temp)) {
            fprintf (suffix, "INCLUDE += -I %s\n", buffer);
        }
        fprintf (suffix, "\n\n");
        fclose (temp);
    }

    /*
    // add all of the php include directories to the 2nd half of the makefile
    if (phpIncludeDirectories != 0) {
    fprintf (suffix, "# here we add additional php include directories\n");
    FILE *temp = fopen(phpIncludeDirectories, "r");
    char buffer[512];
    while (GetLine (buffer, temp)) {
    fprintf (suffix, "PHPFLAGS += -I %s\n", buffer);
    }
    fprintf (suffix, "\n\n");
    fclose (temp);
    }
    */


    // add rules to make all of the .h files from .h.php files
    if (phpFilesForHeaders != 0) {
        fprintf (suffix, "# these are rules for compiling .h.php files\n");
        FILE *temp = fopen(phpFilesForHeaders, "r");
        char buffer[512];
        while (GetLine (buffer, temp)) {
            char *target = strdup (buffer);
            target[strlen(target) - 4] = 0;
            char *temp = replace (target, "/php/private/", "/headers/");
            free (target);
            target = replace (temp, "/php/", "/headers/");
            free (temp);
            fprintf (suffix, "%s: %s\n\t$(PHP) $(PHPFLAGS) %s > %s\n\n", target, buffer, buffer, target);
            free (target);
        }
        fclose (temp);
        temp = fopen(phpFilesForHeaders, "r");
        fprintf (suffix, "# this will compile all of the .h.php files\nallHFromPHP:");
        while (GetLine (buffer, temp)) {
            char *target = strdup (buffer);
            target[strlen(target) - 4] = 0;
            char *temp = replace (target, "/php/private/", "/headers/");
            free (target);
            target = replace (temp, "/php/", "/headers/");
            free (temp);
            fprintf (suffix, " %s", target);
            free (target);
        }
        fclose (temp);
        fprintf (suffix, "\n\n");
    }


    // add rules to make all of the .h files from .h.php files
    if (phpFilesForCPlusPlus != 0) {
        fprintf (suffix, "# these are rules for compiling .h.cc files\n");
        FILE *temp = fopen(phpFilesForCPlusPlus, "r");
        char buffer[512];
        while (GetLine (buffer, temp)) {
            char *target = strdup (buffer);
            target[strlen(target) - 4] = 0;
            char *temp = replace (target, "/php/private/", "/source/");
            free (target);
            target = replace (temp, "/php/", "/source/");
            free (temp);
            fprintf (suffix, "%s: %s\n\t$(PHP) $(PHPFLAGS) %s > %s\n\n", target, buffer, buffer, target);
            free (target);
        }
        fclose (temp);
        temp = fopen(phpFilesForCPlusPlus, "r");
        fprintf (suffix, "# this will compile all of the .cc.php files\nallCPlusPlusFromPHP:");
        while (GetLine (buffer, temp)) {
            char *target = strdup (buffer);
            target[strlen(target) - 4] = 0;
            char *temp = replace (target, "/php/private/", "/source/");
            free (target);
            target = replace (temp, "/php/", "/source/");
            free (temp);
            fprintf (suffix, " %s", target);
            free (target);
        }
        fclose (temp);
        fprintf (suffix, "\n\n");
    }

    if (filesToBuildDependenciesFor != 0 && dependencyFile != 0) {
        FILE *temp = fopen(filesToBuildDependenciesFor, "r");
        char buffer[512];
        fprintf (suffix, "# these rules tell us how to make each of the dependency files\n");
        while (GetLine (buffer, temp)) {

            // find the first "/" in the source file name
            char *dirName = strdup (buffer);
            int i = 0;
            for (; dirName[i] != '/'; i++);
            dirName[i] = 0;

            // find the actual name of the file we are compiling to
            char *output = strdup (buffer);
            output[strlen(output) - 2] = 'o';
            output[strlen(output) - 1] = 0;
            i = strlen(output);
            for (; output[i] != '/'; i--);

            // make sure the object directory exists
            fprintf (suffix, "%s.dep:\n", buffer);
            fprintf (suffix, "\techo -e -n \"%s/object/\" > %s.dep\n", dirName, buffer);
            fprintf (suffix, "\t$(CC) $(CCSTANDARD) $(INCLUDE) $(CCFLAGS) -MM %s >> %s.dep\n", buffer, buffer);
            fprintf (suffix, "\techo -e \"\t\\$$(CC) \\$$(CCFLAGS) \\$$(INCLUDE) -o %s/object/%s %s\\n\" >> %s.dep\n\n",
                    dirName, &(output[i + 1]), buffer, buffer);

            sprintf (buffer, "mkdir %s/object/ 2> /dev/null", dirName);
            system (buffer);
            free (dirName);
            free (output);
        }
        fclose (temp);
        temp = fopen(filesToBuildDependenciesFor, "r");
        fprintf (suffix, "# this rule makes all of the dependency files and cats them together\n%s: ", dependencyFile);
        while (GetLine (buffer, temp)) {
            fprintf (suffix, " %s.dep", buffer);
        }
        fclose (temp);
        temp = fopen(filesToBuildDependenciesFor, "r");
        fprintf (suffix, "\n\ttouch %s\n", dependencyFile);
        while (GetLine (buffer, temp)) {
            fprintf (suffix, "\tcat %s.dep >> %s\n", buffer, dependencyFile);
        }

        fprintf (suffix, "\n\n");
        fclose (temp);
        temp = fopen(filesToBuildDependenciesFor, "r");
        fprintf (suffix, "# this rule kills all of the dependency files\nkillAllDependencies:\n\trm");
        while (GetLine (buffer, temp)) {
            fprintf (suffix, " %s.dep", buffer);
        }
        fprintf (suffix, "\n\n");
        fclose (temp);
    }

    for (int i = 0; i < numProjects; i++) {

        // first, we need to write out all of the object files this executable depends on
        FILE *allDirs = fopen ("AllDirs", "w");
        myProjects[i].PrintDirs (allDirs);
        fclose (allDirs);
        Semijoin(filesToBuildDependenciesFor, "AllDirs", "AllRelevantSourceFiles");
        RemDups("AllRelevantSourceFiles", "AllRelevantSourceFiles");

        char buffer[512];
        fprintf (suffix, "%s: ", myProjects[i].target);
        FILE *temp = fopen("AllRelevantSourceFiles", "r");
        while (GetLine (buffer, temp)) {
            char *target = strdup (buffer);
            target[strlen(target) - 2] = 'o';
            target[strlen(target) - 1] = 0;
            char *temp = replace (target, "/source/private/", "/object/");
            free (target);
            target = replace (temp, "/source/", "/object/");
            free (temp);
            fprintf (suffix, " %s", target);
            free (target);
        }
        fclose (temp);
        myProjects[i].PrintObjs (suffix);

        // now we actually write out how to compile it
        //
        // the first part will crete symbolic links to all of the header files this guy uses
        fprintf (suffix, "\n\tmkdir -p Headers");
        int j;
        for (j = strlen (myProjects[i].target); j >= 0; j--) {
            if (myProjects[i].target[j] == '/')
                break;
        }
        j++;
        fprintf (suffix, "%s", myProjects[i].target + j);
        system ("ls */headers/*.h */headers/private/*.h */headers/*.cc */headers/private/*.cc > AllHeaders 2>/dev/null");
        Semijoin("AllHeaders", "AllDirs", "AllRelevantHeaderFiles");
        RemDups("AllRelevantHeaderFiles", "AllRelevantHeaderFiles");
        temp = fopen("AllRelevantHeaderFiles", "r");
        while (GetLine (buffer, temp)) {
            int k;
            for (k = strlen (buffer); k >= 0; k--) {
                if (buffer[k] == '/')
                    break;
            }
            k++;
            fprintf (suffix, "\n\trm -f Headers%s/%s 2>/dev/null", myProjects[i].target + j, &(buffer[k]));
            fprintf (suffix, "\n\tcp %s Headers%s/%s", buffer, myProjects[i].target + j, &(buffer[k]));
        }
        fclose (temp);


        // now give the rule to do the linking
        fprintf (suffix, "\n\t$(CC) $(CCSTANDARD) -o %s", myProjects[i].target);
        temp = fopen("AllRelevantSourceFiles", "r");
        while (GetLine (buffer, temp)) {
            char *target = strdup (buffer);
            target[strlen(target) - 2] = 'o';
            target[strlen(target) - 1] = 0;
            char *temp = replace (target, "/source/private/", "/object/");
            free (target);
            target = replace (temp, "/source/", "/object/");
            free (temp);
            fprintf (suffix, " %s", target);
            free (target);
        }
        myProjects[i].PrintObjs (suffix);
        myProjects[i].PrintLibs (suffix);
        fprintf (suffix, " $(LINKFLAGS) ");
        fprintf (suffix, "\n\n");
        fclose (temp);
    }

    if (phpFilesForHeaders != 0 || phpFilesForCPlusPlus != 0 || filesToBuildDependenciesFor != 0) {
        fprintf (suffix, "clean:\n\trm -rf Headers $(REMOVES) ");
        for (int i = 0; i < numProjects; i++) {
            myProjects[i].PrintObjs (suffix);
        }
        fprintf (suffix, " ");
    }

    if (phpFilesForHeaders != 0) {
        FILE *temp = fopen(phpFilesForHeaders, "r");
        char buffer[512];
        while (GetLine (buffer, temp)) {
            char *target = replace (buffer, "/php/", "/headers/");
            char *target2 = replace (target, "/php/private/", "/headers/");
            target2[strlen(target2) - 4] = 0;
            fprintf (suffix, "%s ", target2);
            free (target);
            free (target2);
        }
        fclose (temp);
    }

    if (phpFilesForCPlusPlus != 0) {
        FILE *temp = fopen(phpFilesForCPlusPlus, "r");
        char buffer[512];
        while (GetLine (buffer, temp)) {
            char *target = replace (buffer, "/php/", "/source/");
            char *target2 = replace (target, "/php/private/", "/source/");
            target2[strlen(target2) - 4] = 0;
            fprintf (suffix, "%s ", target2);
            free (target);
            free (target2);
        }
        fclose (temp);
    }

    if (filesToBuildDependenciesFor != 0) {
        FILE *temp = fopen(filesToBuildDependenciesFor, "r");
        char buffer[512];
        while (GetLine (buffer, temp)) {

            // find the first "/" in the source file name
            char *dirName = strdup (buffer);
            int i = 0;
            for (; dirName[i] != '/'; i++);
            dirName[i] = 0;

            // find the actual name of the file we are compiling to
            char *output = strdup (buffer);
            output[strlen(output) - 2] = 'o';
            output[strlen(output) - 1] = 0;
            i = strlen(output);
            for (; output[i] != '/'; i--);
            fprintf (suffix, "%s/object/%s ", dirName, &(output[i + 1]));
            free (dirName);
            free (output);
        }
        fclose (temp);
        WriteExecs (suffix, myProjects, numProjects);
    }

    fprintf (suffix, "\n\n");


    fclose (suffix);
    WriteAll (myProjects, numProjects, "targs");
    sprintf (buffer, "cat targs %s suffix > %s", prelude, makefileName);
    system (buffer);

    if (rulesForObjectFiles != 0) {
        sprintf (buffer, "mv %s Temp", makefileName);
        system (buffer);
        sprintf (buffer, "cat Temp %s > %s", rulesForObjectFiles, makefileName);
        system (buffer);
    }

}


int main (int numArgs, char **args) {

    int code = 0;
#define PROB_PHP_TO_H 1
#define PROB_PHP_TO_CC 2
#define PROB_DEPENDENCIES 3
#define PROB_CATTING_DEPENDENCIES 4

    if (numArgs != 3) {
        cerr << "Need to give:\n\t1) the name of the file detailing the executables to make.\n";
        cerr << "\t2) the name of the prelude file for the makefile.\n";
        exit (1);
    }

    // this reads in the file that lists (for each target) the directories that need
    // to be compiled and linked in order to create the target
    FILE *input = fopen(args[1], "r");
    int len = 0;
    FilesToBuildFrom *output = ReadExecsFromFile (input, len);

    // "CPlusPlusPHPFiles" will include all php files that will be compiled to a .cc file
    system ("rm CPlusPlusPHPFiles 2>/dev/null");
    system ("ls */php/*.cc.php */php/private/*.cc.php > CPlusPlusPHPFiles 2>/dev/null");
    system ("touch CPlusPlusPHPFiles");

    // "HeaderPHPFiles" will include all php files that will be compiled to a .h file
    system ("rm HeaderPHPFiles 2>/dev/null");
    system ("ls */php/*.h.php */php/private/*.h.php > HeaderPHPFiles 2>/dev/null");
    system ("touch HeaderPHPFiles");

    // "AllUsefulDirectories" will include all directories useful for some target
    WriteAllDirectoriesToFile(output, len, "Temp");
    RemDups("Temp", "AllUsefulDirectories");

    // "AllUsefulCPlusPlusPHPFiles" will include all php files that will be compiled to
    // a .cc file, AND are part of some useful directory
    Semijoin("CPlusPlusPHPFiles", "AllUsefulDirectories", "AllUsefulCPlusPlusPHPFiles");
    Semijoin("HeaderPHPFiles", "AllUsefulDirectories", "AllUsefulHeaderPHPFiles");

    // "AllSourceDirectories" contains all directories of the firm dir/source that are covered
    // by one of the executables we are building
    system ("rm AllSourceDirectories 2>/dev/null");
    system ("ls -d */source/ > AllSourceDirectories 2>/dev/null");
    system ("touch AllSourceDirectories");
    RemDups("AllSourceDirectories", "Temp");
    Semijoin("Temp", "AllUsefulDirectories", "AllSourceDirectories");

    // "AllHeaderDirectories" contains all directories of the form dir/header that are covered
    // by one of the executables we are building
    system ("rm AllHeaderDirectories 2>/dev/null");
    system ("ls -d */headers/ > AllHeaderDirectories 2>/dev/null");
    system ("touch AllHeaderDirectories");
    RemDups("AllHeaderDirectories", "Temp");
    Semijoin("Temp", "AllUsefulDirectories", "AllHeaderDirectories");

    // see which php files that will be compiled to a .cc file don't have a source directory
    // to be put into
    Strip ("AllSourceDirectories", 1, "AllSourceDirectoriesTemp");
    Strip ("AllHeaderDirectories", 1, "AllHeaderDirectoriesTemp");
    Antijoin("AllUsefulCPlusPlusPHPFiles", "AllSourceDirectoriesTemp", "AllThatNeedASource");
    Antijoin("AllUsefulHeaderPHPFiles", "AllHeaderDirectoriesTemp", "AllThatNeedAHeader");

    // this strips out everything except for the first "n" path directives
    Strip("AllThatNeedASource", 1, "SourceDirectoriesToCreate");
    Strip("AllThatNeedAHeader", 1, "HeaderDirectoriesToCreate");

    // and remove the duplicates
    RemDups("SourceDirectoriesToCreate", "FinalSourceDirsToCreate");
    RemDups("HeaderDirectoriesToCreate", "FinalHeaderDirsToCreate");

    char buffer[512];
    int numLines = 0;
    FILE *sourceDirs = fopen ("FinalSourceDirsToCreate", "r");
    while (GetLine (buffer, sourceDirs)) {
        char command[512];
        sprintf (command, "mkdir %ssource", buffer);
        system (command);
        printf ("Created %ssource.\n", buffer);
        numLines++;
    }
    fclose (sourceDirs);
    if (numLines == 0) {
        cout << "Just verified that all source directories I need to compile .cc.php files to exist.\n";
    } else {
        cout << "Created " << numLines << " new source directories to compile .cc.php files to.\n";
    }

    numLines = 0;
    FILE *headerDirs = fopen ("FinalHeaderDirsToCreate", "r");
    while (GetLine (buffer, headerDirs)) {
        char command[512];
        sprintf (command, "mkdir %sheaders", buffer);
        system (command);
        printf ("Created %sheaders.\n", buffer);
        numLines++;
    }
    fclose (headerDirs);
    if (numLines == 0) {
        cout << "Just verified that all header directories I need to compile .h.php files to exist.\n";
    } else {
        cout << "Created " << numLines << " new header directories to compile .h.php files to.\n";
    }

    // "PHPIncludeDirectories" will include all of the directories that have pure php code...
    system ("rm PHPIncludeDirectories 2>/dev/null");
    system ("ls -d */php/*.php */php/private/*.php > PHPIncludeDirectories 2>/dev/null");
    system ("touch PHPIncludeDirectories");
    Semijoin("PHPIncludeDirectories", "AllUsefulDirectories", "Temp");
    Antijoin("Temp", "AllUsefulHeaderPHPFiles", "Temp2");
    Antijoin("Temp2", "AllUsefulCPlusPlusPHPFiles", "PHPIncludeDirectories");
    Strip("PHPIncludeDirectories", 0, "Temp");
    RemDups("Temp", "PHPIncludeDirectories");

    // now, actually compile all of the files in "AllUsefulCPlusPlusPHPFiles" and "AllUsefulHeaderPHPFiles"
    // this is done by creating a makefile for all of them and then running it...
    BuildMakefile ("Makefile", args[2], 0, "PHPIncludeDirectories",
            "AllUsefulHeaderPHPFiles", "AllUsefulCPlusPlusPHPFiles",
            0, 0, 0, 0, 0);

    // now, run the makefile to compile the .cc.php and .h.php files
    cout << "Now I am going to make all of the .cc.php and .h.php files, so I can check for dependencies.\n";
    if (system("make " EXTRA_MAKE_OPTION " allHFromPHP") != 0) {
        code = PROB_PHP_TO_H;
        goto end;
    }

    if (system("make " EXTRA_MAKE_OPTION " allCPlusPlusFromPHP") != 0) {
        code = PROB_PHP_TO_CC;
        goto end;
    }

    // "AllCPlusPlusFiles" will include all of the C++ files in any interesting directory
    system ("rm AllCPlusPlusFiles 2>/dev/null");
    system ("ls */source/*.cc */source/private/*.cc > AllCPlusPlusFiles 2>/dev/null");
    RemDups("AllCPlusPlusFiles", "Temp");
    Semijoin("Temp", "AllUsefulDirectories", "AllCPlusPlusFiles");

    // "AllHeaderDirectories" will include all of the header files in any interesting directory
    system ("rm AllHeaderDirectories 2>/dev/null");
    system ("ls */headers/*.h */headers/private/*.h */headers/*.cc */headers/private/*.cc > AllHeaderDirectories 2>/dev/null");
    system ("touch AllHeaderDirectories");
    Strip("AllHeaderDirectories", 0, "Temp1");
    RemDups("Temp1", "Temp");
    Semijoin("Temp", "AllUsefulDirectories", "AllHeaderDirectories");

    // now we re-do the Makefile to include a target that makes a file with all of the compilation commands
    system ("rm DependencyFile 2>/dev/null");
    BuildMakefile ("Makefile", args[2], "AllHeaderDirectories", 0, 0, 0,
            "AllCPlusPlusFiles", "DependencyFile", 0, 0, 0);

    if (system("make " EXTRA_MAKE_OPTION " DependencyFile") != 0) {
        code = PROB_DEPENDENCIES;
        goto end;
    }

    if (system("make " EXTRA_MAKE_OPTION " killAllDependencies") != 0) {
        code = PROB_CATTING_DEPENDENCIES;
        goto end;
    }


    BuildMakefile ("Makefile", args[2], "AllHeaderDirectories",
            "PHPIncludeDirectories", "AllUsefulHeaderPHPFiles", "AllUsefulCPlusPlusPHPFiles",
            "AllCPlusPlusFiles", 0, "DependencyFile", len, output);

end:

    delete [] output;

    system ("rm AllDirs AllHeaderDirectories AllHeaderDirectoriesTemp");
    system ("rm AllRelevantSourceFiles AllSourceDirectories AllSourceDirectoriesTemp AllThatNeedAHeader");
    system ("rm AllThatNeedASource AllUsefulCPlusPlusPHPFiles AllUsefulDirectories AllUsefulHeaderPHPFiles");
    system ("rm CPlusPlusPHPFiles DependencyFile FinalHeaderDirsToCreate FinalSourceDirsToCreate");
    system ("rm HeaderDirectoriesToCreate HeaderPHPFiles PHPIncludeDirectories SourceDirectoriesToCreate");
    system ("rm AllHeaders AllRelevantHeaderFiles targs AllCPlusPlusFiles suffix Temp Temp1 Temp2 ");

    if (code == PROB_PHP_TO_H) {
        cout << "\nProblem compiling some .h file from php.";
    } else if (code == PROB_PHP_TO_CC) {
        cout << "\nProblem compiling some .cc file from php.";
    } else if (code == PROB_DEPENDENCIES) {
        cout << "\nProblem when using the C++ compilier to build dependencies for some C++ file.\n";
    } else if (code == PROB_CATTING_DEPENDENCIES) {
        cout << "\nProblem putting all of the dependencies together.\n";
    } else {
        cout << "\nDONE!!\n";
        cout << "Created 'Makefile' with rules for the following executables:\n";
        for (int i = 0; i < len; i++) {
            cout << "\t" << output[i].target << "\n";
        }
        cout << "\n";
        return 0;
    }
    return 1;

}
