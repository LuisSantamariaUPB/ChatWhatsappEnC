// C Program for Message Queue (Writer Process) 
#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <pthread.h> 
#include <string.h>
#include <stdlib.h> //para el atoi
#include <unistd.h>
#include <ctype.h>

#define SIZE 20
#define MESSAGE_LENGTH 280

// structure for message queue 
struct _mbuffer { 
	long mtype; 
	char mtext[MESSAGE_LENGTH]; 
} message[2]; 

int msgid; 
int msgid_desconectar;
int msgid_comunicacion;

int count_users = 0;
char *bufferSalida = "exit";


long arrayPid[SIZE];


//void* th_sendMessage (void* unused);
void* th_receiveMessage(void* unused);
void* th_receiveSincronization(void* unused);
void* th_disconnectUser(void* unused);
void  cerrarServicio(void);
void broadcast(char *msg);
int validarIdentificador(char *msg);


int main(int argc, char const *argv[])
{
    
     key_t key;
    key_t key_desconectar;
    key_t key_comunicacion;


    key = ftok("/tmp/msgid1.txt", 999); //key id
    msgid = msgget(key, 0666 | IPC_CREAT); //queue creation

    key_desconectar = ftok("/tmp/msgid2.txt", 998); //key id
    msgid_desconectar = msgget(key_desconectar, 0666 | IPC_CREAT);

    key_comunicacion = ftok("/tmp/msgid3.txt", 997); //key id
    msgid_comunicacion = msgget(key_comunicacion, 0666 | IPC_CREAT);
    

    pthread_t id_sincronization;
    pthread_t id_disconnectUser;
    pthread_t id_receiveMessage;
    

    pthread_create (&id_sincronization,NULL,&th_receiveSincronization,NULL);
    pthread_create (&id_disconnectUser,NULL,&th_disconnectUser,NULL);
    pthread_create (&id_receiveMessage,NULL,&th_receiveMessage,NULL);
   
    char bufferComando[4];
    while (1)
    {
        
        printf("escriba exit si desea cerrar el servidor\n");
        scanf("%s",bufferComando);
        if (strncmp(bufferComando,bufferSalida,4) == 0)
        {
            broadcast("desconectarse");
            sleep(2);
            if(msgctl(msgid, IPC_RMID, NULL) == -1) perror ("msfctl msgid :");
            if(msgctl(msgid_desconectar, IPC_RMID, NULL) == -1) perror ("msfctl msgid_desconectar:");
            if(msgctl(msgid_comunicacion, IPC_RMID, NULL) == -1) perror ("msfctl msgid_comunicacion :");

            printf("\n\n---se cerraron servicios---\n");
            break;
            
        }
    }

    pthread_join (id_sincronization,NULL);
    pthread_join (id_disconnectUser,NULL);
    pthread_join (id_receiveMessage,NULL);

    

    return 0;
}


void* th_receiveMessage(void* unused){
    char aux_receiveMessage[MESSAGE_LENGTH];
    struct _mbuffer buffer;
    while (1)
    {

        //memset(buffer.mtext,0,MESSAGE_LENGTH); 
        while (1)
        {   
            if( msgrcv(msgid_comunicacion, &buffer, sizeof(buffer.mtext), 10, 0) != -1){
                break;
            }

        }
        //printf("llegó el mensaje \"%s\"\n",buffer.mtext);
        memset(aux_receiveMessage,0,MESSAGE_LENGTH);
        sprintf(aux_receiveMessage,"%s", buffer.mtext);
        printf("%s\n",aux_receiveMessage);
        //printf("se mandará mensaje al broadcast\n");
        broadcast(aux_receiveMessage);
        //printf("se mandó mensaje al broadcast\n");
        
    }
    
    return NULL;
}

void* th_receiveSincronization(void* unused){
    //se usa aux
    int length;
    while (1)
    {
        memset(message[1].mtext,0,MESSAGE_LENGTH); //limpio aux
        while (1) //garantizo que el servidor leyo un mensaje
        {
            
            if( msgrcv(msgid, &message[1], sizeof(message[1].mtext), 11, 0) != -1){
                break;
            }
           
        }
        printf("Nuevo cliente: %s\n",message[1].mtext);

        memset(message[0].mtext,0,MESSAGE_LENGTH); //limpio message
        if (count_users<SIZE)
        {
            count_users++;
            sprintf(message[0].mtext,"%s","exitoso");
            arrayPid[count_users-1] =  (long) atoi(message[1].mtext);
            //char aux[MESSAGE_LENGTH];
        }
        else{
            sprintf(message[0].mtext,"%s","limite de usuarios excedido");
        }
        

        message[0].mtype = (long) atoi(message[1].mtext);

        length = strlen(message[0].mtext);
        
        if( msgsnd(msgid, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");
        if ((count_users-1) < SIZE)
        {
            char aux_receiveSincronization[MESSAGE_LENGTH];
            sprintf(aux_receiveSincronization,"Servidor: Nuevo cliente conectado %ld", arrayPid[count_users-1]);
            broadcast(aux_receiveSincronization);
        }
        
        
    }
    
    return NULL;
}

void* th_disconnectUser(void* unused){
    //se usa aux

    while (1) //garantizo que el servidor leyo un mensaje
        {

            memset(message[1].mtext,0,MESSAGE_LENGTH); //limpio aux
            while (1)
            {
                if( msgrcv(msgid_desconectar, &message[1], sizeof(message[1].mtext), 99, 0) != -1)
                    break;
            }
            
            char bufferUsuario[10];
            memset(bufferUsuario,0,10); //limpio buffer
            for (int i = 0; i < count_users; i++)
            {
                sprintf(bufferUsuario,"%ld",arrayPid[i]);
                strcat(bufferUsuario,"\0");
                if (strcmp(bufferUsuario, message[1].mtext) == 0)
                {
                    int length;
                //Mandar mensaje al usuario: "te has desconectado"

                    memset(message[0].mtext,0,MESSAGE_LENGTH); //limpio aux
                    message[0].mtype = (long) atoi(message[1].mtext);
                    //printf("Variable %d\n", (int)message[0].mtype);
                    sprintf(message[0].mtext,"%s","te has desconectado");
                    length = strlen(message[0].mtext);
                    if( msgsnd(msgid_desconectar, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");

                //Reorganizar array de Pid's
                    for (int j = i; i < (count_users-1); i++)
                    {
                        arrayPid[j] = arrayPid[j+1];
                    }
                    count_users--;
                //Mandar mensaje a todos los usuarios
                    char aux_disconnectUser[MESSAGE_LENGTH];
                    sprintf(aux_disconnectUser,"Servidor: Cliente desconectado: %s", bufferUsuario);
                    broadcast(aux_disconnectUser);
                
                    break;
                }
            }   
        }

    return NULL;
}


void broadcast(char *msg){
    printf("entro al broadcast");
    // printf("numero de usuarios: %d\n",count_users);
    int length;
    //struct _mbuffer buffer;

    //sprintf(buffer.mtext,"%s",msg);
    sprintf(message[0].mtext,"%s",msg);
    for (int i = 0; i < count_users; i++)
    {   
        //memset(message[0].mtext,0,MESSAGE_LENGTH); //limpio aux
        message[0].mtype = arrayPid[i];
        length = strlen(message[0].mtext);
        if( msgsnd(msgid_comunicacion, &message[0], length+1, 0) == -1 ) perror("msgsnd fails: ");
    }
    printf("Se mandó el mensaje: \"%s\"\n",message[0].mtext);
    return;
}



