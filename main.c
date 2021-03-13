
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <string.h>
#include <sys/time.h>

#define MAX 300
#define PIPE_NAME "input_pipe"
#define NORMAL 1
#define PRIORITARIO 2
#define MAX_P 500
#define MAX_C 2000


/**ESTRUTURAS**/
typedef struct condicao_s {
    pthread_cond_t cond;
    pthread_mutex_t mutex_time;
} struct_sh_mem;

typedef struct config{
    int ut;
    int dur_descolagem;
    int int_descolagem;
    int dur_aterragem; /** vai ser dur_descolagem = T * ut **/
    int int_aterragens;
    int hold_min;
    int hold_max;
    int max_partidas;
    int max_chegadas;
}Config;

typedef struct voo_partida{
    char nome[MAX];
    int init,descolagem;
    int iniciado;
    int slot;

}Voo_partida;

typedef struct voo_chegada{
    char nome[MAX];
    int init,eta,fuel;
    int iniciado;
    int slot;

}Voo_chegada;

typedef struct{
    long msgtype;
    int condicao;
    char buffer[MAX];
    int slot;
    int departure;
    int eta;
    int fuel;
}Message;


typedef struct{
    int estado;
    int slot;
    int eta;
    int fuel;
}Chegada;

typedef struct{
    int estado;
    int slot;
    int departure;
}Partida;

typedef struct mem{
    sem_t sem_empty,sem_full,sem_mutex;
}mem_sem_struct;

typedef struct estat{
    int total_voos_criados, total_voos_aterraram;
    float tempo_medio_espera_aterrar, tempo_medio_espera_descolar; /**  tempo medio de espera(para alem do ETA) para aterrar**/
    int total_voos_descolaram;
    int manobras_hoolding_aterragem; /**  numero medio de manobras de holding por voo de aterragem **/
    int manobras_hoolding_urgencia; /** numero medio de manobras de holding por voo em estado de urgencia  **/
    int voos_redirecionados; /**  numero de voos redirecionados para outro aeroporto**/
    int voos_rejeitados; /**  voos rejeitados pela Torre de Controlo **/

}Estatistica;


/*-----------------------------------------*/


void funcao_print_chegada(Voo_chegada lista[], int max);
void funcao_print_partida(Voo_partida lista[], int max);

void init_stats();


/* CHAMADA DAS FUNCOES     */
double converte_tempo(double elapsed,int ut);
int ver_fuel(int init, int eta, int fuel);
static void clean_MQ();
static void cria_MQ();

void clean_SM();

void clean_CT(int signum);

void *decides_departure_arrival(void* arg);

int compara_nome(char aux[]);

void elimina_slot_lista_memoria_partilhada(int slot, int condicao);

int verifica_prioritario(int eta, int fuel, int init);

void gestor_simulacao();
void *pipe_work(void *arg);
void *work(void *ids);

void torre_controlo();
Config ler_ficheiro_configs(char nome[]);
void tira_enter(char str[]);
void cria_ficheiro_log(char nome[]);
void escreve_ficheiro_log(char str[]);

void inicializa_vetores(Voo_chegada lista_chegadas[],Voo_partida lista_partidas[]);
Voo_chegada comando_voo_chegada(char buffer[]);
Voo_partida comando_voo_partida(char buffer[]);
void sort_fila_chegada_prioridade(Chegada fila_chegada[],int size);
void *time_loop(void* id);

void sigint(int signum);
int contador_vetor_chegadas(Voo_chegada vetor_chegadas[], int max);
int contador_vetor_partidas(Voo_partida vetor_partidas[], int max);
void adicionar_aovetor_chegadas(Voo_chegada vetor_chegadas[], int max, Voo_chegada a_adicionar);
void adiciona_aovetor_partidas(Voo_partida vetor_partidas[], int max, Voo_partida a_adicionar);

void inicializa_vetor_inteiros(int vetor[],int size);
int devolve_pos_slot_thread_disponivel(int vetor[],int condicao,int max_partidas,int max_chegadas);

void pista_descolagem(int *arg);

void init_semaforo();

void terminate_semaforo();
void print_stats(int signum);

int gettimeofday(struct timeval *tv, struct timezone *tz);

void inicializa_fila_partidas( Partida fila[], int size);
void inicializa_fila_chegadas( Chegada fila[], int size);
int devolve_slot_disp_partida_chegada(int condicao);
void reseta_listas(int id, int vef);

int devolve_pos_fila_partidas( Partida fila[], int size);
int devolve_pos_fila_chegadas( Chegada fila[], int size);

void sort_fila_chegada(Chegada fila_chegada[],int size);
void sort_fila_partida(Partida fila_partida[],int size);

void limpa_slot_partida(Partida fila_partida[], int slot);
void limpa_slot_chegada(Chegada fila_chegada[], int slot);
void *atualiza_fuel(void *arg);

void inicializar_vetor_disp_pista();

int dois_P_um_C(int dep1, int dep2, int arrival1);
int dois_C_um_P(int arrival1, int arrival2, int dep1);
int compara_arrival_dep(int dep1, int arrival1, int dep2, int arrival2, int urg1, int urg2);

 /** MQ Variables **/
int mqid_SM_receive;
int mqid_CT_receive;
int mqid_Slot_receive;


//time variables
time_t rawtime;
struct tm * timeinfo;

int fd;   /**pipe id**/
fd_set read_set;

/**Variaveis memoria partilhada **/
int shmid_time,shmid_stats,shmid_slots_partida,shmid_slots_chegada,shmid_pista,shmid_cond;
int vetor_disp_thread[MAX_C+MAX_P];

int *disp_pista;

Voo_chegada lista_chegadas[MAX_C];
Voo_partida lista_partidas[MAX_P];

Partida *lista_partidas_slot;
Chegada *lista_chegadas_slot;

Estatistica *stats;

double *elapsed;


/**Variaveis de threads **/
pthread_t my_time_thread,my_threads[MAX_C+MAX_P],my_PIPE_thread,my_fuel_up_thread,my_dep_arrival_thread,my_pista_thread;   // A thread responsavel por contar o tempo do programa


/** Variaveis semaforos thread **/

/*Semaforo para escrever no ficheiro entre processos e threads **/

int shmid_file_w;
mem_sem_struct *mem_file;
sem_t *mutex_file_w;

int *condicao_tempo;

int shmid_t_cond;

struct_sh_mem *sh_mem;

pthread_mutex_t mutex_flight_create = PTHREAD_MUTEX_INITIALIZER; /** E executado simplesmente num num excerto de codigo, na funcao work **/

pthread_mutex_t mutex_flight_action = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_sort = PTHREAD_MUTEX_INITIALIZER;
/** SEMAFOROS PARA A PISTA **/
pthread_mutex_t mutex_pista = PTHREAD_MUTEX_INITIALIZER;
sem_t empty_pista;
sem_t full_pista;

/*Semaforo para print */

int shmid_print;
mem_sem_struct *mem_print;
sem_t *mutex_print;






/** Outras Variaveis **/
Config configuracao;
Message mensagem_SLOT,mensagem_SM,mensagem_CT;
int num_processo_CT;
/** Variaveis a ser usadas pelo processo TORRE DE CONTROLO **/
Partida *fila_partida;
Chegada *fila_chegada;


int main(){

    signal(SIGINT,sigint);
    configuracao = ler_ficheiro_configs("config.txt");


    cria_ficheiro_log("log.txt");
    gestor_simulacao();

    return 0;
}

void tira_enter(char str[]){
    if(str[strlen(str)-1]=='\n'){
        str[strlen(str)-1]='\0';
    }
}



