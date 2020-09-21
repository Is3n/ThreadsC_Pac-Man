#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

// Dimensions de la grille de jeu
#define NB_LIGNE 21
#define NB_COLONNE 17

// Macros utilisees dans le tableau tab
#define VIDE         0
#define MUR          1
#define PACMAN       -2
#define PACGOM       -3
#define SUPERPACGOM  -4
#define BONUS        -5

// Autres macros
#define LENTREE 15
#define CENTREE 8

int tab[NB_LIGNE][NB_COLONNE]
={ {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
   {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
   {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,0,1,1,1,0,1,0,1,1,0,1},
   {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
   {1,1,1,1,0,1,1,0,1,0,1,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,1},
   {0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
   {1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
   {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
   {1,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
   {1,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,1},
   {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
   {1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1},
   {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
   {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

void DessineGrilleBase();
void Attente(int milli);

typedef struct
{
   int L;
   int C;
   int couleur;
   int cache;
} S_FANTOME;

///////////////////////////////////////////////////////////////////////////////////////////////////

void* ThreadPacMan(void*);
void* ThreadEvent(void*);
void* ThreadPacGom(void*);
void* ThreadScore(void*);
void* ThreadBonus(void*);
void* ThreadCompteurFantomes(void*);
void* ThreadFantome(S_FANTOME*);
void* ThreadVies(void*);
void* ThreadTimeOut(void*);

pthread_t tidPacMan;
pthread_t tidEvent;
pthread_t tidPacGom;
pthread_t tidScore;
pthread_t tidBonus;
pthread_t tidCompteurFantomes;
pthread_t tidFantome;
pthread_t tidVies;
pthread_t tidTimeOut;

void* finThreadFantome(S_FANTOME*);

pthread_mutex_t mutexTab;
pthread_mutex_t mutexDir;
pthread_mutex_t mutexNbPacGom;
pthread_mutex_t mutexDelai;
pthread_mutex_t mutexScore;
pthread_mutex_t mutexNbFantomes;
pthread_mutex_t mutexMode;

pthread_cond_t condNbPacGom;
pthread_cond_t condScore;
pthread_cond_t condNbFantomes;
pthread_cond_t condMode;

pthread_key_t cleFantome;

void HandlerKEYLEFT(int sig);
void HandlerKEYRIGHT(int sig);
void HandlerKEYUP(int sig);
void HandlerKEYDOWN(int sig);
void HandlerSIGALRM(int sig);
void HandlerSIGCHLD(int sig);

int l=15, c=8;
int dir=HAUT;
int delai=300;
int NbPacGom;
int score=0;
bool MAJScore;
int nbRouge=0, nbVert=0, nbMauve=0, nbOrange=0;
int mode=1;

struct sigaction Act;
///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{

//Ouverture de la fenetre graphique
  printf("(MAIN %d) Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
  if (OuvertureFenetreGraphique() < 0)
  {
    printf("Erreur de OuvrirGrilleSDL\n");
    fflush(stdout);
    exit(1);
  }

  DessineGrilleBase();

//Armement des signaux

  sigemptyset(&Act.sa_mask);
  Act.sa_flags = 0;
	
  Act.sa_handler = HandlerKEYLEFT;
  if(sigaction(SIGINT,&Act,NULL)==-1)
  {
    perror("Erreur lors de l'armement du signal SIGINT: ");
    exit(1);
  }

  Act.sa_handler = HandlerKEYRIGHT;
  if(sigaction(SIGHUP,&Act,NULL)==-1)
  {
    perror("Erreur lors de l'armement du signal SIGHUP: ");
    exit(1);
  }

  Act.sa_handler = HandlerKEYUP;
  if(sigaction(SIGUSR1,&Act,NULL)==-1)
  {
    perror("Erreur lors de l'armement du signal SIGUSR1: ");
    exit(1);
  }

  Act.sa_handler = HandlerKEYDOWN;
  if(sigaction(SIGUSR2,&Act,NULL)==-1)
  {
    perror("Erreur lors de l'armement du signal SIGUSR2: ");
    exit(1);
  }

  Act.sa_handler = HandlerSIGALRM;
  if(sigaction(SIGALRM,&Act,NULL)==-1)
  {
    perror("Erreur lors de l'armement du signal SIGALRM: ");
    exit(1);
  }

  Act.sa_handler = HandlerSIGCHLD;
  if(sigaction(SIGCHLD,&Act,NULL)==-1)
  {
    perror("Erreur lors de l'armement du signal SIGALRM: ");
    exit(1);
  }

//Initialisation des mutex
  pthread_mutex_init(&mutexTab, NULL); 
  pthread_mutex_init(&mutexDir, NULL);
  pthread_mutex_init(&mutexNbPacGom, NULL);
  pthread_mutex_init(&mutexDelai, NULL);
  pthread_mutex_init(&mutexScore, NULL);
  pthread_mutex_init(&mutexNbFantomes, NULL);

//Initialisation des variables de condition
  pthread_cond_init(&condNbPacGom, NULL);
  pthread_cond_init(&condScore, NULL);
  pthread_cond_init(&condNbFantomes, NULL);

//Initialisation des clés
 
  pthread_key_create(&cleFantome, NULL);

/*
  // Exemple d'utilisation de GrilleSDL et Ressources --> code a supprimer
  DessinePacMan(17,7,GAUCHE);  // Attention !!! tab n'est pas modifie --> a vous de le faire !!!
  DessineChiffre(14,25,9);
  DessineFantome(5,9,ROUGE,DROITE);
  DessinePacGom(7,4);
  DessineSuperPacGom(9,5);
  DessineFantomeComestible(13,15);
  DessineBonus(5,15);
*/  

  if(pthread_create(&tidPacGom,NULL,ThreadPacGom,NULL)!=0)
  {
  	perror("ERROR - Creation du thread PacGom");
  }
  Attente(1);
//Attend que la grille soit remplie avant de continuer

  if(pthread_create(&tidVies,NULL,ThreadVies,NULL)!=0)
  {
  	perror("ERROR - Creation du thread Vies");
  }

  if(pthread_create(&tidCompteurFantomes,NULL,ThreadCompteurFantomes,NULL)!=0)
  {
  	perror("ERROR - Creation du thread CompteurFantomes");
  }

  if(pthread_create(&tidScore,NULL,ThreadScore,NULL)!=0)
  {
  	perror("ERROR - Creation du thread Score");
  }

  if(pthread_create(&tidBonus,NULL,ThreadBonus,NULL)!=0)
  {
  	perror("ERROR - Creation du thread Bonus");
  }

  if(pthread_create(&tidEvent,NULL,ThreadEvent,NULL)!=0)
  {
  	perror("ERROR - Creation du thread Event");
  }

  pthread_join(tidEvent,NULL);


//Attend la fin du ThreadEvent puis termine le programme principal
  printf("Attente de 1500 millisecondes...\n");
  Attente(1500);
  // -------------------------------------------------------------------------
  
  // Fermeture de la fenetre
  printf("(MAIN %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
  FermetureFenetreGraphique();
  printf("OK\n"); fflush(stdout);

  exit(0);
}

//*********************************************************************************************
void Attente(int milli)
{
  struct timespec del;
  del.tv_sec = milli/1000;
  del.tv_nsec = (milli%1000)*1000000;
  nanosleep(&del,NULL);
}

//*********************************************************************************************
void DessineGrilleBase()
{
  for (int l=0 ; l<NB_LIGNE ; l++)
    for (int c=0 ; c<NB_COLONNE ; c++)
    {
      if (tab[l][c] == VIDE) EffaceCarre(l,c);
      if (tab[l][c] == MUR) DessineMur(l,c);
    }
}

//*********************************************************************************************

void* ThreadPacMan(void *p)
{

   sigset_t MasqueReception;
   sigfillset(&MasqueReception);
   sigdelset(&MasqueReception,SIGINT);
   sigdelset(&MasqueReception,SIGHUP);
   sigdelset(&MasqueReception,SIGUSR1);
   sigdelset(&MasqueReception,SIGUSR2);

   sigset_t MasqueBloquage;
   sigfillset(&MasqueBloquage);

   sigprocmask(SIG_SETMASK,&MasqueBloquage,NULL);

   l=15; 
   c=8;
   dir=GAUCHE;

   bool SpawnOk=false;

   while(!SpawnOk)
   {
	pthread_mutex_lock(&mutexTab);
	if(tab[l][c]==VIDE)
	{
	   tab[l][c] = PACMAN;
	   DessinePacMan(l,c,dir);
	   SpawnOk = true;
	}
	else
	{
	   pthread_mutex_unlock(&mutexTab);
	}
   }
   pthread_mutex_unlock(&mutexTab);

   while(1)
   {

      Attente(delai);
      sigprocmask(SIG_SETMASK,&MasqueReception,NULL);
      sigprocmask(SIG_SETMASK,&MasqueBloquage,NULL);

      pthread_mutex_lock(&mutexTab);

      if(dir==GAUCHE)
      {
         if(tab[l][c-1] == MUR)
	 {
	   pthread_mutex_unlock(&mutexTab);
           continue;
	 }
            c--;

	    if(tab[l][c]==PACGOM || tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexNbPacGom);
		NbPacGom--;
		pthread_mutex_unlock(&mutexNbPacGom);
		pthread_cond_signal(&condNbPacGom);
		//printf("NbPacGom : %d\n", NbPacGom);
	    }

	    if(tab[l][c]==PACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score++;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+5;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);

		mode=2;
		if(pthread_create(&tidTimeOut,NULL,ThreadTimeOut,NULL)!=0)
		{
		   perror("ERROR - Creation du thread Vies");
		}
	    }

	    if(tab[l][c]==BONUS)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+30;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(mode==2)
	    {
		if(tab[l][c]>0)
		{
		   printf("PacMan mange le fantome %d !\n", tab[l][c]);
		   pthread_kill(tab[l][c],SIGCHLD);
		}
	    }

            tab[l][c] = PACMAN;
            DessinePacMan(l,c,dir);
            tab[l][c+1] = VIDE;
            EffaceCarre(l,c+1);

	    if(tab[9][0]==PACMAN)
	    {
		c=16;
                tab[l][c] = PACMAN;
                DessinePacMan(l,c,dir);
                tab[l][0] = VIDE;
                EffaceCarre(l,0);
	    }
      }

      if(dir==DROITE)
      {
         if(tab[l][c+1] == MUR)
	 {
	   pthread_mutex_unlock(&mutexTab);
           continue;
	 }
            c++;

	    if(tab[l][c]==PACGOM || tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexNbPacGom);
		NbPacGom--;
		pthread_mutex_unlock(&mutexNbPacGom);
		pthread_cond_signal(&condNbPacGom);
		//printf("NbPacGom : %d\n", NbPacGom);
	    }

	    if(tab[l][c]==PACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score++;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+5;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);

		mode=2;
		if(pthread_create(&tidTimeOut,NULL,ThreadTimeOut,NULL)!=0)
		{
		   perror("ERROR - Creation du thread Vies");
		}
	    }

	    if(tab[l][c]==BONUS)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+30;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(mode==2)
	    {
		if(tab[l][c]>0)
		{
		   printf("PacMan mange le fantome %d !\n", tab[l][c]);
		   pthread_kill(tab[l][c],SIGCHLD);
		}
	    }

            tab[l][c] = PACMAN;
            DessinePacMan(l,c,dir);
            tab[l][c-1] = VIDE;
            EffaceCarre(l,c-1);

	    if(tab[9][16]==PACMAN)
	    {
		c=0;
                tab[l][c] = PACMAN;
                DessinePacMan(l,c,dir);
                tab[l][16] = VIDE;
                EffaceCarre(l,16);
	    }

      }

      if(dir==HAUT)
      {
         if(tab[l-1][c] == MUR)
	 {
	   pthread_mutex_unlock(&mutexTab);
           continue;
	 }

            l--;

	    if(tab[l][c]==PACGOM || tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexNbPacGom);
		NbPacGom--;
		pthread_mutex_unlock(&mutexNbPacGom);
		pthread_cond_signal(&condNbPacGom);
		//printf("NbPacGom : %d\n", NbPacGom);
	    }

	    if(tab[l][c]==PACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score++;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+5;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);

		mode=2;
		if(pthread_create(&tidTimeOut,NULL,ThreadTimeOut,NULL)!=0)
		{
		   perror("ERROR - Creation du thread Vies");
		}
	    }

	    if(tab[l][c]==BONUS)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+30;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(mode==2)
	    {
		if(tab[l][c]>0)
		{
		   printf("PacMan mange le fantome %d !\n", tab[l][c]);
		   pthread_kill(tab[l][c],SIGCHLD);
		}
	    }

            tab[l][c] = PACMAN;
            DessinePacMan(l,c,dir);
            tab[l+1][c] = VIDE;
            EffaceCarre(l+1,c);

      }

      if(dir==BAS)
      {
         if(tab[l+1][c] == MUR)
	 {
	   pthread_mutex_unlock(&mutexTab);
           continue;
	 }
            l++;

	    if(tab[l][c]==PACGOM || tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexNbPacGom);
		NbPacGom--;
		pthread_mutex_unlock(&mutexNbPacGom);
		pthread_cond_signal(&condNbPacGom);
		//printf("NbPacGom : %d\n", NbPacGom);
	    }

	    if(tab[l][c]==PACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score++;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(tab[l][c]==SUPERPACGOM)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+5;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);

		mode=2;
		if(pthread_create(&tidTimeOut,NULL,ThreadTimeOut,NULL)!=0)
		{
		   perror("ERROR - Creation du thread Vies");
		}
	    }

	    if(tab[l][c]==BONUS)
	    {
		pthread_mutex_lock(&mutexScore);
		score=score+30;
		MAJScore = true;
		pthread_mutex_unlock(&mutexScore);
		pthread_cond_signal(&condScore);
	    }

	    if(mode==2)
	    {
		if(tab[l][c]>0)
		{
		   printf("PacMan mange le fantome %d !\n", tab[l][c]);
		   pthread_kill(tab[l][c],SIGCHLD);
		}
	    }

            tab[l][c] = PACMAN;
            DessinePacMan(l,c,dir);
            tab[l-1][c] = VIDE;
            EffaceCarre(l-1,c);
         
      }
         pthread_mutex_unlock(&mutexTab);
   }

   pthread_exit(NULL);
}


void* ThreadEvent(void *p)
{
	
	EVENT_GRILLE_SDL event;
	char ok;
	
	ok = 0;
	while(!ok)
  	{
    	event = ReadEvent();
    	if (event.type == CROIX) ok = 1;
		if (event.type == CLAVIER)
		{
		  switch(event.touche)
		  {
		    case 'q' : ok = 1; break;
		    case KEY_LEFT : 
		    	printf("Fleche gauche !\n");
		    	pthread_kill(tidPacMan,SIGINT);
		    break;
		    case KEY_RIGHT : 
		    	printf("Fleche droite !\n"); 
		    	pthread_kill(tidPacMan,SIGHUP);
		    	break;
		    case KEY_UP : 
		    	printf("Fleche haute !\n");
		    	pthread_kill(tidPacMan,SIGUSR1);
		    	break;
		    case KEY_DOWN : 
		    	printf("Fleche basse !\n");
		    	pthread_kill(tidPacMan,SIGUSR2);
		    	break;
      		  }
    		}
  	}
  	pthread_exit(NULL);
}

void* ThreadPacGom(void *p)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

   int niveau = 1;

  while(1)
  {

   DessineChiffre(14,22,niveau);

   pthread_mutex_lock(&mutexTab);

   if(tab[2][1] == VIDE)
   {
     tab[2][1] = SUPERPACGOM;
     DessineSuperPacGom(2,1);
   }
   if(tab[2][15] == VIDE)
   {
     tab[2][15] = SUPERPACGOM;
     DessineSuperPacGom(2,15);
   }
   if(tab[15][1] == VIDE)
   {
     tab[15][1] = SUPERPACGOM;
     DessineSuperPacGom(15,1);
   }
   if(tab[15][15] == VIDE)
   {
     tab[15][15] = SUPERPACGOM;
     DessineSuperPacGom(15,15); 
   }


   for(int i=0; i<NB_LIGNE; i++)
   {
     for(int j=0; j<NB_COLONNE; j++)
    {
	 if(tab[i][j] != MUR && tab[i][j] != PACMAN && tab[i][j] != SUPERPACGOM && tab[i][j]<=0)
	 {
	    tab[i][j] = PACGOM;
	    DessinePacGom(i,j);
	    pthread_mutex_lock(&mutexNbPacGom);
   	    NbPacGom++;
    	    pthread_mutex_unlock(&mutexNbPacGom);

	 }
      }
   }

   tab[15][8] = VIDE;
   EffaceCarre(15,8);
   tab[8][8] = VIDE;
   EffaceCarre(8,8);
   tab[9][8] = VIDE;
   EffaceCarre(9,8);

   pthread_mutex_unlock(&mutexTab);

   pthread_mutex_lock(&mutexNbPacGom);
   while(NbPacGom>0)
   {
      pthread_cond_wait(&condNbPacGom, &mutexNbPacGom);

      if(NbPacGom < 10)
      {
 	DessineChiffre(12, 22, 0);
	DessineChiffre(12, 23, 0);
	DessineChiffre(12, 24, NbPacGom);
      }
      else if(NbPacGom < 100)
      {
	DessineChiffre(12, 22, 0);
	DessineChiffre(12, 23, NbPacGom/10);
	DessineChiffre(12, 24, NbPacGom%10);
      }
      else if(NbPacGom < 1000)
      {
   	DessineChiffre(12, 22, NbPacGom/100);
	DessineChiffre(12, 23, (NbPacGom%100) /10);
	DessineChiffre(12, 24,(NbPacGom%100) %10);
      }

   }
   pthread_mutex_unlock(&mutexNbPacGom);

   niveau++;
   printf("\nPassage au niveau : %d\n", niveau);

   pthread_mutex_lock(&mutexDelai);
   delai = delai/2;
   pthread_mutex_unlock(&mutexDelai);

  }
   pthread_exit(NULL);
}

void* ThreadScore(void *p)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

	DessineChiffre(16, 22, 0);
	DessineChiffre(16, 23, 0);
	DessineChiffre(16, 24, 0);
	DessineChiffre(16, 25, 0);
	while(1)
	{
		pthread_mutex_lock(&mutexScore);
		while(!MAJScore)
			pthread_cond_wait(&condScore, &mutexScore);
		pthread_mutex_unlock(&mutexScore);
		
		//Dessiner le chiffre
		if(score < 10)
		{
			DessineChiffre(16, 25, score);
		}
		else if(score < 100)
		{
			DessineChiffre(16, 24, score/10);
			DessineChiffre(16, 25, score%10);
		}
		else if(score < 1000)
		{
			DessineChiffre(16, 23, score/100);
			DessineChiffre(16, 24, (score%100) /10);
			DessineChiffre(16, 25, (score%100) %10);
		}
		else if(score < 10000)
		{
			DessineChiffre(16, 22, score/1000);
			DessineChiffre(16, 23, (score%1000)/100);
			DessineChiffre(16, 24, ((score%1000)%100) /10);
			DessineChiffre(16, 25, ((score%1000)%100) %10);
		}
		MAJScore = false;
	}
}

void* ThreadBonus(void *p)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

   int RandomWait, i, j;
   bool test;

   while(1)
   {
	RandomWait = rand()%11 + 10;
	RandomWait = RandomWait*1000;
	test=false;

	Attente(RandomWait);
	
	while(!test)
	{
	   i = rand()%21;
	   j = rand()%17;
	   
	   if((i==9 && j==8) || (i==8 && j==8))
		test=false;
	   else if(tab[i][j]==VIDE)
	   {
		tab[i][j]=BONUS;
		DessineBonus(i,j);
		test=true;
	   }
	}

	Attente(10000);

	if(tab[i][j]==BONUS)
	{
	   tab[i][j]=VIDE;
	   EffaceCarre(i,j);
	}
   }
}

void* ThreadCompteurFantomes(void *p)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

  while(1)
   {
	   S_FANTOME *iFantome = (S_FANTOME*)malloc(sizeof(S_FANTOME));

	   iFantome->L = 9;
	   iFantome->C = 8;
	   iFantome->cache = 0;

	   if(nbRouge<2)
	   {
		iFantome->couleur = ROUGE;
		nbRouge++;
	   }
	   else if(nbVert<2)
	   {
		iFantome->couleur = VERT;
		nbVert++;
	   }
	   else if(nbMauve<2)
	   {
		iFantome->couleur = MAUVE;
		nbMauve++;
	   }
	   else if(nbOrange<2)
	   {
		iFantome->couleur = ORANGE;
		nbOrange++;
	   }

	   if(pthread_create(&tidFantome, NULL, (void*(*)(void*))ThreadFantome, iFantome)!=0)
	   {
		perror("ERROR - Creation du thread Fantome");
	   }

	pthread_mutex_lock(&mutexNbFantomes);
	while(nbRouge==2 && nbVert==2 && nbMauve==2 && nbOrange==2)
	   pthread_cond_wait(&condNbFantomes, &mutexNbFantomes);
	pthread_mutex_unlock(&mutexNbFantomes);

	pthread_mutex_lock(&mutexMode);
	while(mode==2)
	   pthread_cond_wait(&condMode, &mutexMode);
	pthread_mutex_unlock(&mutexMode);

   }
}

