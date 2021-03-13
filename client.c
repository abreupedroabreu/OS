//
//  main.c
//  menu
//
//  Created by Pedro Abreu on 09/11/2019.
//  Copyright © 2019 Pedro Abreu. All rights reserved.
//

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h> // include POSIX semaphores
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

#define MAX 300
#define PIPE_NAME "input_pipe"
#define SIZE 10000

typedef struct voo_partida{
    char voo[MAX];

}Voo_partida;

typedef struct voo_chegada{

    char voo[MAX];

}Voo_chegada;

void sigint(int signum);

void tira_enter(char str[]);


int main(){

    signal(SIGINT,sigint);
    char nome_partida[MAX],init_partida[MAX],takeoff[MAX];
    char nome_chegada[MAX],init_chegada[MAX],eta[MAX],fuel[MAX];

    Voo_chegada lista_chegada[SIZE];
    Voo_partida lista_partida[SIZE];

    int pos_partida=0;
    int pos_chegada=0;

    int comando_geral;
    char comando[MAX];
    int fd;
    char buffer[MAX];
    fd_set write_set;
    int i,vef=1;


    /* PIPES */





    if((fd = open(PIPE_NAME, O_WRONLY)) < 0){
        perror("Cannot open pipe for writing\n");
        exit(0);
    }

    FD_ZERO(&write_set);






    printf(" ***************************************\n");
    printf(" ***************************************\n");
    printf(" ------- BEM VINDO AO AEROPORTO-------\n");
    printf(" ***************************************\n");
    printf(" ***************************************\n");




    do{
        printf("\n\n1-ADICIONAR VOO DE PARTIDA\n2-ADICIONAR VOO DE CHEGADA\n3-VER LISTA DE VOOS DE PARTIDA\n4-VER LISTA DE VOOS DE CHEGADA");

        printf("\nIntroduza um comando: ");
        fgets(comando,MAX,stdin);
        tira_enter(comando);
        comando_geral = atoi(comando);


        switch(comando_geral){
            default:
                printf("\n\nComando Inválido\n");
                break;
            case 1:
                printf("\n\nIntroduza um nome para o Voo de Partida: ");
                fgets(nome_partida,MAX,stdin);
                tira_enter(nome_partida);

                printf("Introduza um init: ");
                fgets(init_partida,MAX,stdin);
                tira_enter(init_partida);

                printf("Introduza um takeoff: ");
                fgets(takeoff,MAX,stdin);
                tira_enter(takeoff);
                for(i=0;i<strlen(nome_partida);i++){
                   if(nome_partida[i]==' '){
                       printf("\nERRO NA CRIACAO DO NOME, NAO PODE CONTER ESPACOS\n\n ");
                       vef=0;
                       break;
                    }
                    if(nome_partida[i]==':'){
                        printf("\nERRO NA CRIACAO DO NOME, NAO PODE CONTER ':' DENTRO DO NOME\n ");
                        vef=0;
                        break;

                    }


                }

                if(vef==1){
                    buffer[0]='\0';

                    /*CONCATENACAO DAS STRINGS*/
                    strcat(buffer,"DEPARTURE ");
                    strcat(buffer,nome_partida);
                    strcat(buffer," init: ");
                    strcat(buffer,init_partida);
                    strcat(buffer," takeoff: ");
                    strcat(buffer,takeoff);


                    strcpy(lista_partida[pos_partida].voo,buffer);
                    write(fd,&buffer,sizeof(buffer));
                    pos_partida++;



                }
                vef=1;


                break;

            case 2:
                printf("\n\nIntroduza um nome para o Voo de Chegada: ");
                fgets(nome_chegada,MAX,stdin);
                tira_enter(nome_chegada);

                printf("Introduza um init: ");
                fgets(init_chegada,MAX,stdin);
                tira_enter(init_chegada);

                printf("Introduza um ETA: ");
                fgets(eta,MAX,stdin);
                tira_enter(eta);

                printf("Introduza um quantidade de combustível: ");
                fgets(fuel, MAX, stdin);
                tira_enter(fuel);

                /* PIPE*/
                for(i=0;i<strlen(nome_chegada);i++){

                    if(nome_chegada[i]==' '){
                        printf("\nERRO NA CRIACAO DO NOME, NAO PODE CONTER ESPACOS\n\n ");
                        vef=0;
                        break;
                    }
                    if(nome_chegada[i]==':'){
                        printf("\nERRO NA CRIACAO DO NOME, NAO PODE CONTER ':' DENTRO DO NOME\n ");
                        vef=0;
                        break;

                    }

                }
                if(vef==1){

                    buffer[0]='\0';

                    /*CONCATENACAO DAS STRINGS*/
                    strcat(buffer,"ARRIVAL ");
                    strcat(buffer,nome_chegada);
                    strcat(buffer," init: ");
                    strcat(buffer,init_chegada);
                    strcat(buffer," eta: ");
                    strcat(buffer,eta);
                    strcat(buffer," fuel: ");
                    strcat(buffer,fuel);


                    strcpy(lista_chegada[pos_chegada].voo,buffer);
                    write(fd,&buffer,sizeof(buffer));
                    pos_chegada++;



                }
                vef=1;


                break;

            case 3:
                printf("\n\nACEDER Á LISTA DE VOOS DE PARTIDA\n\n");
                printf("\n-----LISTA VOOS DE PARTIDA-----\n");
                for(i=0;i<pos_partida;i++){
                    printf("%s\n",lista_partida[i].voo);


                }
                break;
            case 4:
                printf("\n\nACEDER Á LISTA DE VOOS DE CHEGADA\n\n");
                printf("\n-----LISTA VOOS DE CHEGADA-----\n");
                for(i=0;i<pos_chegada;i++){
                    printf("%s\n",lista_chegada[i].voo);
                }
                break;


        }


    }while(1);
    printf("\n\nVOO ENVIADO PARA O PIPE COM SUCESSO!");

}


void tira_enter(char str[]){
    if(str[strlen(str)-1]=='\n'){
        str[strlen(str)-1]='\0';
    }
}
void sigint(int signum){
    char option[2];

    printf("\nCaracter Ctrl-Z clicado, gostaria de sair do programa?  ( s|n ) ");
    scanf("%s",option);
    if(option[0]=='s'){
        printf("\nOK, a sair do programa.....");
        exit(0);
    }

}