void gestor_simulacao(){

    int pos,i;
    char buffer_aux[MAX];
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    struct timespec   ts;
    struct timeval    tp;
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec = configuracao.ut/1000;

    if((shmid_t_cond = shmget(IPC_PRIVATE, sizeof(struct_sh_mem), IPC_CREAT|0700)) <0){
        perror("Error in shmget with IPC_CREAT\n");
    }
    if( (sh_mem = (struct_sh_mem *) shmat(shmid_t_cond, NULL, 0)) == (struct_sh_mem *)-1){
        perror("SHMAT Error sem cond\n");
        exit(1);
    }
    pthread_cond_init(&sh_mem->cond, &cattr);
    pthread_mutex_init(&sh_mem->mutex_time, &mattr);



    if( (shmid_time=shmget(IPC_PRIVATE, sizeof(double), IPC_CREAT|0700)) <0){ /**criacao da shared memory para o tempo corrido desde o inicio do programa**/
        perror("Error in shmget with IPC_CREAT\n");
    }
    if ( (elapsed = (double*)shmat(shmid_time, NULL, 0))==((double *)-1)){
        perror("SHMAT Error\n");
        exit(1);
    }


    if( ( shmid_stats = shmget(IPC_CREAT, sizeof(Estatistica),IPC_CREAT|0700)) <0){  /** criacao da shared memory para as estatiticas **/
        perror("Error in shmget with IPC_CREAT\n");
    }
    if(( stats = (Estatistica*)shmat(shmid_stats,NULL,0)) == (Estatistica*)-1){
        perror("SHMAT ERROR\n");
    }

    /** ------------------------------------------------------------------------------------------**/


    if( (shmid_pista=shmget(IPC_PRIVATE, 6*sizeof(int), IPC_CREAT|0700)) <0){ /**criacao da shared memory para as disponibilidades das pistas**/
        perror("Error in shmget with IPC_CREAT\n");
    }
    if ( (disp_pista = (int*)shmat(shmid_pista, NULL, 0))==((int *)-1)){
        perror("SHMAT Error\n");
        exit(1);
    }


    if( (shmid_cond=shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT|0700)) <0){ /**criacao da shared memory para as disponibilidades das pistas**/
        perror("Error in shmget with IPC_CREAT\n");
    }
    if ( (condicao_tempo = (int*)shmat(shmid_cond, NULL, 0))==((int *)-1)){
        perror("SHMAT Error\n");
        exit(1);
    }
    *condicao_tempo = 0;

    //pthread_cond_init(&cond, NULL);

    /** -----------------------------------------------------------------------------------------**/

    /** VARIAVEIS POR ONDE SE VAI SABER SE VAI FAZER HOLDING, ARRANCAR, DESVIADO, ETC **/

    if( (shmid_slots_partida=shmget(IPC_PRIVATE, (configuracao.max_partidas)*sizeof(Partida), IPC_CREAT|0700)) <0){ /**criacao da shared memory para as slots de PARTIDAS/CHEGADAS/....
                                                                                                                                (tamanho mais reduzido de acordo com o que esta nas configs)  **/
        perror("Error in shmget with IPC_CREAT\n");
    }
    if ( (lista_partidas_slot = (Partida*)shmat(shmid_slots_partida, NULL, 0))==((Partida *)-1)){
        perror("SHMAT Error\n");
        exit(1);
    }
    if( (shmid_slots_chegada=shmget(IPC_PRIVATE, (configuracao.max_chegadas)*sizeof(Chegada), IPC_CREAT|0700)) <0){ /**criacao da shared memory para as slots de PARTIDAS/CHEGADAS/....
                                                                                                                                (tamanho mais reduzido de acordo com o que esta nas configs)  **/
        perror("Error in shmget with IPC_CREAT\n");
    }
    if ( (lista_chegadas_slot= (Chegada*)shmat(shmid_slots_chegada, NULL, 0))==((Chegada *)-1)){
        perror("SHMAT Error\n");
        exit(1);
    }

    /** -----------------------------------------------------------------------------------------**/
    init_stats();
    cria_MQ();     /** Vai CRIAR a MQ **/
    init_semaforo(); /** Vai criar um semaforo **/

    inicializa_vetores(lista_chegadas,lista_partidas);
    inicializa_vetor_inteiros(vetor_disp_thread,MAX_C+MAX_P);

    inicializar_vetor_disp_pista();

    inicializa_fila_partidas(lista_partidas_slot,configuracao.max_partidas);
    inicializa_fila_chegadas(lista_chegadas_slot,configuracao.max_chegadas);
    // CRIACAO DO PROCESS TORRE DO CONTROLO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    signal(SIGINT, SIG_IGN);   //para ignorar o sinal
    if( (num_processo_CT=fork()) == 0 ){
        torre_controlo();
        exit(0);
    }

    signal(SIGUSR1, SIG_IGN);


    /** CRIAR THREADS **/
    if(pthread_create(&my_time_thread,NULL,time_loop,NULL)!=0){   /** Cria a thread responsavel pelo tempo **/
        printf("Deu erro na criacao da thread\n");
    }
    if(pthread_create(&my_PIPE_thread,NULL,pipe_work,NULL)!=0){   /** Cria a thread responsavel pelo PIPE **/
        printf("Deu erro na criacao da thread\n");
    }

    signal(SIGINT,sigint);   //para redirecionar o sinal outra vez
    while(1){

        pthread_mutex_lock(&sh_mem->mutex_time);
        while(*condicao_tempo != 1){
            gettimeofday(&tp, NULL);
            ts.tv_sec = 0;
            pthread_cond_timedwait(&sh_mem->cond,&sh_mem->mutex_time,&ts);
    	}
    	pthread_mutex_unlock(&sh_mem->mutex_time);

        for(i=0;i<MAX_P;i++){
            if(lista_partidas[i].init!=-1){
                if( ( lista_partidas[i].init <= converte_tempo(*elapsed,configuracao.ut) ) && (lista_partidas[i].iniciado==0)){
                    pos = devolve_pos_slot_thread_disponivel(vetor_disp_thread,1,MAX_P,MAX_C);
                    vetor_disp_thread[pos]=pos;
                    lista_partidas[i].iniciado=1;
                    if( pthread_create ( &my_threads[pos] , NULL , work, &vetor_disp_thread[pos] ) != 0 ){
                        printf("Deu erro na criacao da thread\n");
                    }
                    else{
                        buffer_aux[0]='\0';

                        strcat(buffer_aux,"THREAD CREATED SUCESSFULLY => DEPARTURE: ");
                        strcat(buffer_aux,lista_partidas[i].nome);
                        stats->total_voos_criados++;
                        sem_wait(mutex_file_w);
                        escreve_ficheiro_log(buffer_aux);
                        sem_post(mutex_file_w);


                        sem_wait(mutex_print);
                        printf("%s\n",buffer_aux);
                        sem_post(mutex_print);

                        buffer_aux[0]='\0';
                    }
                }
            }
        }
        for(i=0;i<MAX_C;i++){
            if(lista_chegadas[i].init!=-1){
                if( (lista_chegadas[i].init <= converte_tempo(*elapsed,configuracao.ut)) && (lista_chegadas[i].iniciado==0) ){
                    pos = devolve_pos_slot_thread_disponivel(vetor_disp_thread,2,MAX_P,MAX_C);
                    vetor_disp_thread[pos]=pos;
                    lista_chegadas[i].iniciado=1;
                    if( pthread_create ( &my_threads[pos] , NULL , work, &vetor_disp_thread[pos] ) != 0 ){
                        printf("Deu erro na criacao da thread\n");
                    }
                    else{
                        buffer_aux[0]='\0';
                        strcat(buffer_aux,"THREAD CREATED SUCESSFULLY => ARRIVAL: ");
                        strcat(buffer_aux,lista_chegadas[i].nome);
                        stats->total_voos_criados++;
                        sem_wait(mutex_file_w);
                        escreve_ficheiro_log(buffer_aux);
                        sem_post(mutex_file_w);

                        sem_wait(mutex_print);
                        printf("%s\n",buffer_aux);
                        sem_post(mutex_print);

                        buffer_aux[0]='\0';
                    }
                }
            }
        }
    }
}

void cria_ficheiro_log(char nome[]){
    FILE *f;
    char tempo[MAX];
    f = fopen(nome,"w");
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    strcpy(tempo,asctime(timeinfo));
    tira_enter(tempo);
    if(f!=NULL){
        printf("Ficheiro LOG criado com sucesso!\n");
        fprintf(f,"%s THE PROGRAM STARTED\t MY PROCESS ID ==> %d\n",tempo,getpid());
    }
    else{
        printf("ERRO: Nao foi possivel criar o ficheiro LOG!\n");
    }
    fclose(f);
}

void escreve_ficheiro_log(char str[]){
    FILE *f;
    char tempo[MAX];
    f = fopen("log.txt","a");
    time ( &rawtime );

    timeinfo = localtime ( &rawtime );
    strcpy(tempo,asctime(timeinfo));
    tira_enter(tempo);
    if(f!=NULL){
        fprintf(f,"%s %s\n",tempo,str);
    }
    else{
        printf("ERRO: Nao foi possivel escrever no ficheiro LOG!\n");
    }
    fclose(f);

}

void torre_controlo(){
    char buffer[MAX];
    int slot,pos;
    char pid[10];

    signal(SIGUSR1,print_stats);   /**SIGUSR1 SIGNAL **/
    signal(SIGTERM,clean_CT);

    fila_partida = (Partida*)malloc(configuracao.max_partidas*sizeof(Partida));
    fila_chegada = (Chegada*)malloc(configuracao.max_chegadas*sizeof(Chegada));

    inicializa_fila_partidas(fila_partida,configuracao.max_partidas);
    inicializa_fila_chegadas(fila_chegada,configuracao.max_chegadas);


    if(pthread_create(&my_fuel_up_thread,NULL,atualiza_fuel,fila_chegada)!=0){   /** Cria a thread responsavel por atualizar o FUEL do aviao **/
        printf("Deu erro na criacao da thread\n");
    }

    if(pthread_create(&my_dep_arrival_thread,NULL,decides_departure_arrival,NULL)!=0){   /** Cria a thread responsavel por atualizar o FUEL do aviao **/
        printf("Deu erro na criacao da thread\n");
    }

    strcat(buffer,"CONTROL TOWER PROCESS CREATED! MY PROCESS ID ==> ");
    snprintf(pid, 10,"%d",(int)getpid());
    strcat(buffer, pid);

    sem_wait(mutex_file_w);
    escreve_ficheiro_log(buffer);
    sem_post(mutex_file_w);

    sem_wait(mutex_print);
    printf("%s\n",buffer);
    sem_post(mutex_print);

    while(1){
        if(msgrcv(mqid_CT_receive, &mensagem_CT, sizeof(Message), NORMAL, IPC_NOWAIT)==sizeof(Message) || msgrcv(mqid_CT_receive, &mensagem_CT, sizeof(Message), PRIORITARIO, IPC_NOWAIT)==sizeof(Message)){  /*  VAI RECEBER OU NORMAL OU PRIORITARIO*/

            if( mensagem_CT.condicao == 1){  /** Para identificar que o voo e de partida **/

                slot = devolve_slot_disp_partida_chegada(1);    /** Para nao percorrer o vetor vezes desnecessarias **/

                if( slot !=- 1){ /** significa que ainda ha posicoes livres **/
                    lista_partidas_slot[slot].estado = 0;    /** ---> Este vai ser o estado de inatividade **/
                    lista_partidas_slot[slot].slot = slot;
                    lista_partidas_slot[slot].departure = mensagem_CT.departure;
                    mensagem_SLOT.msgtype=NORMAL;
                    mensagem_SLOT.slot = slot;     /** Para enviar o slot correspondente em memoria partilhada **/

                    pos = devolve_pos_fila_partidas(fila_partida,configuracao.max_partidas);
                    fila_partida[pos].slot = slot;
                    fila_partida[pos].estado = 0;
                    fila_partida[pos].departure = mensagem_CT.departure;
                    sort_fila_partida(fila_partida,configuracao.max_partidas);

                    msgsnd(mqid_Slot_receive, &mensagem_SLOT, sizeof(Message), IPC_NOWAIT);

                    slot = -1;
                    pos = -1;
                }
                else{
                    printf("Limite partidas alcancado \n");
                    mensagem_SLOT.msgtype=NORMAL;
                    mensagem_SLOT.slot = -1;
                    msgsnd(mqid_Slot_receive, &mensagem_SLOT, sizeof(Message), IPC_NOWAIT);
                    slot = -1;

                }
            }
            else if( mensagem_CT.condicao == 2 ){ /** Para identificar que o voo e de chegada **/

                slot = devolve_slot_disp_partida_chegada(2);    /** Para nao percorrer o vetor vezes desnecessarias **/

                if( slot !=- 1){ /** significa que ainda ha posicoes livres **/
                    lista_chegadas_slot[slot].estado = 0;    /** ---> Este vai ser o estado de inatividade **/
                    lista_chegadas_slot[slot].slot = slot;
                    lista_chegadas_slot[slot].eta = mensagem_CT.eta;

                    mensagem_SLOT.msgtype=NORMAL;
                    mensagem_SLOT.slot = slot;

                    pos = devolve_pos_fila_chegadas(fila_chegada,configuracao.max_chegadas);

                    fila_chegada[pos].slot = slot;
                    fila_chegada[pos].eta = mensagem_CT.eta;
                    fila_chegada[pos].fuel = mensagem_CT.fuel;

                    if(mensagem_CT.msgtype==PRIORITARIO){
                        fila_chegada[pos].estado = 5;     /** ESTADO PRIORITARIO **/


                        sort_fila_chegada(fila_chegada,configuracao.max_chegadas);
			sort_fila_chegada_prioridade(fila_chegada,configuracao.max_chegadas);

                    }
                    else{
                        fila_chegada[pos].estado = 0;   /** ESTADO NORMAL **/

                        sort_fila_chegada(fila_chegada,configuracao.max_partidas);
			sort_fila_chegada_prioridade(fila_chegada,configuracao.max_chegadas);
                    }

                    msgsnd(mqid_Slot_receive, &mensagem_SLOT, sizeof(Message), IPC_NOWAIT);

                    slot = -1;
                    pos = -1;
                }
                else{
                    mensagem_SLOT.msgtype=NORMAL;
                    mensagem_SLOT.slot = -1;
                    msgsnd(mqid_Slot_receive, &mensagem_SLOT, sizeof(Message), IPC_NOWAIT);
                    slot = -1;
                }
            }
            else{
                printf("A condicao de mensagem enviada nao foi a correta\n");
            }
        }
    }
}