void* ThreadFantome(S_FANTOME *fantome)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigdelset(&Masque,SIGCHLD);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

   pthread_setspecific(cleFantome, fantome);

   int dirFantome=HAUT, randomDir;
   int changeDir=false;
   int delaiFantome;
   bool SpawnOk=false;

   pthread_cleanup_push(finThreadFantome,fantome);

   while(!SpawnOk)
   {
	pthread_mutex_lock(&mutexTab);
	if(tab[9][8]==VIDE && tab[8][8]==VIDE)
	{
	   tab[9][8] = pthread_self();
	   DessineFantome(9,8,fantome->couleur,dirFantome);
	   SpawnOk = true;
	}
	else
	{
	   pthread_mutex_unlock(&mutexTab);
	}
   }
   pthread_mutex_unlock(&mutexTab);

   while(1)
   {
//Changement de direction à la fin de la boucle pour d'abord aller vers le haut au Spawn

      pthread_mutex_lock(&mutexDelai);
      delaiFantome=(5*delai)/3;
      pthread_mutex_unlock(&mutexDelai);

      Attente(delaiFantome);

      pthread_mutex_lock(&mutexTab);

      if(mode==1)
      {

	      if(dirFantome==GAUCHE)
	      {
		 if(tab[fantome->L][fantome->C-1] != MUR  && tab[fantome->L][fantome->C-1] <= 0)	    
		 {

		    if(tab[9][0]==pthread_self())
		    {
			fantome->C=16;
		        tab[fantome->L][fantome->C] = pthread_self();
		        DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);
		        tab[fantome->L][0] = VIDE;
		        EffaceCarre(fantome->L,0);
			continue;
		    }

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }


		    fantome->C--;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;

		    if(tab[fantome->L][fantome->C]==PACMAN)
			pthread_cancel(tidPacMan);

		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);

		 }
		 else
		 {
		   changeDir=true;
		 }
	      }

	      if(dirFantome==DROITE)
	      {
		 if(tab[fantome->L][fantome->C+1] != MUR  && tab[fantome->L][fantome->C+1] <= 0)
		 {

		    if(tab[9][16]==pthread_self())
		    {
			fantome->C=0;
		        tab[fantome->L][fantome->C] = pthread_self();
		        DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);
		        tab[fantome->L][16] = VIDE;
		        EffaceCarre(fantome->L,16);
			continue;
		    }

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }

		    fantome->C++;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;

		    if(tab[fantome->L][fantome->C]==PACMAN)
			pthread_cancel(tidPacMan);

		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);
		 }
		 else
		 {
		   changeDir=true;
		 }
	      }


	      if(dirFantome==HAUT)
	      {
		 if(tab[fantome->L-1][fantome->C] != MUR  && tab[fantome->L-1][fantome->C] <= 0)
		 {

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }

		    fantome->L--;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;

		    if(tab[fantome->L][fantome->C]==PACMAN)
			pthread_cancel(tidPacMan);

		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);


		 }
		 else
		 {
		   changeDir=true;
		 }
	      }

	      if(dirFantome==BAS)
	      {
		 if(tab[fantome->L+1][fantome->C] != MUR  && tab[fantome->L+1][fantome->C] <= 0)
		 {

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }

		    fantome->L++;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;

		    if(tab[fantome->L][fantome->C]==PACMAN)
			pthread_cancel(tidPacMan);

		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantome(fantome->L, fantome->C, fantome->couleur, dirFantome);

		 }

		 else
		 {
		   changeDir=true;
		 }
	      }
      }

      if(mode==2)
      {

	      if(dirFantome==GAUCHE)
	      {
		 if(tab[fantome->L][fantome->C-1] != MUR  && tab[fantome->L][fantome->C-1] <= 0 && tab[fantome->L][fantome->C-1] != PACMAN )	    
		 {

		    if(tab[9][0]==pthread_self())
		    {
			fantome->C=16;
		        tab[fantome->L][fantome->C] = pthread_self();
		        DessineFantomeComestible(fantome->L, fantome->C);
		        tab[fantome->L][0] = VIDE;
		        EffaceCarre(fantome->L,0);
			continue;
		    }

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }


		    fantome->C--;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;

		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantomeComestible(fantome->L, fantome->C);

		 }
		 else
		 {
		   changeDir=true;
		 }
	      }

	      if(dirFantome==DROITE)
	      {
		 if(tab[fantome->L][fantome->C+1] != MUR  && tab[fantome->L][fantome->C+1] <= 0 && tab[fantome->L][fantome->C+1] != PACMAN )
		 {

		    if(tab[9][16]==pthread_self())
		    {
			fantome->C=0;
		        tab[fantome->L][fantome->C] = pthread_self();
		        DessineFantomeComestible(fantome->L, fantome->C);
		        tab[fantome->L][16] = VIDE;
		        EffaceCarre(fantome->L,16);
			continue;
		    }

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }

		    fantome->C++;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;


		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantomeComestible(fantome->L, fantome->C);
		 }
		 else
		 {
		   changeDir=true;
		 }
	      }


	      if(dirFantome==HAUT)
	      {
		 if(tab[fantome->L-1][fantome->C] != MUR  && tab[fantome->L-1][fantome->C] <= 0 && tab[fantome->L-1][fantome->C] != PACMAN)
		 {

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }

		    fantome->L--;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;


		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantomeComestible(fantome->L, fantome->C);


		 }
		 else
		 {
		   changeDir=true;
		 }
	      }

	      if(dirFantome==BAS)
	      {
		 if(tab[fantome->L+1][fantome->C] != MUR  && tab[fantome->L+1][fantome->C] <= 0 && tab[fantome->L+1][fantome->C] != PACMAN)
		 {

		    if(fantome->cache==VIDE)
		    {
			tab[fantome->L][fantome->C] = VIDE;
			EffaceCarre(fantome->L,fantome->C);
		    }
		    if(fantome->cache==PACGOM)
		    {
			tab[fantome->L][fantome->C] = PACGOM;
			DessinePacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==SUPERPACGOM)
		    {
			tab[fantome->L][fantome->C] = SUPERPACGOM;
			DessineSuperPacGom(fantome->L,fantome->C);
		    }
		    if(fantome->cache==BONUS)
		    {
			tab[fantome->L][fantome->C] = BONUS;
			DessineBonus(fantome->L,fantome->C);
		    }

		    fantome->L++;

		    if(tab[fantome->L][fantome->C]==VIDE)
			fantome->cache=VIDE;

		    if(tab[fantome->L][fantome->C]==PACGOM)
			fantome->cache=PACGOM;

		    if(tab[fantome->L][fantome->C]==SUPERPACGOM)
			fantome->cache=SUPERPACGOM;

		    if(tab[fantome->L][fantome->C]==BONUS)
			fantome->cache=BONUS;


		    tab[fantome->L][fantome->C] = pthread_self();
		    DessineFantomeComestible(fantome->L, fantome->C);

		 }

		 else
		 {
		   changeDir=true;
		 }
	      }
      }

      if(changeDir)  
      {
         randomDir=rand()%4+1;

         if(randomDir==1)
	   dirFantome=GAUCHE;
         else if(randomDir==2)
	   dirFantome=DROITE;
         else if(randomDir==3)
   	   dirFantome=HAUT;
         else if(randomDir==4)
	   dirFantome=BAS;

	 changeDir=false;
      }
      pthread_mutex_unlock(&mutexTab);
   }

   pthread_cleanup_pop(1);

}

