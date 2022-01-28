#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <fcntl.h>


#define NANOSECONDES 1000000000 // nombre de nanosecondes en 1 sec
#define NANOFLOAT 1000000000.0 // nombre de nanosecondes en 1 sec en float
#define MICROFLOAT 1000000.0  //nombre de microsecondes en 1 sec en float


/* fonction qui calcul la différence de temps entre deux struct timespec
 * modifie la struct timespec presente en argument afin qu'elle contienne le résultat de la difference
 */
void differenceTemps(struct timespec debut, struct timespec fin,struct timespec* resultat){


    size_t tempsNanoDebut = debut.tv_sec*NANOSECONDES+ debut.tv_nsec;
    size_t tempsNanoFin = fin.tv_sec*NANOSECONDES+ fin.tv_nsec;
    size_t difference = tempsNanoFin - tempsNanoDebut;

    resultat->tv_sec=difference/NANOSECONDES;
    resultat->tv_nsec= difference%NANOSECONDES;

}


/* fonction du man de perf_event_attr qui nous permet d'executer le bon appel système
 *  car perf_event_attr ne possède pas de wrapper
 */
static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                  group_fd, flags);
    return ret;
}

//Fonction qui retourne vrai ou faux si un string est un nombre positif ou non
int isNumber(char *argv) {
    char *endptr;
    long int argument = strtol(argv, &endptr, 0);
    return !(*argv == '\0' || *endptr != '\0' || argument <= 0);
}


/* Gère les arguments de la ligne de commande retourne :
 * -1 s'il y a une erreur dans les arguments
 * retourne nombre d'options qui est le nombre d'options dans la ligne de commande allant de 0 a 4
 * La fonction stocke dans retour les differentes options de pf de cette maniere :
 * retour[0] = option -u -c -a ou vide s'il n'y a pas une de ses options
 * retour[1] = option - n ou vide s'il n'y a pas cette option
 * retour[2] = le nombre de repetition de -n ou vide s'il n'y a pas cette option
 * retour[3] = option -s ou vide s'il n'y a pas cette option
 */
int cmd(int argc, char **argv,char** retour){


    int option1=0; // int qui sera égale à 1 s'il ya une option -u -c -a ou 0 sinon
    char firstOption[10]=""; // variable qui va contenir  -u -c -a ou vide
    int nombreOptions=0; // variable qui contient le nombre d'options données à pf



    //On parcourt tout les arguments de pf et on compte le nombre d'options
    // les options de pf contient un - ou un nombre, donc on avance jusqu'à ce qu'on rencontre un argument
    // sans tiret ni de nombre donc on assume que c'est un programme à lancer
    for(int i=1;i<argc;i++){

        if(argv[i][0]=='-'||isNumber(argv[i])) {
            nombreOptions++;
        } else {
            break;
        }
    }


    for(int i=1;i<nombreOptions+1;i++){


        if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "-a") == 0) {
            strcpy(firstOption,argv[i]);
            option1++;
        } else if (!(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-n") == 0 || isNumber(argv[i]))) {
            return -1;
        }

    }


    int option2=0; // int qui sera égale à 0 s'il y n'y a pas d'option -n ni de nombre, 1 s'il ya juste -n ou un nombre, 2 s'il y a les deux
    char secondOption[10]="";  // va contenir -n ou sera vide
    char nombre[10]="";    // va contenir le nombre de repetition ou sera vide


    for(int i=1;i<nombreOptions+1;i++){
        if(strcmp(argv[i],"-n") == 0 ){

            if( i+1<=argc-1 && isNumber(argv[i+1]) ){
                strcpy(secondOption,argv[i]);
                strcpy(nombre,argv[i+1]);
                option2=option2+2;
            } else {
                return -1;
            }

        }
    }


    int option3=0;  // int qui va contenir 0 s'il n'y a pas l'option -s et 1 s'il y a l'option
    char thirdOption[10]=""; // va contenir -s ou sera vide


    for(int i=1;i<nombreOptions+1;i++){


        if(strcmp(argv[i],"-s") == 0){
            strcpy(thirdOption,argv[i]);
            option3++;
        }

    }
    if(option1>1||option2>2||option3>1){ // cas ou la meme options a été entrée a plusieurs reprises
        return -1;
    }else if(nombreOptions==argc-1) { // cas ou seulement les options sont present dans les arguments donc pf n'a pas de programme
        return -1;
    }else{



        *(retour)=malloc(sizeof(char)*strlen(firstOption)+1);
        if (retour == NULL) {
            fprintf(stderr, "Impossible d'allouer de la mémoire.");
            return 127;
        }
        *(retour+1)=malloc(sizeof(char)*strlen(secondOption)+1);
        if (retour+1 == NULL) {
            fprintf(stderr, "Impossible d'allouer de la mémoire.");
            return 127;
        }
        *(retour+2)=malloc(sizeof(char)*strlen(nombre)+1);
        if (retour+2 == NULL) {
            fprintf(stderr, "Impossible d'allouer de la mémoire.");
            return 127;
        }
        *(retour+3)=malloc(sizeof(char)*strlen(thirdOption)+1);
        if (retour+3 == NULL) {
            fprintf(stderr, "Impossible d'allouer de la mémoire.");
            return 127;
        }


        strcpy(retour[0],firstOption);
        strcpy(retour[1],secondOption);
        strcpy(retour[2],nombre);
        strcpy(retour[3],thirdOption);


        return nombreOptions;

    }


}