int compara_arrival_dep(int dep1, int arrival1, int dep2, int arrival2, int urg1, int urg2){ /** Para decidir quem e que vai descolar ou aterrar primeiro **/

    int output1,output2;

    if(urg1 == 5){
        if( urg2 == 5){
            return 4;
        }
        else{
            if(arrival2 != 99999){ /** Para o caso de APENAS um ser de urgencia e o outro nao **/
                if(arrival2 < arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens){
                    return 4;
                }
                else{
                    return 2;
                }
            }
            else{
                return 2;
            }

        }
    }

    if(dep1 != 99999){

        if(dep2 != 99999){

            if( arrival1 != 99999){

                if( arrival2 != 99999){

                        /** EXISTEM 2 VOOS DE CADA **/

                    if(  dep2 <  dep1 + configuracao.dur_descolagem + configuracao.int_descolagem){
                        if( arrival2 <  arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens){

                            /** Deste caso, vao sair ou 2 de CHEGADA ou 2 de PARTIDA **/

                            output1 = dep2 + configuracao.dur_descolagem + configuracao.int_descolagem;
                            output2 = arrival2 + configuracao.dur_aterragem + configuracao.int_aterragens;

                            /** Neste IF e neste ELSE vamos calcular o tempo total caso saiam primeiro os de PARTIDA **/


                            if(output1 > arrival2){

                                output1 = output1 + configuracao.dur_aterragem + configuracao.int_aterragens;

                            }
                            else{

                                output1 = arrival2 + configuracao.dur_aterragem + configuracao.int_aterragens;

                            }
                            /** Neste IF e neste ELSE vamos calcular o tempo total caso saiam primeiro os de CHEGADA **/

                            if( output2 > dep2){

                                /** Este caso significa que o ETA ja "passou" e pode aterrar (isto a nivel de contas ) **/

                                output2 = output2 + configuracao.dur_descolagem + configuracao.int_descolagem;

                            }
                            else{
                                /** Este caso significa que o ETA ainda nao "passou" e por isso ainda nao pode aterrar (isto a nivel de contas ) **/
                                output2 = dep2 + configuracao.dur_descolagem + configuracao.int_descolagem;

                            }

                            if(output1 < output2){
                                return 3;  /** Saiem 2 de CHEGADA **/
                            }
                            else{
                                return 4; /** Saiem 2 de CHEGADA **/
                            }

                        }
                        else{ //2P - 1C
                            return dois_P_um_C(dep1,dep2,arrival1);
                        }

                    }
                    else{
                        if(  arrival2 < arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens){ // 1P - 2C
                            return dois_C_um_P(arrival1,arrival2,dep1);
                        }
                        else{

                            if( dep1 + configuracao.dur_descolagem + configuracao.int_descolagem < arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens){
                                return 1; /** Vai para a pista PRIMEIRO o voo de PARTIDA **/
                            }
                            else{
                                return 2;    /** Vai para a pista PRIMEIRO o voo de CHEGADA **/
                            }
                        }
                    }
                }

                else{    /** Aqui temos 2 VOOS PARTIDA e 1 CHEGADA **/
                    return dois_P_um_C(dep1,dep2,arrival1);

                }

            }

            else{

                 /** Significa que SO EXISTEM 2 voos de partida nas  duas filas **/
                 if( dep2 > dep1 + configuracao.dur_descolagem ){

                    return 1; /** So sai 1 de PARTIDA **/
                 }

                 else{

                    return 3; /** Saiem os 2 de PARTIDA **/

                 }
            }

        }

        else{

           if( arrival1 != 99999 ) {

                if( arrival2 != 99999 ) {
                    /** Neste ponto aqui temos 1 Voo de PARTIDA e 2 voos de CHEGADA  **/

                    return dois_C_um_P(arrival1,arrival2,dep1);
                }

                else{       /** Significa que existe apenas 1 Voo de PARTIDA e 1 Voo de CHEGADA **/

                    if( dep1 + configuracao.dur_descolagem + configuracao.int_descolagem < arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens){
                        return 1; /** Vai para a pista PRIMEIRO o voo de PARTIDA **/
                    }
                    else{
                        return 2;    /** Vai para a pista PRIMEIRO o voo de CHEGADA **/
                    }
                }
            }

            else{
                return 1;    /** Significa que SO EXISTE 1 voo de partida nas  duas filas **/
            }

        }


    }
    else if(arrival1 != 99999){

        if(arrival2 != 99999){
            /**Significa que SO EXISTEM 2 voos de chegada nas  duas filas**/

            if(arrival2 > arrival1 + configuracao.dur_aterragem){
                return 2;
            }
            else{
                return 4;
            }

        }
        else{
            return 2;         /** Significa que SO EXISTE 1 voo de chegada naS duas filas **/
        }
    }
    else{

        return -1;    /** PARA O CASO DE NAO HAVER NADA **/

    }


}

int dois_C_um_P(int arrival1, int arrival2, int dep1){
    int output1,output2;

    if( arrival2 > arrival1 + configuracao.dur_aterragem ){
        /** Significa que de certa forma e indiferente sairem os 2 dois de chegada "ao mesmo tempo" **/

        if( dep1 + configuracao.dur_descolagem + configuracao.int_descolagem < arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens ){

            /** Sai PRIMEIRO o de PARTIDA **/
            return 1;

        }

        else{
            /** Sai PRIMEIRO o de CHEGADA e apenas esse!!!!!!!! **/
            return 2;
        }

    }
    else{

        /** Aqui vamos calcular quanto tempo demoraria caso saisse 1o o de PARTIDA e depois os outros, e caso saisse 1o os 2 de CHEGADA e apenas depois o de PARTIDA **/


        /** Neste IF e neste ELSE vamos calcular o tempo total caso saia primeiro o de PARTIDA **/
        if(dep1 + configuracao.dur_descolagem + configuracao.int_descolagem > arrival1){

            output1 = dep1 + configuracao.dur_descolagem + configuracao.int_descolagem + ( arrival2 - arrival1 ) + configuracao.dur_aterragem;

        }
        else{

            output1 = arrival1 + configuracao.dur_aterragem + ( arrival2 - arrival1 );

        }
        /** Neste IF e neste ELSE vamos calcular o tempo total caso saiam primeiro os de CHEGADA **/

        if( arrival2 + configuracao.dur_aterragem + configuracao.int_aterragens < dep1){

            /** Este caso significa que o DEP ja "passou" e pode descolar (isto a nivel de contas ) **/

            output2 = arrival2 + configuracao.dur_aterragem + configuracao.int_aterragens + configuracao.dur_descolagem;

        }
        else{
            /** Este caso significa que o DEP ainda nao "passou" e por isso ainda nao pode descolar (isto a nivel de contas ) **/
            output2 = dep1 + configuracao.dur_descolagem;

        }

        if( output1 < output2){
            /** Vai descolar apenas um VOO de PARTIDA **/
            return 1;

        }
        else{
            /** Vao aterrar os dois VOOS de CHEGADA **/
            return 4;
        }
    }

}

int dois_P_um_C(int dep1, int dep2, int arrival1){
    int output1,output2;
    if( dep2 > dep1 + configuracao.dur_descolagem ){
            /** Significa que de certa forma e indiferente sairem os 2 dois de chegada "ao mesmo tempo" **/

        if(arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens  < dep1 + configuracao.dur_descolagem + configuracao.int_descolagem ){

            /** Sai PRIMEIRO o de CHEGADA **/
            return 2;

        }

        else{

            /** Sai PRIMEIRO o de PARTIDA e apenas esse!!!!!!!! **/
            return 1;
        }

    }
    else{

        /** Aqui vamos calcular quanto tempo demoraria caso saisse 1o o de CHEGADA e depois os outros, e caso saisse 1o os 2 de PARTIDA e apenas depois o de CHEGADA **/


        /** Neste IF e neste ELSE vamos calcular o tempo total caso saia primeiro o de CHEGADA **/
        if(arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens > dep1){

            output1 = arrival1 + configuracao.dur_aterragem + configuracao.int_aterragens + ( dep2 - dep1 ) + configuracao.dur_descolagem;

        }
        else{

            output1 = dep1 + configuracao.dur_descolagem + ( dep2 - dep1 );

        }
        /** Neste IF e neste ELSE vamos calcular o tempo total caso saiam primeiro os de PARTIDA **/

        if( dep2 + configuracao.dur_descolagem + configuracao.int_descolagem < arrival1){

            /** Este caso significa que o ETA ja "passou" e pode aterrar (isto a nivel de contas ) **/

            output2 = dep2 + configuracao.dur_descolagem + configuracao.int_descolagem + configuracao.dur_aterragem;

        }
        else{
            /** Este caso significa que o ETA ainda nao "passou" e por isso ainda nao pode aterrar (isto a nivel de contas ) **/
            output2 = arrival1 + configuracao.dur_aterragem;

        }

        if( output1 < output2){
            /** Vai ATERRAR apenas um VOO de CHEGADA **/
            return 2;

        }
        else{
            /** Vao DESCOLAR os dois VOOS de PARTIDA **/
            return 3;
        }

    }

}

static void clean_MQ(){

    msgctl(mqid_SM_receive, IPC_RMID, 0);
    msgctl(mqid_CT_receive, IPC_RMID, 0);

}
/** CRIA MESSAGE Queue **/