void* ThreadVies(void *p)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

   int nbVies = 3;

   DessineChiffre(18,22,nbVies);

   while(nbVies>0)
   {

	if(pthread_create(&tidPacMan,NULL,ThreadPacMan,NULL)!=0)
	{
	   perror("ERROR - Creation du thread PacMan");
	}

	pthread_join(tidPacMan,NULL);

	nbVies--;

	DessineChiffre(18,22,nbVies);
	printf("\nNombre de vies restantes : %d\n", nbVies);

   }
   pthread_mutex_lock(&mutexTab);
   DessineGameOver(9,4);
}

void* ThreadTimeOut(void *p)
{
   sigset_t Masque;
   sigfillset(&Masque);
   sigdelset(&Masque,SIGALRM);
   sigprocmask(SIG_SETMASK,&Masque,NULL);

   int duree;

  while(mode==2)  
  {   
    duree = rand()%8+8;

    alarm(duree); 
    pause();

  } 

}

void* finThreadFantome(S_FANTOME *fantome)
{
   pthread_mutex_lock(&mutexScore);
   score=score+50;
   MAJScore = true;
   pthread_mutex_unlock(&mutexScore);
   pthread_cond_signal(&condScore);

   if(fantome->cache==PACGOM || fantome->cache==SUPERPACGOM)
   {
	pthread_mutex_lock(&mutexNbPacGom);
	NbPacGom--;
	pthread_mutex_unlock(&mutexNbPacGom);
	pthread_cond_signal(&condNbPacGom);
	//printf("NbPacGom : %d\n", NbPacGom);
   }

   if(fantome->cache==PACGOM)
   {
	pthread_mutex_lock(&mutexScore);
	score++;
	MAJScore = true;
	pthread_mutex_unlock(&mutexScore);
	pthread_cond_signal(&condScore);
   }

   if(fantome->cache==SUPERPACGOM)
   {
	pthread_mutex_lock(&mutexScore);
	score=score+5;
	MAJScore = true;
	pthread_mutex_unlock(&mutexScore);
	pthread_cond_signal(&condScore);

	mode=2;
	if(pthread_create(&tidTimeOut,NULL,ThreadTimeOut,NULL)!=0)
	{
	   perror("ERROR - Creation du thread Vies");
	}
   }

   if(fantome->cache==BONUS)
   {
	pthread_mutex_lock(&mutexScore);
	score=score+30;
	MAJScore = true;
	pthread_mutex_unlock(&mutexScore);
	pthread_cond_signal(&condScore);
   }

   pthread_mutex_lock(&mutexNbFantomes);
   if(fantome->couleur==ROUGE)
	nbRouge--;
   else if(fantome->couleur==VERT)
	nbVert--;
   else if(fantome->couleur==MAUVE)
	nbMauve--;
   else if(fantome->couleur==ORANGE)
	nbOrange--;
   pthread_mutex_unlock(&mutexNbFantomes);
   pthread_cond_signal(&condNbFantomes);
}

//*********************************************************************************************

void HandlerKEYLEFT(int sig)
{
   //printf("--- HandlerKEYLEFT ---\n");

   dir=GAUCHE;
	
}

void HandlerKEYRIGHT(int sig)
{	
   //printf("--- HandlerKEYRIGHT ---\n");

   dir=DROITE;
	
}

void HandlerKEYUP(int sig)
{	
   //printf("--- HandlerKEYUP ---\n");

   dir=HAUT;
	
}

void HandlerKEYDOWN(int sig)
{	
   //printf("--- HandlerKEYDOWN ---\n");

   dir=BAS;
	
}

void HandlerSIGALRM(int sig) //Quand on est en mode fantomes comestibles
{
   pthread_mutex_lock(&mutexMode);
   mode=1;
   pthread_mutex_unlock(&mutexMode);
   pthread_cond_signal(&condMode);
}

void HandlerSIGCHLD(int sig) //Quand PacMan mange un fantome
{
   pthread_exit(NULL);
}