/* Fonction qui retourne un tableau qui va contenir le programme de pf et "sh -c"
 * afin de l'ajouter au exec
 */
void TraitementOptionS(int argc,int nombreOptions,char **argv,char** optionS){
    *(optionS)=malloc(sizeof(char)*strlen("sh")+1);
    if (optionS == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        exit(127);
    }
    *(optionS+1)=malloc(sizeof(char)*strlen("-c")+1);
    if (optionS+1 == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        exit(127);
    }

    strcpy(optionS[0],"sh");
    strcpy(optionS[1],"-c");

    int last=0;

    //Boucle qui va ajouter le programme de pf au tableau optionS a la fin  qu'on ait apres "sh -c commande..." afin de l'ajouter au exec
    for(int i=1;i<argc-nombreOptions;i++){
        *(optionS+i+1)=malloc(sizeof(char)*strlen(argv[i+nombreOptions])+1);
        if (optionS+i+1 == NULL) {
            fprintf(stderr, "Impossible d'allouer de la mémoire.");
            exit(127);
        }
        strcpy(optionS[i+1],argv[i+nombreOptions]);
        last=i;

    }

    //ajoute un NULL a la fin du tableau optionS car le exec necessite un tableau qui finit par NULL
    *(optionS+last+2)=NULL;
}