static void cria_MQ(){

  /* Cria uma message queue */
    mqid_CT_receive = msgget(IPC_PRIVATE, IPC_CREAT|0700);
    mqid_SM_receive = msgget(IPC_PRIVATE, IPC_CREAT|0700);
    mqid_Slot_receive = msgget(IPC_PRIVATE, IPC_CREAT|0700);


    if (mqid_CT_receive < 0 || mqid_SM_receive <0 || mqid_Slot_receive<0){
        perror("ERRO: Ao criar a message queue\n");
        exit(0);
    }
}

Config ler_ficheiro_configs(char nome[]){
    FILE *f;
    int i,linhas=1,primeira_linha=1,k=0;
    char str[MAX],cad[MAX],cad2[MAX];  /* vamos ler o ficheiro de configs */

    Config configuracao;

    f = fopen(nome,"r");


    if(f!=NULL){

        while(!feof(f)){
            fgets(str,MAX,f);
            tira_enter(str);
            if( linhas == 1 ){
                configuracao.ut = atoi(str);

            }
            else if( linhas == 5 ){
                configuracao.max_partidas = atoi(str);
            }
            else if( linhas == 6 ){
                configuracao.max_chegadas = atoi(str);
            }
            else{
                for(i=0;str[i]!='\0';i++){
                    if( str[i] != ',' ){
                        if(primeira_linha==1)
                            cad[i]=str[i];
                        else{
                            cad2[k]=str[i];
                            k++;
                        }
                    }
                    else{
                        if( linhas == 2 ){

                            if( primeira_linha == 1)
                                configuracao.dur_descolagem = atoi(cad);
                            else{

                                configuracao.int_descolagem= atoi(cad2);
                            }

                            primeira_linha++;

                        }
                        else if( linhas == 3 ){

                            if( primeira_linha == 1)
                                configuracao.dur_aterragem = atoi(cad);
                            else{
                                configuracao.int_aterragens = atoi(cad2);
                            }
                            primeira_linha++;
                        }
                        else if( linhas == 4 ){

                            if( primeira_linha == 1)
                                configuracao.hold_min = atoi(cad);
                            else{
                                configuracao.hold_max = atoi(cad2);
                            }
                            primeira_linha++;
                        }
                    }
                }
                primeira_linha=1;
                k=0;

            }
            linhas++;
        }
    }
    fclose(f);
    return configuracao;
}

void inicializa_vetores(Voo_chegada lista_chegadas[],Voo_partida lista_partidas[]){
    int i;
    for(i=0;i<MAX_P;i++){
        lista_partidas[i].init=-1;
    }

    for(i=0;i<MAX_C;i++){
        lista_chegadas[i].init=-1;
    }
}


double converte_tempo(double elapsed,int ut){
    return elapsed/(ut*0.001);
}


void adiciona_aovetor_partidas(Voo_partida vetor_partidas[], int max, Voo_partida a_adicionar){
    int i;
    int vef=0;
    for(i=0;i<max;i++){
        if(vef==0){
            if(vetor_partidas[i].init==-1){
                vetor_partidas[i] = a_adicionar;
                vef=1;
            }
        }
    }
}

void adicionar_aovetor_chegadas(Voo_chegada vetor_chegadas[], int max, Voo_chegada a_adicionar){
    int i;
    int vef=0;
    for(i=0;i<max;i++){
        if(vef==0){
            if(vetor_chegadas[i].init==-1){
                vetor_chegadas[i] = a_adicionar;
                vef=1;
            }
        }
    }
}



int ver_fuel(int init, int eta, int fuel){
    if( (fuel -( eta - init) ) >= 0 ){
        return 1;      /** Devolve 1 caso o FUEL chegue para aterrar de acordo com o ETA **/
    }
    else{
        return 0;
    }
}

int contador_vetor_partidas(Voo_partida vetor_partidas[], int max){
    int contador_partidas=0,i;

    for(i=0;i<max;i++){
        if(vetor_partidas[i].init!=-1){
            contador_partidas++;
        }
    }
    return contador_partidas;

}

int contador_vetor_chegadas(Voo_chegada vetor_chegadas[], int max){
    int contador_chegadas=0,i;

    for(i=0;i<max;i++){
        if(vetor_chegadas[i].init!=-1){
            contador_chegadas++;
        }
    }
    return contador_chegadas;
}

int devolve_pos_slot_thread_disponivel(int vetor[],int condicao,int max_partidas,int max_chegadas){
    int i;
    if(condicao==1){
        if(vetor[0]==-1){
            return 0;
        }
        for(i=1;i<max_partidas;i++){     /** Se condicao ==1 estamos a ver os slots disponiveis para as partidas, se ==2 e para as chegadas **/
            if(vetor[i]==0){        /** e devolvemos o indice da posicao disponivel **/
                return i;
            }
        }
    }
    else if(condicao == 2){
        for(i=max_partidas;i<max_chegadas+max_partidas;i++){
            if(vetor[i]==0){
                return i;
            }
        }
    }
    return -1;
}

void inicializa_vetor_inteiros(int vetor[],int size){
    int i;
    for(i=0;i<size;i++){
        vetor[i]=0;
    }
    vetor[0]=-1;
}

int verifica_prioritario(int eta, int fuel, int init){
    int verifica;
    verifica = fuel - configuracao.dur_aterragem - ( eta - init ) ;
    if ( verifica <= 4){
        return 1;       /** Significa que este voo e considerado PRIORITARIO **/
    }
    else{
        return 0;       /** NAO E PRIORITARIO **/
    }
}

void *work(void *ids){
    int id = *((int *)ids); /** Para nao ser usado, porque e void **/
    char buffer_aux[MAX],str[MAX];
    Message mensagem_aux;
    int vef,estado;


    pthread_mutex_lock(&mutex_flight_create); /** Isto e para a thread receber imediatamente o seu slot da MQ, portanto neste pedaco de codigo so pode estar uma thread de cada vez **/
    vef=1;
    if(id<MAX_P){                          /**Isto significa que a thread corresponde a um voo de partida **/
        mensagem_aux.condicao=1;
        mensagem_aux.departure=lista_partidas[id].descolagem;

        mensagem_aux.msgtype=NORMAL;
        msgsnd(mqid_CT_receive, &mensagem_aux, sizeof(Message), IPC_NOWAIT);

    }
    else{                               /**Isto significa que a thread corresponde a um voo de chegada **/
        if( verifica_prioritario(lista_chegadas[id-MAX_P].eta,lista_chegadas[id-MAX_P].fuel,lista_chegadas[id-MAX_P].init) == 1){    /** Significa que e prioritario **/

            strcat(buffer_aux,"FLIGHT ");
            strcat(buffer_aux,lista_chegadas[id-MAX_P].nome);
            strcat(buffer_aux," EMERGENCY LANDING REQUESTED ");

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer_aux);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer_aux);
            sem_post(mutex_print);

            buffer_aux[0] = '\0';
            mensagem_aux.condicao = 2;
            mensagem_aux.eta=lista_chegadas[id-MAX_P].eta;
            mensagem_aux.fuel=lista_chegadas[id-MAX_P].fuel;

            mensagem_aux.msgtype=PRIORITARIO;

            msgsnd(mqid_CT_receive, &mensagem_aux, sizeof(Message), IPC_NOWAIT);

        }
        else{
            mensagem_aux.condicao = 2;
            mensagem_aux.eta=lista_chegadas[id-MAX_P].eta;

            mensagem_aux.fuel=lista_chegadas[id-MAX_P].fuel;

            mensagem_aux.msgtype=NORMAL;

            msgsnd(mqid_CT_receive, &mensagem_aux, sizeof(Message), IPC_NOWAIT);

        }

    }
    while(vef!=0){    /**Enquanto nao receber a mensagem do seu slot  na memoria reservada vai ficar a espera**/
        if(msgrcv(mqid_Slot_receive, &mensagem_SLOT, sizeof(Message), NORMAL, IPC_NOWAIT)==sizeof(Message)){
            vef=0;           /** da Torre de COntrolo que lhe envie mensagem relativamente a isso, para poder seguir com a seu trabalho ou terminar **/
        }
    }
    if(id<MAX_P){
        lista_partidas[id].slot=mensagem_SLOT.slot;
        if( lista_partidas[id].slot == -1){   /** Se for -1 e pq nao ha espaco na lista para ter um novo voo **/
            strcat(buffer_aux,"THREAD TERMINATED BECAUSE DEPARTURE FLIGHTS LIMIT REACHED: ");
            strcat(buffer_aux,lista_partidas[id].nome);

            stats->voos_rejeitados++;

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer_aux);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer_aux);
            sem_post(mutex_print);

            buffer_aux[0]='\0';
            reseta_listas(id,1);
            pthread_mutex_unlock(&mutex_flight_create);           /** DAMOS UNLOCK ANTES DE ELIMINAR A THREAD, QUE E PARA AS OUTRAS THREADS NAO FICAREM PRESAS EM CIMA **/
            pthread_exit(NULL);                                         /** Vou eliminar a thread **/
        }
    }
    else{
        lista_chegadas[id-MAX_P].slot=mensagem_SLOT.slot;
        if( lista_partidas[id-MAX_P].slot == -1){   /** Se for -1 e pq nao ha espaco na lista para ter um novo voo **/
            strcat(buffer_aux,"THREAD TERMINATED BECAUSE ARRIVAL FLIGHTS LIMIT REACHED: ");
            strcat(buffer_aux,lista_chegadas[id-MAX_P].nome);

            stats->voos_rejeitados++;

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer_aux);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer_aux);
            sem_post(mutex_print);

            buffer_aux[0]='\0';
            reseta_listas(id,2);
            pthread_mutex_unlock(&mutex_flight_create);         /** DAMOS UNLOCK ANTES DE ELIMINAR A THREAD, QUE E PARA AS OUTRAS THREADS NAO FICAREM PRESAS EM CIMA **/
            pthread_exit(NULL);                                     /** Vou eliminar a thread **/
        }
    }

    pthread_mutex_unlock(&mutex_flight_create);

    while(1)
    {
        pthread_mutex_lock(&mutex_flight_action);

        if(id<MAX_P){

            estado = lista_partidas_slot[ lista_partidas[id].slot ].estado; /** Vai armazenar o estado e depois comparar com uma series de estados **/

            if( estado == 1 ){

                pthread_mutex_unlock(&mutex_flight_action);         /** DAMOS UNLOCK ANTES DE ELIMINAR A THREAD, QUE E PARA AS OUTRAS THREADS NAO FICAREM PRESAS EM CIMA **/;
                pista_descolagem(&id);
                stats->total_voos_descolaram = stats->total_voos_descolaram + 1;
                reseta_listas(id,1);
                elimina_slot_lista_memoria_partilhada(lista_partidas[ id ].slot,1);
                pthread_exit(NULL);
            }
        }
        else{

            estado = lista_chegadas_slot[ lista_chegadas[ id - MAX_P ].slot ].estado; /** Vai armazenar o estado e depois comparar com uma series de estados **/

            if(estado > 1 ){

                if(estado==2){
                    pthread_mutex_unlock(&mutex_flight_action);         /** DAMOS UNLOCK ANTES DE ELIMINAR A THREAD, QUE E PARA AS OUTRAS THREADS NAO FICAREM PRESAS EM CIMA **/
                    pista_descolagem(&id);
                    stats->total_voos_aterraram = stats->total_voos_aterraram + 1;
                    reseta_listas(id,2);
                    elimina_slot_lista_memoria_partilhada(lista_chegadas[ id - MAX_P ].slot,2);

                    pthread_exit(NULL);
                }
                else if( estado == 3){

                    strcat(buffer_aux,lista_chegadas[id-MAX_P].nome);
                    strcat(buffer_aux," HOLDING ");
                    snprintf(str, 10,"%d",(int)(lista_chegadas_slot[ lista_chegadas[ id - MAX_P ].slot ].eta - lista_chegadas[id - MAX_P].eta  ));
                    strcat(buffer_aux,str);
                    lista_chegadas[id - MAX_P].eta = lista_chegadas_slot[ lista_chegadas[ id - MAX_P ].slot ].eta;
                    lista_chegadas[id - MAX_P].fuel = -5;   /** Significa que ja fez holding **/
                    sem_wait(mutex_file_w);
                    escreve_ficheiro_log(buffer_aux);
                    sem_post(mutex_file_w);

                    sem_wait(mutex_print);
                    printf("%s\n",buffer_aux);
                    sem_post(mutex_print);

                    buffer_aux[0]='\0';
                    lista_chegadas_slot[ lista_chegadas[ id - MAX_P ].slot ].estado=0;
                }
                else if( estado == 4){
                    strcat(buffer_aux,lista_chegadas[id-MAX_P].nome);
                    strcat(buffer_aux," LEAVING TO OTHER AIRPORT ==>  FUEL = 0");

                    stats->voos_redirecionados++;

                    if(lista_chegadas[id - MAX_P].fuel==-5){ /** SIgnifica que o combustivel chegou a 0 pq fez HOLDING **/
                        strcat(buffer_aux," \t( MADE HOLDING MOVES )");
                    }

                    sem_wait(mutex_file_w);
                    escreve_ficheiro_log(buffer_aux);
                    sem_post(mutex_file_w);

                    sem_wait(mutex_print);
                    printf("%s\n",buffer_aux);
                    sem_post(mutex_print);

                    buffer_aux[0]='\0';
                    reseta_listas(id,2);

                    elimina_slot_lista_memoria_partilhada(lista_chegadas[ id - MAX_P ].slot,2);

                    pthread_mutex_unlock(&mutex_flight_action);         /** DAMOS UNLOCK ANTES DE ELIMINAR A THREAD, QUE E PARA AS OUTRAS THREADS NAO FICAREM PRESAS EM CIMA **/
                    pthread_exit(NULL);
                    printf("  -----------> DEVIA TER SAIDO AQUI <----------- \n ");
                }

            }
        }

        pthread_mutex_unlock(&mutex_flight_action);
    }

}

