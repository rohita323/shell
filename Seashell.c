/* If the command has a '&' at the end of the line, this means that
   the process should be kept in the background until it finishes.
   This is when it sends a signal to the user that it has terminated.
*/
/*terminal will not exit unless quit is written*/

#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

typedef struct node{
  char command[4096];
  pid_t id;
  int stat;
}node;

node background[500];
int count=0;


void clrscrn();
void welcom();
void prompt();

void cd(char* commi[]);
void checktermination();
int  checkinput(char* buf);
int  checkre(char* commi[], int spaz);
int  compare(char *commi[]);
void controlZ(int sig_num);
void fg(char *commi[]);
void insert(char buf[], pid_t id);
void jobs();
void kjob(int job, int sig);
void overkill();
void pinfo(char *commi[]);
int  pipes(char *commi[], int spaz);
void copier(int start,int flag,char *no1[],char *commi[]);
int  redir(char *commi[], int spaz);
int  redirection(char *commi[], int spaz);
void siggi(int sig_num);

int size;
pid_t shell;

int main(){
        
    char buf[4096], add[1000], **commi=NULL, *p, c[2], d[2]; 
    int status, spaz=0, i, flag=0,in,j=0;
    pid_t pid,temp;

    getcwd(add,1000); size=strlen(add);
    clrscrn(); welcom(); 

    shell=getpid(); //get shell's pid for future reference
    
    while(1){
      int pipflag=0,redirect=0;
      char pipstring[4096]="";

      prompt();spaz=0;flag=0;j=0;redirect=0;
      
      signal(SIGINT, siggi); //shell doesn't exit with ctrl-c
      signal(SIGQUIT, siggi);//shell doesn't exit with any quit signals
      signal(SIGTSTP, siggi);//shell doesn't exit with ctrl-z
      signal(SIGCHLD, siggi);
      signal(SIGCHLD, checktermination);
    //  signal(SIGTSTP, controlZ);
           
      if (fgets(buf,4096,stdin)==NULL)  //shell doesn't exit with ctrl-d
      {write(1,"\n", 1);continue;}

      in=checkinput(buf);               //check for weird inputs like empty spaces and just an enter
      if(in==1)     continue;

      if(buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0';

          p= strtok (buf, " \t");
      
          while (p) {
            commi = realloc (commi, sizeof (char*) * ++spaz);
            if (commi == NULL)  break;
           commi[spaz-1] = p;
           p = strtok (NULL, " \t");}

          commi = realloc (commi, sizeof (char*) * (spaz+1));
          commi[spaz] = 0;

      in=compare(commi);
      if(in==1)       continue;
      if(in==2)       break;

      if(strcmp(commi[spaz-1],"&")==0)  //this puts process in background
        {flag=1; commi[spaz-1]=NULL; spaz--;}

      
      for(i=0;i<spaz;i++)
      {if(strcmp( commi[i],"|")==0)
      {pipflag=1;break;}}

     if(pipflag==1)
      { pipes(commi,spaz); continue;}

      if((pid = fork()) < 0 ){
        write(1,"Fork Error\n", 12);
          return -1;}

      if(pid == 0)
      {
        if(!redir(commi,spaz)) 
        {execvp(commi[0], commi);
        write(1,"Could not execute\n", 19);
        return -1;}
      }

      else if(!flag)              //process is in foreground
      { if((pid = waitpid(pid,&status,0)) < 0)
          write(1,"Wait Error\n",12);}
     
      if (flag)
      {insert(buf,pid);}
    
      }

    return 0;
}

int compare(char *commi[])
{
  int flag=0;
  char c[2],d[2];

  if(commi[0]==NULL)  //handles cases where user presses enter
    flag=1;

  if(strcmp( commi[0],"quit")==0)  //exits shell "quit"
    flag=2;

  if (strcmp( commi[0],"cd")==0)   //hardcoding cd
    {cd(commi); flag=1;}

  if (strcmp( commi[0],"fg")==0)   //hardcoding cd
    {fg(commi); flag=1;}


  if (strcmp( commi[0],"pinfo")==0)   //hardcoding pinfo
    {pinfo(commi); flag=1;}

  if (strcmp( commi[0],"jobs")==0)   //hardcoding jobs
    {jobs(); flag=1;}

  if (strcmp( commi[0],"overkill")==0)   //hardcoding jobs
    {overkill(); flag=1;checktermination();}

  if (strcmp( commi[0],"kjob")==0)   //hardcoding kjobs
    {
       if (commi[1]==NULL || commi[2]==NULL)
        {write(1,"missing arguments\n",18);return (1);}
       strcpy(c,commi[1]);
       strcpy(d,commi[2]);
       kjob(c[0]-'0', d[0]-'0'); flag=1;
       checktermination();
    }
return (flag);
}

void welcom()
{ char  use[1000], welcome[1000]; 
  sprintf(use,"Welcome to the Terminal %s!!!\n",getlogin());
  write(1, use, strlen(use));}

void clrscrn()
{ write(1, "\e[1;1H\e[2J", 11);}

void prompt()
{  int h;
   char address[1000], use[1000],current[1000],new[1000]="";

   gethostname(address,1000);     //getting the username and sytem name
   getcwd(current,1000);                                 //getting the current working directory
   h=strlen(current);
   
   strncpy(new,current + size,h-size);
 
   sprintf(use,"<%s@%s:~%s> ",getlogin(),address,new);
   write(1,use,strlen(use));}

int checkinput(char* buf)
{
  int i=0,j=0, flag=0;

  if(strlen(buf)==1 && (buf[0]=='\n'))  //no errors occur when user presses just enter
    flag=1;
      
  for(i=0;i<strlen(buf);i++)   //no errors with multiple spaces or tabs
   { if(buf[i]!=' ' && buf[i]!='\t')
       break;
     else
       j++;}    
  if(j==strlen(buf)-1)
    flag=1;

  return (flag);
}

void cd(char* commi[])
{
  char addr[1000], child[1000], *parent;
  getcwd(addr,1000);
  
  if(commi[1]==NULL)
  { strncpy(child,addr,size);
    chdir(child);return;}
  
  if(strcmp(commi[1],"..")==0)
  {
    if(strlen(addr)<=size)
      return;
    parent=strrchr(addr,'/');
    *parent='\0';
    chdir(addr);  }
  
  else
  { strcat(addr,"/");
    strcat(addr,commi[1]);
    chdir(addr);}
}

void checktermination()
{
  pid_t child;
  int status,i;
  char message[4096];

  while((child=waitpid(-1, &status, WNOHANG))>0)
    {
      for(i=0;i<count;i++)
      {
        if(background[i].id==child)
          {
            sprintf(message,"\n%s with pid %d exited normally\n", background[i].command,background[i].id);
            background[i].stat=0;
            write(1,message,strlen(message));
          }
      }
       prompt(size); 
     }
     signal(SIGCHLD, checktermination);

}

void controlZ(int sig_num)
{
    pid_t id=getpid();
    kill(id,SIGTSTP);
    fflush(stdout);
}

void fg(char *commi[])
{
  char c[2];
  int v,i,cc=0,status,id;

  strcpy(c,commi[1]);
  v=c[0]-'0';
  
  for (i=0;i<count;i++)
  {
    if(background[i].stat==1)
      cc++;    
    if(cc==v)
    {
      if((background[i].id = waitpid(background[i].id,&status,0)) < 0)
        {write(1,"Wait Error\n",12);return;}
      background[i].stat=0; 
    }
  }
}

void insert(char buf[], pid_t id)
{
    strcpy(background[count].command, buf);
    background[count].id=id;
    background[count].stat=1;
    count++;
}

void jobs()
{
  int i, c=1;
  char com[4096];
  for(i=0;i<count;i++)
  {
    if(background[i].stat==1)
    { sprintf(com, "[%d] %s [%d]\n", c,background[i].command,background[i].id);
      write(1,com,strlen(com));
      c++;}
  }
}

void kjob(int job, int sig)
{
  int i,cc=0,v;
  for(i=0;i<count;i++)
  {
    if(background[i].stat==1)
      cc++;
    if(cc==job)
    {
      if((v =kill(background[i].id,sig))<0)
        write(1,"Kill Error\n",11);
      return;
    }
  }
  write(1,"nothing to kill\n",17);
}

void overkill()
{
  int i, c=1;
  char com[4096];
  for(i=0;i<count;i++)
  {
    if(background[i].stat==1)
       kill(background[i].id,9);
  }
}

void pinfo(char *commi[])
{
  char print[4096], path[150], content[1000],buf;
  char *p, **sigh, con[1000],new[1000],edit[1000];
  int fd,i=0,j=0,h;
  int id;

  if(commi[1]==NULL)
    id=shell;
  else
    id=atoi(commi[1]);
  
  sprintf(path, "/proc/%d/stat", id);
  
  if((fd= open(path, O_RDONLY))<0)
    {write(1,"Process doesn't exist\n",22);return;}

  while(read(fd,&buf,1)!=0)
    content[i++]=buf;
  content[i]='\0';

   p= strtok (content, " ");
          while (p) {
            sigh = realloc (sigh, sizeof (char*) * ++j);
            if (sigh == NULL)
             break;
           sigh[j-1] = p;
           p = strtok (NULL, " ");}

          sigh = realloc (sigh, sizeof (char*) * (j+1));
          sigh[j] = 0;

  sprintf(path, "/proc/%d/exe", id);

  if(readlink(path,con,1000)>=0)
  {
    strcpy(edit,con);
    h=strlen(edit);
    if(h>=size)
    strncpy(new,edit+size,h-size);
  }
  else
    perror("error with readlink");

  sprintf(print, "pid -- %d\nProcess Status --%s\nMemory --%s\nExecutablepath -- ~%s\n",id,sigh[2],sigh[22],new);
  write(1,print,strlen(print));
}

int pipes(char *commi[], int spaz)
{
  int i,k,l,count=0,location[20],status;
  char **no1;
  pid_t pid;
  int flag=0,flag1=0;

  for(i=0;i<spaz;i++)
    if(strcmp( commi[i],"|")==0)
      {count++;location[count]=i;}

  int pipers[count][2];
  location[0]=-1;   location[count+1]=spaz;

  for(i=0;i<count;i++)
    pipe(pipers[i]);

  for(i=0;i<=count;i++)
  {
     flag=0;flag1=0;
     if((pid = fork()) < 0 )   {write(1,"Fork Error\n", 12); return (1);}

     if(pid == 0)
     {
        for(k=0,l=(location[i]+1);l<location[i+1];l++,k++)
        { no1 = realloc (no1, sizeof (char*) * ++k);
          no1[--k]=commi[l];}
          no1[k]=NULL;
         
          flag = checkre(no1,k);
          //printf("%d\n", flag);

          if(flag>0)
            flag1=redir(no1,k);
          if(flag1)
            continue;
            
          if((i!=count) && (flag<2))
          {if(dup2(pipers[i][1],STDOUT_FILENO)<0)
            {write(1,"dup2 output problem\n",20); return (1);}}

          if((i>0) && (flag!=1))
          {if(dup2(pipers[i-1][0],STDIN_FILENO) < 0)  
            {write(1,"dup2 input problem\n",19); return (1);}}

        for(k=0;k<count;k++)    
          {close(pipers[k][0]);close(pipers[k][1]);}
          
        execvp(no1[0], no1);
        write(1,"Could not execute\n", 19); return (1);}
    }

    for(k=0;k<count;k++)    
      {close(pipers[k][0]);close(pipers[k][1]);}
    for(k=0;k<count+1;k++)    
    wait(&status);
    return 0;
  }

int  checkre(char* commi[], int spaz)
{
  int flag=0,i;

  for(i=0;i<spaz;i++)
  {
    if(strcmp( commi[i],"<")==0)
      {flag=1;break;}
    if(strcmp( commi[i],">")==0)
      {flag=2;break;}
    if(strcmp( commi[i],">>")==0)
      {flag=3;break;}
  }

  return(flag);
}

int redir(char *commi[], int spaz)
{
  int i,in=0,out=0,append=0, fin, fout, fappend,flag=0;
        for(i=0;i<spaz;i++)
        {
          if(strcmp( commi[i],">")==0)
          {
            out=i+1;
            if(access(commi[out], F_OK)>=0)
            {write(1,"File already exists\n",20); flag=1;break;}
        
            if((fout = creat(commi[out], 0644))<0)
            {write(1, "Error creating the output file\n", 31); flag=1;break;}
            dup2(fout,STDOUT_FILENO);
          } 

          else if(strcmp( commi[i],">>")==0)
          {
            append=i+1;
            if(access(commi[append], F_OK)<0)
            {write(1,"File doesn't exist\n",19); flag=1;break;}
        
            if((fappend= open(commi[append],O_WRONLY|O_APPEND,0666))<0)
            {write( 1, "Error opening the input file\n", 29); flag=1;break;}
        
            dup2(fappend,STDOUT_FILENO);
          }    

          else if(strcmp( commi[i],"<")==0)
          {
            in=i+1;
            if((fin = open(commi[in],O_RDONLY,0))<0)
            {write( 1, "Error opening the input file\n", 29); flag=1;break;}
            dup2(fin,STDIN_FILENO);
          }
        }
 
    if(in>0)
    {commi[in]=commi[in-1]=NULL;}

    if(out>0)
    {commi[out]=commi[out-1]=NULL;}

    if(append>0)
    {commi[append]=commi[append-1]=NULL;}

 // if(in>0 || out>0 || append>0)
   // return (1);
  
  return (flag);
}

void siggi(int sig_num)
{
    signal(SIGINT, siggi);
    signal(SIGQUIT, siggi);
    signal(SIGTSTP,siggi);
    fflush(stdout);
}