int main (int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "pf [-u|-c|-a] [-n n] [-s] commande [argument...]\n");
        return 127;
    }
    char** retour=NULL;
    retour=realloc(retour,sizeof(*retour)*4);


    int nombreOptions=cmd(argc,argv,retour);
           if(nombreOptions==-1){
            fprintf(stderr, "pf [-u|-c|-a] [-n n] [-s] commande [argument...]\n");
            return 127;
          }


    char *endptr;
    // si  retour[2] qui contient le nombre de repetition est vide alors on prend 1 repetition sinon on prend
    // ce qu'il ya dans retour[2]
    int nombreTour= ( strcmp(retour[2],"")==0 ) ? 1 : strtol(retour[2], &endptr, 0);
    int compteur=0;


    char **optionS = NULL;
    optionS = realloc(optionS, sizeof(*optionS) * argc+1);
    if (optionS == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire.");
        exit(127);
    }

    if(   strcmp(retour[3],"-s") == 0 ) {
        TraitementOptionS(argc,nombreOptions, argv, optionS);
    }



    pid_t pid_fils;
    int wstatut;
    struct timespec debut, fin, difference;
    struct rusage rusage;
    struct perf_event_attr pe;
    long long count; // variable qui contient le nombre de cycles
    int fd; //file descriptor


    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.exclude_kernel = 1;
    pe.inherit = 1; // permet de compter les evenements du fils
    pe.disabled = 1; // specifie qu'on veut le compteur desactive au debut afin de l'activer plus tard avec ioctl
    pe.comm = 1;   // permet de tracker les processus qui sont execute par le exec
    pe.exclude_hv = 1;  // exclus les cycles de l'hyperviseur
    pe.exclude_idle = 1;  //exclus les cycles lorsque le processus est inactif
    pe.inherit_stat = 1; // permet de sauver les stats pour les fils
    pe.task = 1; // utile pour le exec



    float moyenneTime=0;
    float moyenneUsage=0;
    long long int moyenneCycle=0;

    int retourGeTime=0;
    int retourIoctl=0;

    while ( compteur<nombreTour) {


        pid_fils = fork();
        if (pid_fils == -1) {
            perror("Echec du fork");
            return 127;
        }

        if( strcmp(retour[0],"-c")==0 || strcmp(retour[0],"-a")==0  )
            fd = perf_event_open(&pe, pid_fils, -1, -1, 0);

        if (fd == -1) {
            perror("Echec du perf_event_open");
            return 127;
        }

        if( strcmp(retour[0],"")==0 || strcmp(retour[0],"-a")==0  )
            retourGeTime=clock_gettime (CLOCK_MONOTONIC, &debut);

        if (retourGeTime == -1) {
            perror("Echec du clock_gettime");
            return 127;
        }


        if( strcmp(retour[0],"-c")==0 || strcmp(retour[0],"-a")==0  ) {
            retourIoctl=ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            if(retourIoctl==-1){
                perror("Echec de ioctl");
                return 127;
            }
            retourIoctl=ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
            if(retourIoctl==-1){
                perror("Echec de ioctl");
                return 127;
            }
        }

        if (pid_fils == 0 && strcmp(retour[3],"-s") != 0) {
            execvp(argv[1+nombreOptions], argv + (nombreOptions+1) );
            perror("La commande ne peut être exécuté");
            return 127;
        } else if ( pid_fils == 0) {
            execvp("sh", optionS);
            perror("La commande ne peut être exécuté");
            return 127;

        }
        

        pid_fils = wait4(-1, &wstatut, 0, &rusage);
        if(pid_fils == -1){
            perror("Erreur de wait4");
            return 127;
        }

        if( strcmp(retour[0],"")==0 || strcmp(retour[0],"-a")==0  )
            retourGeTime=clock_gettime (CLOCK_MONOTONIC, &fin);

        if (retourGeTime == -1) {
            perror("Echec du clock_gettime");
            return 127;
        }

        int retourRead=0;

        if( strcmp(retour[0],"-c")==0 || strcmp(retour[0],"-a")==0  ) {
            retourIoctl=ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
            if(retourIoctl==-1){
                perror("Echec de ioctl");
                return 127;
            }
            read(fd, &count, sizeof(long long));
        }

        if(retourRead==-1){
            perror("Erreur du read : ");
            return 127;
        }


        if (WIFEXITED(wstatut) && !WEXITSTATUS(wstatut) ) { //retourne "true" si le fils a terminé normalement


            if( strcmp(retour[0],"")==0 ) {
                differenceTemps(debut, fin, &difference);
                float resultat = difference.tv_sec + (difference.tv_nsec / NANOFLOAT);
                moyenneTime+=resultat;
                printf("%.2f\n", resultat);

            }else if( strcmp(retour[0],"-u")==0 ) {
                float usage = rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / MICROFLOAT);
                moyenneUsage+=usage;
                printf("%.2f\n", usage);


            }else if( strcmp(retour[0],"-c")==0 ) {
                moyenneCycle+=count;
                printf("%lld\n", count);

            } else {
                differenceTemps(debut, fin, &difference);
                float resultat = difference.tv_sec + (difference.tv_nsec / NANOFLOAT);
                float usage = rusage.ru_utime.tv_sec + (rusage.ru_utime.tv_usec / MICROFLOAT);


                moyenneTime+=resultat;
                moyenneUsage+=usage;
                moyenneCycle+=count;


                printf("%.2f %.2f %lld\n", resultat,usage,count);

            }



        } else {
            differenceTemps(debut, fin, &difference);
            float resultat = difference.tv_sec + (difference.tv_nsec / NANOFLOAT);
            moyenneTime+=resultat;
            printf("%.2f\n", resultat);
            return 127;
        }

        compteur++;
    }

    if(nombreTour>1) {

        if( strcmp(retour[0],"")==0 ) {
            printf("%.2f\n", moyenneTime/nombreTour);

        } else if( strcmp(retour[0],"-u")==0 ) {

            printf("%.2f\n", moyenneUsage/nombreTour);

        } else if ( strcmp(retour[0],"-c")==0 ) {

            printf("%lld\n", moyenneCycle/nombreTour);

        } else {
            printf("%.2f %.2f %lld\n", moyenneTime/nombreTour,moyenneUsage/nombreTour,moyenneCycle/nombreTour);
        }

    }


    return 0;
}