void *time_loop(void* id){            /**FUncao que calcula tempo passado desdo o inicio do programa **/
    time_t start, end;
    double ant=0;
    time(&start);


    do {

        time(&end);
        *elapsed = difftime(end, start);
        if(converte_tempo(*elapsed,configuracao.ut) >= ant + 1){
            pthread_mutex_lock(&sh_mem->mutex_time);
            *condicao_tempo=1;
            //printf("Condicao_tempo = %d\n",*condicao_tempo);

        	pthread_cond_broadcast(&sh_mem->cond);
        	pthread_mutex_unlock(&sh_mem->mutex_time);
        	ant++;

        }
    }while(1);
}

void *atualiza_fuel(void *arg){
    int i,pos;
    Chegada *fila_chegada =  (Chegada *)arg;

    struct timespec   ts;
    struct timeval    tp;
    gettimeofday(&tp, NULL);
    ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec = configuracao.ut/1000;
    while(1){
        pthread_mutex_lock(&sh_mem->mutex_time);

        while(*condicao_tempo != 1){
            gettimeofday(&tp, NULL);
            ts.tv_sec = 0;
            pthread_cond_timedwait(&sh_mem->cond,&sh_mem->mutex_time,&ts);
    	}
    	*condicao_tempo=0;
    	pthread_mutex_unlock(&sh_mem->mutex_time);
        for(i=0;i<configuracao.max_chegadas;i++){

            if(fila_chegada[i].slot != -1){    /** VAI ATUALIZAR O FUEL , SE O FUEL CHEGAR A 0, VAMOS ATRIBUIR A LISTA O VALOR 4 = DESVIADO**/

                fila_chegada[i].fuel = fila_chegada[i].fuel - 1;


                if(fila_chegada[i].fuel == 0){
                    pos = fila_chegada[i].slot;
                    lista_chegadas_slot[pos].estado = 4; /** ESTADO 4 CORRESPONDE AO DESVIADO**/
                    limpa_slot_chegada(fila_chegada,fila_chegada[i].slot);

                    sort_fila_chegada(fila_chegada,configuracao.max_chegadas);
		    sort_fila_chegada_prioridade(fila_chegada,configuracao.max_chegadas);
                }
            }
        }
    }
}

void limpa_slot_chegada(Chegada fila_chegada[], int slot ){
    /** ZERAR A POSICAO DA FILA DE CHEGADAS , SE O AVIAO FOR DESVIADO **/
    int i;

    for(i=0;i<configuracao.max_chegadas;i++){
        if(fila_chegada[i].slot==slot){
            fila_chegada[i].slot = -1;
            fila_chegada[i].estado = -1;
            fila_chegada[i].eta = 99999;
            fila_chegada[i].fuel = 99999;
        }
    }
    sort_fila_chegada(fila_chegada,configuracao.max_chegadas);
    sort_fila_chegada_prioridade(fila_chegada,configuracao.max_chegadas);
}

void limpa_slot_partida(Partida fila_partida[], int slot){
    /** ZERAR A POSICAO DA FILA DE PARTIDA , SE O AVIAO FOR DESVIADO **/
    int i;

    for(i=0;i<configuracao.max_partidas;i++){
        if(fila_partida[i].slot==slot){
            fila_partida[i].slot = -1;
            fila_partida[i].estado = -1;
            fila_partida[i].departure = 99999;
        }
    }

    sort_fila_partida(fila_partida,configuracao.max_partidas);
}

void *pipe_work(void *arg){
    char buffer_rcv[MAX],buffer_aux[MAX];
    Voo_chegada voo_chegada_aux;
    Voo_partida voo_partida_aux;

    if((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) && (errno!=EEXIST)){  // criar o PIPE se nao ainda tiver sido criado nenhum
        perror("Erro ao criar o pipe\n");
        exit(0);
    }


    if(( fd = open(PIPE_NAME, O_RDONLY)) < 0){

        perror("Nao consegue abrir o PIPE para ler\n");    // abrir o pipe para ler
    }

    while(1){

        FD_ZERO(&read_set);   // E 0 pq so nos interessa a leitura!!!!!!!
        FD_SET(fd, &read_set);
        if(select(fd+1,&read_set,NULL,NULL,NULL)){ // para ver se esta disponivel, senao bloqueia
            if(FD_ISSET(fd,&read_set)){
                if(read(fd, &buffer_rcv, sizeof(buffer_rcv))>0){
                    strcat(buffer_aux,"NEW COMMAND => ");
                    strcat(buffer_aux,buffer_rcv);

                    sem_wait(mutex_file_w);
                    escreve_ficheiro_log(buffer_aux); //vai escrever no log.txt
                    sem_post(mutex_file_w);

                    sem_wait(mutex_print);
                    printf("%s\n",buffer_aux);
                    sem_post(mutex_print);

                    buffer_aux[0]= '\0';  //para "limpar" o buffer auxiliar

                    if ( strncmp(buffer_rcv, "ARRIVAL", 7) == 0) {

                        voo_chegada_aux = comando_voo_chegada(buffer_rcv);      /** Vai validar o comando **/
                        if( contador_vetor_chegadas(lista_chegadas,MAX_C) <= MAX_C ){   /** Vai verificar se o numero maximo de chegadas foi atingido **/
                            if(voo_chegada_aux.init==-1){     /** Se for igual a -1 e porque e invalido **/
                                strcat(buffer_aux,"WRONG COMMAND => ");
                                strcat(buffer_aux,buffer_rcv);

                                sem_wait(mutex_file_w);
                                escreve_ficheiro_log(buffer_aux);
                                sem_post(mutex_file_w);

                                sem_wait(mutex_print);
                                printf("%s\n",buffer_aux);
                                sem_post(mutex_print);

                                buffer_aux[0]= '\0';
                            }
                            else{
                                adicionar_aovetor_chegadas(lista_chegadas,MAX_C,voo_chegada_aux); /** Se o init for != -1 vai adicionar ao vetor o voo **/
                            }
                        }
                        else{    /** Se o numero maximo de chegyadas foi atingido vai escrever no log **/
                            strcat(buffer_aux,"CANNOT ADD FLIGHT (MAX REACHED) => ");
                            strcat(buffer_aux,buffer_rcv);

                            sem_wait(mutex_file_w);
                            escreve_ficheiro_log(buffer_aux);
                            sem_post(mutex_file_w);

                            sem_wait(mutex_print);
                            printf("%s\n",buffer_aux);
                            sem_post(mutex_print);

                            buffer_aux[0]= '\0';
                        }
                    }
                    else if( strncmp(buffer_rcv, "DEPARTURE", 9) == 0 ){
                        voo_partida_aux=comando_voo_partida(buffer_rcv);    /** Vai validar o comando **/
                        if( contador_vetor_partidas(lista_partidas,MAX_P) <= MAX_P ){       /** Vai verificar se o numero maximo de partidas foi atingido **/
                            if(voo_partida_aux.init==-1){    /** Se for igual a -1 e porque e invalido **/
                                strcat(buffer_aux,"WRONG COMMAND => ");
                                strcat(buffer_aux,buffer_rcv);

                                sem_wait(mutex_file_w);
                                escreve_ficheiro_log(buffer_aux);
                                sem_post(mutex_file_w);

                                sem_wait(mutex_print);
                                printf("%s\n",buffer_aux);
                                sem_post(mutex_print);

                                buffer_aux[0]= '\0';
                            }
                            else{
                                adiciona_aovetor_partidas(lista_partidas,MAX_P,voo_partida_aux);  /** Se o init for != -1 vai adicionar ao vetor o voo **/
                            }
                        }
                        else{      /** Se o numero maximo de partidas foi atingido vai escrever no log **/
                            strcat(buffer_aux,"CANNOT ADD FLIGHT (MAX REACHED) => ");
                            strcat(buffer_aux,buffer_rcv);

                            sem_wait(mutex_file_w);
                            escreve_ficheiro_log(buffer_aux);
                            sem_post(mutex_file_w);

                            sem_wait(mutex_print);
                            printf("%s\n",buffer_aux);
                            sem_post(mutex_print);

                            buffer_aux[0]= '\0';
                        }
                    }
                    else{
                            strcat(buffer_aux,"WRONG COMMAND => ");
                            strcat(buffer_aux,buffer_rcv);

                            sem_wait(mutex_file_w);
                            escreve_ficheiro_log(buffer_aux);
                            sem_post(mutex_file_w);

                            sem_wait(mutex_print);
                            printf("%s\n",buffer_aux);
                            sem_post(mutex_print);

                            buffer_aux[0]= '\0';

                    }
                }

            }
        }
    }
}

