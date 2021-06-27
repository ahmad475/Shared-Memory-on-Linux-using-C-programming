#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>






#define MS 1000000 
#define SN "PS"
#define MB 70000 
void writeOutput(char* command, char* output)
{
	printf("The output of: %s : is\n", command);
	printf(">>>>>>>>>>>>>>>\n%s<<<<<<<<<<<<<<<\n", output);	
}









int JoinArguments(char *clines)
{
    int totarg = 0;
    int size = strlen(clines);
    for (int i = 0; i < size; i++)
    {
        if (clines[i] == ' ')
        {
            clines[i] = '\0';
            totarg++;
        }
    }
    return totarg + 1;
}





 




void CommandExecution(char *const ag[], int writepipe_fd)
{
    if (dup2(writepipe_fd, STDOUT_FILENO) == -1)
    {
        perror("error dup2");
        exit(-1);
    }
    execvp(ag[0], ag);
}











int JoinCommands(char *m2)
{
    int c1 = 0;
    int i = 0;
    while (m2[i] != '\0')
    {
        if (m2[i] == '\n')
        {
            c1++;
            m2[i] = '\0';
        }
        if (m2[i] == '\r')
        {
            c1++;
            m2[i] = '\0';
        }
        i++;
    }
    if (i > 0 && m2[i - 1] == '\n')
    {
        m2[i - 1] = '\0';
        c1--;
    }
    return c1 + 1;
}
















int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "no second argument for %s\n", argv[0]);
        exit(-1);
    }
    int sfd = shm_open(SN, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (sfd < 0)
    {
        perror("Could not initiate main memory");
        exit(-1);
    }
    if (ftruncate(sfd, MS) == -1)
    {
        perror("could not truncate there was an error");
        exit(-1);
    }
    char *m1 = mmap(NULL, MS, PROT_READ | PROT_WRITE, MAP_SHARED, sfd, 0);
    if (m1 == (char *)-1)
    {
        perror("error with the memory");
        exit(-1);
    }
    FILE *fp1 = fopen(argv[1], "r");
    if (fp1 == NULL)
    {
        exit(-1);
    }
    int nr1 = fread(m1, 1, MS - 1, fp1);
    if (nr1 == 0)
    {
        exit(-1);
    }
    m1[nr1] = '\0';
    int nl = JoinCommands(m1);
    char **clines = malloc(sizeof(char *) * nl);
    int oset = 0;
    for (int i = 0; i < nl; i++)
    {
        clines[i] = strdup(m1 + oset);
        oset += strlen(clines[i]) + 1;  
    }
    munmap(m1, MS);
    close(sfd);
    char *bff = malloc(sizeof(char) * MB);
    for (int i = 0; i < nl; i++)
    {
        char *newcline = strdup(clines[i]);
        int args = JoinArguments(newcline);
        char *argar[args + 1];
        int arg_offset = 0;
        for (int i = 0; i < args; i++)
        {
            argar[i] = newcline + arg_offset;
            arg_offset += strlen(argar[i]) + 1;
        }
        argar[args] = NULL;
        int pfd[2];
        if (pipe(pfd) == -1)
        {
            perror("error with the pipes");
            exit(-1);
        }
        if (fork() == 0)
        {
            close(pfd[0]);
            CommandExecution(argar, pfd[1]);
            exit(0);
        }
        close(pfd[1]);
        wait(NULL);
        int rnum = read(pfd[0], bff, MB - 1);
        close(pfd[0]);
        bff[rnum] = '\0';
        if (strcmp(clines[i], "\0\0") != 0)
        {
            writeOutput(clines[i], bff);
        }
        free(newcline);
    }
    return 0;
}