/* CRIACAO DO SEMAFORO */
void init_semaforo(){

    /** VAI CRIAR O SEMAFORO PARA DAR PRINT NO ECRA, POIS SO UMA THREAD E QUE O PODE FAZER **/
    shmid_print = shmget(IPC_PRIVATE, sizeof(mem_print),IPC_CREAT|0700);
    if(shmid_print<1){
        printf("shmGET file\n");
        exit(0);
    }
    mem_print = (mem_sem_struct*)shmat(shmid_print,NULL,0);
    if(mem_print < ((mem_sem_struct*) 1)){
        printf("shmat file\n");
        exit(0);
    }
    sem_init(&mem_print->sem_mutex,1,1);
    mutex_print = &mem_print->sem_mutex;



    /** VAI CRIAR O SEMAFORO PARA DAR WRITE NO LOG.TXT, POIS SO "UMA THREAD" E QUE O PODE FAZER **/
    shmid_file_w = shmget(IPC_PRIVATE, sizeof(mem_file),IPC_CREAT|0700);
    if(shmid_file_w<1){
        printf("shmGET file\n");
        exit(0);
    }

    mem_file = (mem_sem_struct *)shmat(shmid_file_w,NULL,0);
    if(mem_file < ((mem_sem_struct*) 1)){
        printf("shmat lll file\n");
        exit(0);
    }

    sem_init(&mem_file->sem_mutex,1,1);
    mutex_file_w = &mem_print->sem_mutex;


    /** ---------------------------------------------------**/
    /** SEMAFOROS PARA A PISTA DE DESCOLAGENS/ATERRAGENS **/
    sem_init(&empty_pista,0,2);
    sem_init(&full_pista,0,0);
    /** ---------------------------------------------------**/

}


void inicializar_vetor_disp_pista(){
    int i;
    for(i=0;i<6;i++){
        disp_pista[i] = 0;
    }
    //disp_pista[1] = 0;
}



void elimina_slot_lista_memoria_partilhada(int slot, int condicao){
    if(condicao == 1){
        lista_partidas_slot[slot].slot=-1;
        lista_partidas_slot[slot].estado = -1;
    }
    else{
        lista_chegadas_slot[slot].slot=-1;
        lista_chegadas_slot[slot].estado = -1;
        lista_chegadas_slot[slot].fuel=-1;
        lista_chegadas_slot[slot].eta=-1;

    }
}

void sigint(int signum){
    char option[2],buffer[MAX];
    int i;

    printf("\nCaracter Ctrl-Z clicado, gostaria de sair do programa? ( s | n ) ");

    scanf("%s",option);
    if(option[0]=='s' || option[0]=='S'){
        buffer[0]='\0';
        strcat(buffer,"\tFIM DO PROGRAMA ");
        escreve_ficheiro_log(buffer);
        sem_wait(mutex_print);
        printf("%s\n",buffer);
        sem_post(mutex_print);

        terminate_semaforo();
        clean_MQ();

        if(vetor_disp_thread[0]==0){
            if(pthread_cancel(my_threads[0])==0){
                pthread_join(my_threads[0],NULL);
                printf("Thread 0 apagada com sucesso\n");
            }
        }
        for(i=1;i<MAX_P+MAX_C;i++){
            if(vetor_disp_thread[i]==i){
                if(pthread_cancel(my_threads[i])==0){
                    pthread_join(my_threads[i],NULL);
                    printf("Thread %d apagada com sucesso\n",i);
                }
            }
        }
        clean_SM();

        if(pthread_cancel(my_PIPE_thread)==0){
            pthread_join(my_PIPE_thread,NULL);
            printf("Thread PIPE apagada com sucesso\n");
        }

        if(pthread_cancel(my_time_thread)==0){

            pthread_join(my_time_thread,NULL);
            printf("Thread TIME apagada com sucesso\n");
        }

        kill(num_processo_CT,SIGTERM);   /** Serve para enviar um SINAL à TORRE DE CONTROLO a dizer que tem de terminar, e proceder entao à limpeza de recursos antes de terminar **/
        wait(NULL);
        printf("\nOK, a sair do programa.....");
        exit(0);
    }
}

void clean_CT(int signum){

    free(fila_partida);
    free(fila_chegada);
    if(pthread_cancel(my_fuel_up_thread)==0){
        pthread_join(my_fuel_up_thread,NULL);
        printf("Thread FUEL apagada com sucesso\n");
    }
    if(pthread_cancel(my_dep_arrival_thread)==0){
        pthread_join(my_dep_arrival_thread,NULL);
        printf("Thread que decide PARTIDAS/CHEGADAS apagada com sucesso\n");
    }

    exit(0);
}

void clean_SM(){
    shmdt(sh_mem);
    shmdt(elapsed);
    shmdt(stats);
    shmdt(disp_pista);
    shmdt(lista_partidas_slot);
    shmdt(lista_chegadas_slot);
    shmdt(condicao_tempo);
    shmctl(shmid_time, IPC_RMID, NULL);
    shmctl(shmid_stats, IPC_RMID, NULL);
    shmctl(shmid_pista, IPC_RMID, NULL);
    shmctl(shmid_slots_partida, IPC_RMID, NULL);
    shmctl(shmid_slots_chegada, IPC_RMID, NULL);
    shmctl(shmid_t_cond,IPC_RMID, NULL);
    shmctl(shmid_cond,IPC_RMID, NULL);
}

/* ELIMINACAO DO SEMAFORO */

void terminate_semaforo(){
    sem_destroy(mutex_file_w);
    shmctl(shmid_file_w, IPC_RMID, NULL);

    sem_destroy(mutex_print);
    shmctl(shmid_print, IPC_RMID, NULL);

    pthread_cond_destroy(&sh_mem->cond);
    pthread_mutex_destroy(&sh_mem->mutex_time);




}

void reseta_listas(int id, int vef ){ /**passar como vef OU 1 OU 2 **/

    if(vef==1){
        if(id==0){
            lista_partidas[id].init = -1;   /** se o vef for =1 vamos resetar a lista das partidas referente ao ID passado**/
            lista_partidas[id].descolagem =-1;
            lista_partidas[id].nome[0] = '\0';
            lista_partidas[id].iniciado = 0;
            lista_partidas[id].slot = -1;

            vetor_disp_thread[id] = -1;
        }
        else{
            lista_partidas[id].init = -1;   /** se o vef for =1 vamos resetar a lista das partidas referente ao ID passado **/
            lista_partidas[id].descolagem =-1;
            lista_partidas[id].nome[0] = '\0';
            lista_partidas[id].iniciado = 0;
            lista_partidas[id].slot = -1;

            vetor_disp_thread[id] = 0;
        }

    }
    if(vef==2){

        lista_chegadas[id-MAX_P].init = -1;   /** se for =2 vamos resetar a lista das chegadas referente ao ID passado**/
        lista_chegadas[id-MAX_P].nome[0] = '\0';
        lista_chegadas[id-MAX_P].eta = -1;
        lista_chegadas[id-MAX_P].fuel = -1;
        lista_chegadas[id-MAX_P].iniciado = 0;
        lista_chegadas[id-MAX_P].slot = -1;

        vetor_disp_thread[id] = 0;

    }


}

void inicializa_fila_partidas( Partida fila[], int size){
    int i;
    for(i=0;i<size;i++){
        fila[i].slot=-1;
        fila[i].departure = 99999;
    }
}

void sort_fila_partida(Partida fila_partida[],int size){

    Partida aux;
    int i,j;

    for (i = 0; i < size; ++i){

        for (j = i + 1; j < size; ++j){

            if (fila_partida[i].departure > fila_partida[j].departure) {
                aux =  fila_partida[i];
                fila_partida[i] = fila_partida[j];
                fila_partida[j] = aux;
            }
        }
    }
}

void sort_fila_chegada(Chegada fila_chegada[],int size){

    Chegada aux;
    int i,j;
    pthread_mutex_lock(&mutex_sort);
    for (i = 0; i < size; ++i){

        for (j = i + 1; j < size; ++j){

            if (fila_chegada[i].eta > fila_chegada[j].eta) {
                aux =  fila_chegada[i];
                fila_chegada[i] = fila_chegada[j];
                fila_chegada[j] = aux;
            }
        }
    }
    pthread_mutex_unlock(&mutex_sort);
}

void sort_fila_chegada_prioridade(Chegada fila_chegada[],int size){

    Chegada aux;
    int i,j;

    pthread_mutex_lock(&mutex_sort);

    for (i = 0; i < size; ++i){

        for (j = i + 1; j < size; ++j){
                                                                            /** Aqui tem-se o cuidado de dar prioridade a um voo de EMERGENCIA, caso que o que esteja antes na fila
                                                                            tenha um ETA + DUR_ATERRAGEM + INT_ATERRAGEM superior ao de EMERGENCIA, passando esse para a frente na fila
                                                                            , porque caso contrario o que nao e de emergencia, teria tempo de aterrar **/
            if ( (fila_chegada[i].estado!=5 && fila_chegada[j].estado==5)  && (  (fila_chegada[i].eta + configuracao.int_aterragens +  configuracao.dur_aterragem ) > fila_chegada[j].eta ) ) {
                aux =  fila_chegada[i];
                fila_chegada[i] = fila_chegada[j];
                fila_chegada[j] = aux;
            }
            else if(fila_chegada[i].estado==5 && fila_chegada[j].estado==5){
                if(fila_chegada[i].eta > fila_chegada[j].eta){
                    aux =  fila_chegada[i];
                    fila_chegada[i] = fila_chegada[j];
                    fila_chegada[j] = aux;
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex_sort);
}

void inicializa_fila_chegadas( Chegada fila[], int size){
    int i;
    for(i=0;i<size;i++){
        fila[i].slot=-1;
        fila[i].eta = 99999;
    }
}

int devolve_slot_disp_partida_chegada(int condicao){
    int i;
    if( condicao == 1){                                     /** Devolve o indice do slot disponivel no vetor de voos de partida ou de chegada, conforme, e se os vetores ja estivem**/
        for(i=0;i<configuracao.max_partidas;i++){               /** FULL devolve -1 **/
            if(lista_partidas_slot[i].slot == -1){
                return i;
            }
        }
        return -1;
    }
    else if( condicao == 2){
        for(i=0;i<configuracao.max_chegadas;i++){
            if(lista_chegadas_slot[i].slot == -1){
                return i;
            }
        }
        return -1;
    }
    else{
        printf("Deu erro a devolver o slot. Nunca devia ter chegado a este print\n");
        return -1;
    }
}

int devolve_pos_fila_partidas( Partida fila[], int size){
    int i;

    for(i=0;i<size;i++){

        if(fila[i].slot==-1){
            return i;
        }
    }
    return -1;        /**Devolve -1 se nao encontrar uma posicao **/
}

int devolve_pos_fila_chegadas( Chegada fila[], int size){
    int i;

    for(i=0;i<size;i++){

        if(fila[i].slot==-1){
            return i;
        }
    }
    return -1;        /**Devolve -1 se nao encontrar uma posicao **/
}

void print_stats(int signum){
    printf("\nNUMERO TOTAL VOOS CRIADOS: %d\n", stats->total_voos_criados);
    printf("NUMERO TOTAL DE VOOS QUE ATERRARAM: %d\n", stats->total_voos_aterraram);
    if(stats->total_voos_aterraram==0){
        printf("TEMPO MEDIO DE ESPERA PARA ATERRAR: 0\n");
    }
    else{
        printf("TEMPO MEDIO DE ESPERA PARA ATERRAR: %f\n", stats->tempo_medio_espera_aterrar/(stats->total_voos_aterraram*1.0));
    }

    printf("NUMERO TOTAL DE VOOS QUE DESCOLARAM: %d\n", stats->total_voos_descolaram);
    if(stats->total_voos_descolaram==0){
        printf("TEMPO MEDIO DE ESPERA PARA DESCOLAR: 0\n");
    }
    else{
        printf("TEMPO MEDIO DE ESPERA PARA DESCOLAR: %f\n", stats->tempo_medio_espera_descolar/(stats->total_voos_descolaram*1.0));
    }
    if(stats->total_voos_aterraram==0){
        printf("NUMERO MEDIO DE MANOBRAS DE HOOLDING POR VOO DE ATERRAGEM: 0\n");
        printf("NUMERO MEDIO DE MANOBRAS DE HOOLDING POR VOO EM ESTADO DE URGENCIA: 0\n");
    }
    else{
        printf("NUMERO MEDIO DE MANOBRAS DE HOOLDING POR VOO DE ATERRAGEM: %f\n", stats->manobras_hoolding_aterragem/(stats->total_voos_aterraram*1.0));
        printf("NUMERO MEDIO DE MANOBRAS DE HOOLDING POR VOO EM ESTADO DE URGENCIA: %f\n", stats->manobras_hoolding_urgencia/(stats->total_voos_aterraram*1.0));
    }

    printf("NUMERO DE VOOS REDIRECIONADOS PARA OUTRO AEROPORTO: %d\n", stats->voos_redirecionados);
    printf("VOOS REJEITADOS PELA TORRE DE CONTROLO: %d\n", stats->voos_rejeitados);
}

int compara_nome(char aux[]){
    int i;
    if(lista_chegadas[0].init!=-1){
        if(strcmp(aux, lista_chegadas[0].nome) == 0){
            return 0;
        }
    }
    for(i=1;i<MAX_C;i++){
        if(lista_chegadas[i].init!=0){
            if(strcmp(aux, lista_chegadas[i].nome) == 0){ /** DEVOLVE 0 SE HA OUTRO IGUAL **/
                return 0;                               /** VERIFICA O NOME DAS CHEGADAS**/
            }
        }

    }
    if(lista_partidas[0].init!=-1){
        if(strcmp(aux, lista_partidas[0].nome) == 0){
            return 0;
        }
    }
    for(i=1;i<MAX_P;i++){    /** VERIFICA O NOME PARA AS PARTIDAS **/
        if(lista_partidas[i].init!=-1){
            if(strcmp(aux, lista_partidas[i].nome) == 0){
                return 0;
            }
        }
    }
    return 1;

}

void pista_descolagem(int *arg){
    int id = *((int *)arg);
    char buffer1[MAX],buffer2[MAX];
    sem_wait(&empty_pista);

    if(id<MAX_P){ /**Significa que e voo de PARTIDA **/

        pthread_mutex_lock(&mutex_pista);
        if(disp_pista[0]==0){ /** PISTA 1L **/
            disp_pista[0]=1;
            buffer1[0] = '\0';
            pthread_mutex_unlock(&mutex_pista);
            strcat(buffer1,lista_partidas[id].nome);
            strcat(buffer1," DEPARTURE 1L started");

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer1);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer1);
            sem_post(mutex_print);

            buffer1[0] = '\0';

            usleep(configuracao.ut*1000*configuracao.dur_descolagem); /** TEMPO QUE DEMORA A DESCOLAR **/

            strcat(buffer1,lista_partidas[id].nome);
            strcat(buffer1," DEPARTURE 1L concluded");

            stats->tempo_medio_espera_descolar = stats->tempo_medio_espera_descolar + converte_tempo(*elapsed,configuracao.ut)-lista_partidas[id].descolagem;

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer1);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer1);
            sem_post(mutex_print);

            buffer1[0] = '\0';
            usleep(configuracao.ut*1000*configuracao.int_descolagem); /** Tempo para se voltar a poder usar a PISTA EM QUESTAO **/
            disp_pista[0]=0;
            disp_pista[4] = disp_pista[4] - 1;

        }
        else if(disp_pista[1]==0){ /** PISTA 1R **/
            disp_pista[1]=1;
            buffer2[0] = '\0';
            pthread_mutex_unlock(&mutex_pista);
            strcat(buffer2,lista_partidas[id].nome);
            strcat(buffer2," DEPARTURE 1R started");

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer2);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer2);
            sem_post(mutex_print);

            buffer2[0] = '\0';

            usleep(configuracao.ut*1000*configuracao.dur_descolagem); /** TEMPO QUE DEMORA A DESCOLAR **/

            strcat(buffer2,lista_partidas[id].nome);
            strcat(buffer2," DEPARTURE 1R concluded");

            stats->tempo_medio_espera_descolar = stats->tempo_medio_espera_descolar + converte_tempo(*elapsed,configuracao.ut)-lista_partidas[id].descolagem;

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer2);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer2);
            sem_post(mutex_print);

            buffer2[0] = '\0';
            usleep(configuracao.ut*1000*configuracao.int_descolagem); /** Tempo para se voltar a poder usar a PISTA EM QUESTAO **/
            disp_pista[1]=0;
            disp_pista[4] = disp_pista[4] - 1;
        }

    }
    else{ /**Significa que e voo de CHEGADA **/
        pthread_mutex_lock(&mutex_pista);
        if(disp_pista[2]==0){ /** PISTA 28L **/
            disp_pista[2]=1;
            buffer1[0] = '\0';
            pthread_mutex_unlock(&mutex_pista);
            strcat(buffer1,lista_chegadas[id-MAX_P].nome);
            strcat(buffer1," ARRIVAL 28L started");

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer1);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer1);
            sem_post(mutex_print);

            buffer1[0] = '\0';

            usleep(configuracao.ut*1000*configuracao.dur_aterragem); /** Tempo que o AVIAO demora a ATERRAR  **/

            strcat(buffer1,lista_chegadas[id-MAX_P].nome);
            strcat(buffer1," ARRIVAL 28L concluded");
            stats->tempo_medio_espera_aterrar = stats->tempo_medio_espera_aterrar + converte_tempo(*elapsed,configuracao.ut)-lista_chegadas[id-MAX_P].eta;

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer1);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer1);
            sem_post(mutex_print);

            buffer1[0] = '\0';
            usleep(configuracao.ut*1000*configuracao.int_aterragens); /** Tempo para se voltar a poder usar a PISTA EM QUESTAO **/
            disp_pista[2]=0;
            disp_pista[5] = disp_pista[5] - 1;
        }
        else if(disp_pista[3]==0){ /** PISTA 28R **/
            disp_pista[3]=1;
            buffer2[0] = '\0';
            pthread_mutex_unlock(&mutex_pista);
            strcat(buffer2,lista_chegadas[id-MAX_P].nome);
            strcat(buffer2," ARRIVAL 28R started");

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer2);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer2);
            sem_post(mutex_print);

            buffer2[0] = '\0';

            usleep(configuracao.ut*1000*configuracao.dur_aterragem); /** Tempo que o AVIAO demora a ATERRAR  **/

            strcat(buffer2,lista_chegadas[id-MAX_P].nome);
            strcat(buffer2," ARRIVAL 28R concluded");

            stats->tempo_medio_espera_aterrar = stats->tempo_medio_espera_aterrar + converte_tempo(*elapsed,configuracao.ut)-lista_chegadas[id-MAX_P].eta;

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(buffer2);
            sem_post(mutex_file_w);

            sem_wait(mutex_print);
            printf("%s\n",buffer2);
            sem_post(mutex_print);

            buffer2[0] = '\0';
            usleep(configuracao.ut*1000*configuracao.int_aterragens); /** Tempo para se voltar a poder usar a PISTA EM QUESTAO **/
            disp_pista[3]=0;
            disp_pista[5]=disp_pista[5]-1;
        }
    }
    sem_post(&empty_pista);

}

void *decides_departure_arrival(void* arg){
    int i=0;
    int condicao,vef1=1,vef2=1;
    int slot1,slot2,eta_aux;

    while(1){

        if( (fila_partida[0].slot != -1 || fila_chegada[0].slot != -1 ) && ( (disp_pista[0] + disp_pista[1] + disp_pista[2] + disp_pista[3]) < 2 ) ){

            if(fila_partida[0].departure <= converte_tempo(*elapsed,configuracao.ut) || fila_chegada[0].eta<= converte_tempo(*elapsed, configuracao.ut) ){

                    condicao = compara_arrival_dep(fila_partida[0].departure,fila_chegada[0].eta,fila_partida[1].departure,fila_chegada[1].eta, fila_partida[2].estado, fila_chegada[2].estado);

                    if(condicao == 1){

                        if((disp_pista[0]==0 || disp_pista[1]==0) && (disp_pista[2]==0 && disp_pista[3]==0) && disp_pista[5]==0){
                            disp_pista[4]=disp_pista[4]+1;
                            lista_partidas_slot[ fila_partida[0].slot ].estado = 1;
                            limpa_slot_partida(fila_partida,fila_partida[0].slot);
                        }
                    }
                    if(condicao == 3){
                        slot1 = fila_partida[0].slot;
                        slot2 = fila_partida[1].slot;

                        while(vef1==1){

                            if(lista_partidas_slot[slot1].departure <= converte_tempo(*elapsed,configuracao.ut)){

                                if((disp_pista[0]==0 || disp_pista[1]==0) && (disp_pista[2]==0 && disp_pista[3]==0) && disp_pista[5]==0){
                                    vef1=0;

                                    lista_partidas_slot[ slot1 ].estado = 1;
                                    disp_pista[4]=disp_pista[4]+1;
                                    limpa_slot_partida(fila_partida,slot1);

                                    while( vef2==1){
                                        if(lista_partidas_slot[slot2].departure <= converte_tempo(*elapsed,configuracao.ut)  && (disp_pista[0]==0 || disp_pista[1]==0) && (disp_pista[2]==0 && disp_pista[3]==0)){
                                            vef2=0;
                                        }
                                    }
                                    disp_pista[4]=disp_pista[4]+1;
                                    lista_partidas_slot[ slot2 ].estado = 1;

                                    limpa_slot_partida(fila_partida,slot2);
                                    vef2=1;
                                }
                            }
                        }
                        vef1=1;
                    }
                    if(condicao == 2){

                        if((disp_pista[2]==0 || disp_pista[3]==0) && (disp_pista[0]==0 && disp_pista[1]==0 && disp_pista[4]==0)){
                            disp_pista[5]=disp_pista[5]+1;
                            lista_chegadas_slot[ fila_chegada[0].slot ].estado = 2;
                            limpa_slot_chegada(fila_chegada,fila_chegada[0].slot);
                        }
                    }
                    if(condicao == 4){
                        slot1 = fila_chegada[0].slot;
                        slot2 = fila_chegada[1].slot;

                        while(vef1==1){

                            if(lista_chegadas_slot[slot1].eta <= converte_tempo(*elapsed,configuracao.ut)){

                                if((disp_pista[2]==0 || disp_pista[3]==0) && (disp_pista[0]==0 && disp_pista[1]==0) && disp_pista[4]==0){

                                    vef1=0;
                                    disp_pista[5]=disp_pista[5]+1;
                                    lista_chegadas_slot[ slot1 ].estado = 2;
                                    limpa_slot_chegada(fila_chegada,slot1);

                                    while( vef2==1){
                                        if(lista_chegadas_slot[slot2].eta <= converte_tempo(*elapsed,configuracao.ut)  && (disp_pista[2]==0 || disp_pista[3]==0) && (disp_pista[0]==0 && disp_pista[1]==0)){
                                            vef2=0;
                                        }
                                    }
                                    lista_chegadas_slot[ slot2 ].estado = 2;
                                    disp_pista[5]=disp_pista[5]+1;
                                    limpa_slot_chegada(fila_chegada,slot2);
                                    vef2=1;
                                }
                            }
                        }
                        vef1=1;
                    }
            }

        }
        if(fila_chegada[5].slot!=-1 && (fila_chegada[5+i].eta <= (converte_tempo(*elapsed,configuracao.ut) + configuracao.hold_max ) ) ){ /** CENARIO HOLDING --> Para o caso de um voo ter mais que 5 voos a sua frente na FILA_CHEGADAS **/

            while(fila_chegada[5 + i].slot!=-1 && (fila_chegada[5+i].eta <= (converte_tempo(*elapsed,configuracao.ut) + configuracao.hold_max ) ) ){

                while( vef1==1 ){
                    eta_aux = ( rand() % (configuracao.hold_max - configuracao.hold_min))+ configuracao.hold_min;

                    if(fila_chegada[5+i].fuel - eta_aux > 0 ){
                        vef1 = 0 ;
                        fila_chegada[5+i].eta = fila_chegada[5+i].eta + eta_aux;
                        lista_chegadas_slot[ fila_chegada[5+i].slot ].eta = fila_chegada[5+i].eta;
                        if(fila_chegada[5+i].estado==5){
                            stats->manobras_hoolding_urgencia= stats->manobras_hoolding_urgencia + 1;
                        }
                        else{
                            stats->manobras_hoolding_aterragem = stats->manobras_hoolding_aterragem + 1;
                        }
                    }
                }
                vef1=1;
                lista_chegadas_slot[fila_chegada[5+i].slot].estado = 3;
                i++;

            }
            i=0;
            sort_fila_chegada(fila_chegada,configuracao.max_chegadas);
	    sort_fila_chegada_prioridade(fila_chegada,configuracao.max_chegadas);
        }
    }
}

Voo_chegada comando_voo_chegada(char buffer[]){
    int i,vef_escreve=0,vef_copia=0,conta_simbolos=0,pos=0,repetido=-1;
    Voo_chegada aux;
    char str_aux[MAX];

    for(i=0;i<strlen(buffer);i++){
        if(vef_escreve==1 && ( buffer[i]!=' ' && buffer[i]!=':' ) ){
            str_aux[pos]=buffer[i];
            pos++;
        }
        if( buffer[i]==' ' || buffer[i]==':' || buffer[i+1] == '\0' ){
            conta_simbolos++;
            if(vef_escreve==0){
                vef_escreve=1;
            }
            else{
                if(buffer[i+1]!=' ' || buffer[i+1] == '\0'){
                    str_aux[pos]='\0';
                    pos=0;
                    vef_copia=1;
                    if(conta_simbolos==5 || conta_simbolos==8 || conta_simbolos==11){
                        vef_copia=1;
                        vef_escreve=0;
                    }
                }
            }
        }
        if(conta_simbolos==2 && vef_copia==1){

            strcpy(aux.nome,str_aux);
            vef_copia=0;
        }
        if(conta_simbolos==5 && vef_copia==1){
            aux.init = atoi(str_aux);
            vef_copia=0;
        }
        if(conta_simbolos==8 && vef_copia==1){
            aux.eta = atoi(str_aux);
            vef_copia=0;
        }
        if(conta_simbolos==11 && vef_copia==1){
            aux.fuel = atoi(str_aux);
            vef_copia=0;
        }
    }


/** Vai verificar se o init recebido pelo pipe e superior ao instante em que o programa se encontra e vai verificar se o FUEL e suficiente **/
    repetido = compara_nome(aux.nome);
    if(converte_tempo(*elapsed,configuracao.ut)>=aux.init || ver_fuel(aux.init,aux.eta,aux.fuel)==0 || repetido == 0 || conta_simbolos!=11 || ( aux.init >= aux.eta )){
        aux.init=-1;
        if(repetido == 0){
            str_aux[0]='\0';
            strcat(str_aux,"ALREADY EXISTS A FLIGHT WITH THIS NAME: ");
            strcat(str_aux,aux.nome);
            printf("ALREADY EXISTS A FLIGHT WITH THIS NAME: %s",aux.nome);

            sem_wait(mutex_file_w);
            escreve_ficheiro_log(str_aux);
            sem_post(mutex_file_w);

        }

    }
    else{
        aux.iniciado=0;
    }

    return aux;
}

Voo_partida comando_voo_partida(char buffer[]){
    int i,vef_escreve=0,vef_copia=0,conta_simbolos=0,pos=0,repetido=-1;
    Voo_partida aux;
    char str_aux[MAX];

    for(i=0;i<strlen(buffer);i++){
        if(vef_escreve==1 && ( buffer[i]!=' ' && buffer[i]!=':' ) ){
            str_aux[pos]=buffer[i];
            pos++;
        }
        if( buffer[i]==' ' || buffer[i]==':' || buffer[i+1] == '\0' ){
            conta_simbolos++;
            if(vef_escreve==0){
                vef_escreve=1;
            }
            else{
                if(buffer[i+1]!=' ' || buffer[i+1] == '\0'){
                    str_aux[pos]='\0';
                    pos=0;
                    vef_copia=1;
                    if(conta_simbolos==5 || conta_simbolos==8){
                        vef_copia=1;
                        vef_escreve=0;
                    }
                }
            }
        }
        if(conta_simbolos==2 && vef_copia==1){

            strcpy(aux.nome,str_aux);
            vef_copia=0;
        }
        if(conta_simbolos==5 && vef_copia==1){
            aux.init = atoi(str_aux);
            vef_copia=0;
        }
        if(conta_simbolos==8 && vef_copia==1){
            aux.descolagem = atoi(str_aux);
            vef_copia=0;
        }

    }

    /** Vai verificar se o init recebido pelo pipe e superior ao instante em que o programa se encontra**/
    repetido = compara_nome(aux.nome);
    if(converte_tempo(*elapsed,configuracao.ut)>=aux.init || repetido==0 || conta_simbolos!=8 || ( aux.init >= aux.descolagem) ){
        aux.init=-1;
        if(repetido == 0){
            str_aux[0]='\0';
            strcat(str_aux,"ALREADY EXISTS A FLIGHT WITH THIS NAME: ");
            strcat(str_aux,aux.nome);
            printf("ALREADY EXISTS A FLIGHT WITH THIS NAME: %s",aux.nome);


            sem_wait(mutex_file_w);
            escreve_ficheiro_log(str_aux);
            sem_post(mutex_file_w);

        }
    }
    else{
        aux.iniciado=0;
    }

    return aux;
}

void init_stats(){
    stats->total_voos_criados=0;
    stats->total_voos_aterraram=0;
    stats->tempo_medio_espera_aterrar=0;
    stats->total_voos_descolaram=0;
    stats->tempo_medio_espera_descolar=0;
    stats->manobras_hoolding_aterragem=0;
    stats->manobras_hoolding_urgencia=0;
    stats->voos_redirecionados=0;
    stats->voos_rejeitados=0;
}

